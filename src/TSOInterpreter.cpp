/* Copyright (C) 2014-2017 Carl Leonardsson
 *
 * This file is part of Nidhugg.
 *
 * Nidhugg is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nidhugg is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "Debug.h"
#include "TSOInterpreter.h"

#if defined(HAVE_LLVM_IR_LLVMCONTEXT_H)
#include <llvm/IR/LLVMContext.h>
#elif defined(HAVE_LLVM_LLVMCONTEXT_H)
#include <llvm/LLVMContext.h>
#endif

static void SetValue(llvm::Value *V, llvm::GenericValue Val, llvm::ExecutionContext &SF) {
  SF.Values[V] = Val;
}

TSOInterpreter::TSOInterpreter(llvm::Module *M, TSOTraceBuilder &TB,
                               const Configuration &conf)
  : Interpreter(M,TB,conf) {
  tso_threads.push_back(TSOThread());
}

TSOInterpreter::~TSOInterpreter(){
}

llvm::ExecutionEngine *TSOInterpreter::create(llvm::Module *M, TSOTraceBuilder &TB,
                                              const Configuration &conf,
                                              std::string *ErrorStr){
#ifdef LLVM_MODULE_MATERIALIZE_ALL_PERMANENTLY_ERRORCODE_BOOL
  if(std::error_code EC = M->materializeAllPermanently()){
    // We got an error, just return 0
    if(ErrorStr) *ErrorStr = EC.message();
    return 0;
  }
#elif defined LLVM_MODULE_MATERIALIZE_ALL_PERMANENTLY_BOOL_STRPTR
  if (M->MaterializeAllPermanently(ErrorStr)){
    // We got an error, just return 0
    return 0;
  }
#else
  if(std::error_code EC = M->materializeAll()){
    // We got an error, just return 0
    if(ErrorStr) *ErrorStr = EC.message();
    return 0;
  }
#endif

  return new TSOInterpreter(M,TB,conf);
}

void TSOInterpreter::runAux(int proc, int aux){
  /* Perform an update from store buffer to memory. */

  assert(aux == 0);
  assert(tso_threads[proc].store_buffer.size());

  const MBlock &blk = tso_threads[proc].store_buffer.front();

  TB.atomic_store(blk.get_ref());

  if(DryRun) return;

  if(!CheckedMemCpy((uint8_t*)blk.get_ref().ref,(uint8_t*)blk.get_block(),blk.get_ref().size)){
    return;
  };

  for(unsigned i = 0; i < tso_threads[proc].store_buffer.size()-1; ++i){
    tso_threads[proc].store_buffer[i] = tso_threads[proc].store_buffer[i+1];
  }
  tso_threads[proc].store_buffer.pop_back();

  if(int(tso_threads[proc].store_buffer.size()) <= tso_threads[proc].partial_buffer_flush){
    assert(0 <= tso_threads[proc].partial_buffer_flush);
    /* The real thread was waiting for the buffer to flush. Enable the
     * real thread. */
    tso_threads[proc].partial_buffer_flush = -1;
    TB.mark_available(proc);
  }

  if(tso_threads[proc].store_buffer.empty()){
    /* Disable update thread. */
    TB.mark_unavailable(proc,0);
    /* If the real thread has terminated, then wake up other threads
     * which are waiting to join with this one. */
    if(Threads[proc].ECStack.empty()){
      for(int p : Threads[proc].AwaitingJoin){
        TB.mark_available(p);
      }
    }
  }
}

int TSOInterpreter::newThread(const CPid &cpid){
  tso_threads.push_back(TSOThread());
  return Interpreter::newThread(cpid);
}

bool TSOInterpreter::isFence(llvm::Instruction &I){
  if(llvm::isa<llvm::CallInst>(I)){
    llvm::CallSite CS(static_cast<llvm::CallInst*>(&I));
    llvm::Function *F = CS.getCalledFunction();
    if(F && F->isDeclaration() &&
       F->getIntrinsicID() == llvm::Intrinsic::not_intrinsic &&
       conf.extfun_no_fence.count(F->getName().str()) == 0){
      return true;
    }
    if(F && F->getName().str().find("__VERIFIER_atomic_") == 0) return true;
    {
      std::string asmstr;
      if(isInlineAsm(CS,&asmstr) && asmstr == "mfence") return true;
    }
  }else if(llvm::isa<llvm::StoreInst>(I)){
    return static_cast<llvm::StoreInst&>(I).getOrdering() == LLVM_ATOMIC_ORDERING_SCOPE::SequentiallyConsistent;
  }else if(llvm::isa<llvm::FenceInst>(I)){
    return static_cast<llvm::FenceInst&>(I).getOrdering() == LLVM_ATOMIC_ORDERING_SCOPE::SequentiallyConsistent;
  }else if(llvm::isa<llvm::AtomicCmpXchgInst>(I)){
#ifdef LLVM_CMPXCHG_SEPARATE_SUCCESS_FAILURE_ORDERING
    llvm::AtomicOrdering succ = static_cast<llvm::AtomicCmpXchgInst&>(I).getSuccessOrdering();
    llvm::AtomicOrdering fail = static_cast<llvm::AtomicCmpXchgInst&>(I).getFailureOrdering();
    if(succ != LLVM_ATOMIC_ORDERING_SCOPE::SequentiallyConsistent || fail != LLVM_ATOMIC_ORDERING_SCOPE::SequentiallyConsistent){
#else
    if(static_cast<llvm::AtomicCmpXchgInst&>(I).getOrdering() != LLVM_ATOMIC_ORDERING_SCOPE::SequentiallyConsistent){
#endif
      Debug::warn("TSOInterpreter::isFence::cmpxchg") << "WARNING: Non-sequentially consistent CMPXCHG instruction interpreted as sequentially consistent.\n";
    }
    return true;
  }else if(llvm::isa<llvm::AtomicRMWInst>(I)){
    if(static_cast<llvm::AtomicRMWInst&>(I).getOrdering() != LLVM_ATOMIC_ORDERING_SCOPE::SequentiallyConsistent){
      Debug::warn("TSOInterpreter::isFence::rmw") << "WARNING: Non-sequentially consistent RMW instruction interpreted as sequentially consistent.\n";
    }
    return true;
  }
  return false;
}

void TSOInterpreter::terminate(llvm::Type *RetTy, llvm::GenericValue Result){
  if(CurrentThread != 0){
    assert(RetTy == llvm::Type::getInt8PtrTy(RetTy->getContext()));
    Threads[CurrentThread].RetVal = Result;
  }
  if(tso_threads[CurrentThread].store_buffer.empty()){
    for(int p : Threads[CurrentThread].AwaitingJoin){
      TB.mark_available(p);
    }
  }
}

bool TSOInterpreter::checkRefuse(llvm::Instruction &I){
  int tid;
  if(isPthreadJoin(I,&tid)){
    if(0 <= tid && tid < int(Threads.size()) && tid != CurrentThread){
      if(Threads[tid].ECStack.size() ||
         tso_threads[tid].store_buffer.size()){
        /* The awaited thread is still executing */
        TB.refuse_schedule();
        Threads[tid].AwaitingJoin.push_back(CurrentThread);
        return true;
      }
    }else{
      // Erroneous thread id
      // Allow execution (will produce an error trace)
    }
  }
  /* Refuse if I has fence semantics and the store buffer is
   * non-empty.
   */
  if(isFence(I) && !tso_threads[CurrentThread].store_buffer.empty()){
    tso_threads[CurrentThread].partial_buffer_flush = 0;
    TB.refuse_schedule();
    return true;
  }
  /* Refuse if I is a load and the latest entry in the store buffer
   * which overlaps with the memory location targeted by I does not
   * target precisely the same bytes as I.
   */
  if(llvm::isa<llvm::LoadInst>(I)){
    llvm::ExecutionContext &SF = ECStack()->back();
    llvm::GenericValue SRC = getOperandValue(static_cast<llvm::LoadInst&>(I).getPointerOperand(), SF);
    llvm::GenericValue *Ptr = (llvm::GenericValue*)GVTOP(SRC);
    MRef mr = GetMRef(Ptr,static_cast<llvm::LoadInst&>(I).getType());
    for(int i = int(tso_threads[CurrentThread].store_buffer.size())-1; 0 <= i; --i){
      if(mr.overlaps(tso_threads[CurrentThread].store_buffer[i].get_ref())){
        if(mr != tso_threads[CurrentThread].store_buffer[i].get_ref()){
          /* Block until this store buffer entry has disappeared from
           * the buffer.
           */
          tso_threads[CurrentThread].partial_buffer_flush =
            int(tso_threads[CurrentThread].store_buffer.size()) - i - 1;
          TB.refuse_schedule();
          return true;
        }
        break;
      }
    }
  }
  return Interpreter::checkRefuse(I);
}

void TSOInterpreter::visitLoadInst(llvm::LoadInst &I){
  llvm::ExecutionContext &SF = ECStack()->back();
  llvm::GenericValue SRC = getOperandValue(I.getPointerOperand(), SF);
  llvm::GenericValue *Ptr = (llvm::GenericValue*)GVTOP(SRC);
  llvm::GenericValue Result;

  TB.load(GetMRef(Ptr,I.getType()));

  if(DryRun && DryRunMem.size()){
    assert(tso_threads[CurrentThread].store_buffer.empty());
    DryRunLoadValueFromMemory(Result, Ptr, I.getType());
    SetValue(&I, Result, SF);
    return;
  }

  /* Check store buffer for ROWE opportunity. */
  for(int i = int(tso_threads[CurrentThread].store_buffer.size())-1; 0 <= i; --i){
    if(Ptr == tso_threads[CurrentThread].store_buffer[i].get_ref().ref){
      /* Read-Own-Write-Early */
      assert(GetMRef(Ptr,I.getType()).size == tso_threads[CurrentThread].store_buffer[i].get_ref().size);
      CheckedLoadValueFromMemory(Result,(llvm::GenericValue*)tso_threads[CurrentThread].store_buffer[i].get_block(),I.getType());
      SetValue(&I, Result, SF);
      return;
    }
  }

  /* Load from memory */
  if(!CheckedLoadValueFromMemory(Result, Ptr, I.getType())) return;
  SetValue(&I, Result, SF);
}

void TSOInterpreter::visitStoreInst(llvm::StoreInst &I){
  llvm::ExecutionContext &SF = ECStack()->back();
  llvm::GenericValue Val = getOperandValue(I.getOperand(0), SF);
  llvm::GenericValue *Ptr = (llvm::GenericValue *)GVTOP(getOperandValue(I.getPointerOperand(), SF));

  if(I.getOrdering() == LLVM_ATOMIC_ORDERING_SCOPE::SequentiallyConsistent ||
     0 <= AtomicFunctionCall){
    /* Atomic store */
    assert(tso_threads[CurrentThread].store_buffer.empty());
    TB.atomic_store(GetMRef(Ptr,I.getOperand(0)->getType()));
    if(DryRun){
      DryRunMem.push_back(GetMBlock(Ptr, I.getOperand(0)->getType(), Val));
      return;
    }
    CheckedStoreValueToMemory(Val, Ptr, I.getOperand(0)->getType());
  }else{
    /* Store to buffer */
    TB.store(GetMRef(Ptr,I.getOperand(0)->getType()));
    if(DryRun){
      DryRunMem.push_back(GetMBlock(Ptr, I.getOperand(0)->getType(), Val));
      return;
    }
    tso_threads[CurrentThread].store_buffer.push_back(GetMBlock(Ptr, I.getOperand(0)->getType(), Val));
  }
}

void TSOInterpreter::visitFenceInst(llvm::FenceInst &I){
  if(I.getOrdering() == LLVM_ATOMIC_ORDERING_SCOPE::SequentiallyConsistent){
    TB.fence();
  }
}

void TSOInterpreter::visitAtomicCmpXchgInst(llvm::AtomicCmpXchgInst &I){
  assert(tso_threads[CurrentThread].store_buffer.empty());
  Interpreter::visitAtomicCmpXchgInst(I);
}

void TSOInterpreter::visitAtomicRMWInst(llvm::AtomicRMWInst &I){
  assert(tso_threads[CurrentThread].store_buffer.empty());
  Interpreter::visitAtomicRMWInst(I);
}

void TSOInterpreter::visitInlineAsm(llvm::CallSite &CS, const std::string &asmstr){
  if(asmstr == "mfence"){
    TB.fence();
  }else if(asmstr == ""){ // Do nothing
  }else{
    throw std::logic_error("Unsupported inline assembly: "+asmstr);
  }
}
