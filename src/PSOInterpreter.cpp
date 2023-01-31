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
#include "PSOInterpreter.h"

#if defined(HAVE_LLVM_IR_LLVMCONTEXT_H)
#include <llvm/IR/LLVMContext.h>
#elif defined(HAVE_LLVM_LLVMCONTEXT_H)
#include <llvm/LLVMContext.h>
#endif

#include <utility>

static void SetValue(llvm::Value *V, llvm::GenericValue Val, llvm::ExecutionContext &SF) {
  SF.Values[V] = Val;
}

PSOInterpreter::PSOInterpreter(llvm::Module *M, PSOTraceBuilder &TB,
                               const Configuration &conf)
  : Interpreter(M,TB,conf) {
  pso_threads.push_back(PSOThread());
}

PSOInterpreter::~PSOInterpreter(){
}

bool PSOInterpreter::PSOThread::readable(const SymAddrSize &ml) const {
  for(SymAddr b : ml){
    auto it = store_buffers.find(b);
    if(it != store_buffers.end()){
      assert(it->second.size());
      if(it->second.back().ml != ml) return false;
    }
  }
  return true;
}

std::unique_ptr<PSOInterpreter> PSOInterpreter::
create(llvm::Module *M, PSOTraceBuilder &TB, const Configuration &conf,
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
#elif defined LLVM_MODULE_MATERIALIZE_LLVM_ALL_ERROR
  if (llvm::Error Err = M->materializeAll()) {
    std::string Msg;
    handleAllErrors(std::move(Err), [&](llvm::ErrorInfoBase &EIB) {
      Msg = EIB.message();
    });
    if (ErrorStr)
      *ErrorStr = Msg;
    // We got an error, just return 0
    return nullptr;
  }
#else
  if(std::error_code EC = M->materializeAll()){
    // We got an error, just return 0
    if(ErrorStr) *ErrorStr = EC.message();
    return 0;
  }
#endif

  return std::unique_ptr<PSOInterpreter>(new PSOInterpreter(M,TB,conf));
}

void PSOInterpreter::runAux(int proc, int aux){
  /* Perform an update from store buffer to memory. */

  assert(0 <= aux && aux < int(pso_threads[proc].aux_to_byte.size()));

  SymAddr b0 = pso_threads[proc].aux_to_byte[aux];

  assert(pso_threads[proc].store_buffers.count(b0));
  assert(pso_threads[proc].store_buffers[b0].size());

  SymAddrSize ml = pso_threads[proc].store_buffers[b0].front().ml;
  assert(ml.addr == b0);

  SymData sd(ml, ml.size);
  uint8_t *blk = (uint8_t*)sd.get_block();

  for(SymAddr b : ml){
    assert(pso_threads[proc].store_buffers.count(b));
    std::vector<PendingStoreByte> &sb = pso_threads[proc].store_buffers[b];
    assert(sb.size());
    assert(sb.front().ml == ml);
    blk[unsigned(b-b0)] = sb.front().val;
    if(!DryRun) {
      sb.erase(sb.begin());
      if(sb.empty()) pso_threads[proc].store_buffers.erase(b);
    }
  }

  if(!TB.atomic_store(sd)){
    abort();
    return;
  }
  if (DryRun) return;

  std::memcpy(pso_threads[proc].aux_to_addr[aux],blk,ml.size);

  /* Should we re-enable the thread after awaiting buffer flush? */
  switch(pso_threads[proc].awaiting_buffer_flush){
  case PSOThread::BFL_NO: // Do nothing
    break;
  case PSOThread::BFL_FULL:
    if(pso_threads[proc].all_buffers_empty()){
      /* Enable */
      pso_threads[proc].awaiting_buffer_flush = PSOThread::BFL_NO;
      TB.mark_available(proc);
    }
    break;
  case PSOThread::BFL_PARTIAL:
    if(ml.overlaps(pso_threads[proc].buffer_flush_ml) &&
       pso_threads[proc].readable(pso_threads[proc].buffer_flush_ml)){
      /* Enable */
      pso_threads[proc].awaiting_buffer_flush = PSOThread::BFL_NO;
      TB.mark_available(proc);
    }
    break;
  }

  if(pso_threads[proc].all_buffers_empty()){
    /* If the real thread has terminated, then wake up other threads
     * which are waiting to join with this one. */
    if(Threads[proc].ECStack.empty()){
      for(int p : Threads[proc].AwaitingJoin){
        TB.mark_available(p);
      }
    }
  }
}

int PSOInterpreter::newThread(const CPid &cpid){
  pso_threads.push_back(PSOThread());
  return Interpreter::newThread(cpid);
}

bool PSOInterpreter::isFence(llvm::Instruction &I){
  if(llvm::isa<llvm::CallInst>(I)){
    AnyCallInst CS(static_cast<llvm::CallInst*>(&I));
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
      Debug::warn("PSOInterpreter::isFence::cmpxchg") << "WARNING: Non-sequentially consistent CMPXCHG instruction interpreted as sequentially consistent.\n";
    }
    return true;
  }else if(llvm::isa<llvm::AtomicRMWInst>(I)){
    if(static_cast<llvm::AtomicRMWInst&>(I).getOrdering() != LLVM_ATOMIC_ORDERING_SCOPE::SequentiallyConsistent){
      Debug::warn("PSOInterpreter::isFence::rmw") << "WARNING: Non-sequentially consistent RMW instruction interpreted as sequentially consistent.\n";
    }
    return true;
  }
  return false;
}

void PSOInterpreter::terminate(llvm::Type *RetTy, llvm::GenericValue Result){
  if(CurrentThread != 0){
    assert(RetTy == llvm::Type::getInt8PtrTy(RetTy->getContext()));
    Threads[CurrentThread].RetVal = Result;
  }
  if(pso_threads[CurrentThread].all_buffers_empty()){
    for(int p : Threads[CurrentThread].AwaitingJoin){
      TB.mark_available(p);
    }
  }
}

bool PSOInterpreter::checkRefuse(llvm::Instruction &I){
  int tid;
  if(isPthreadJoin(I,&tid)){
    if(0 <= tid && tid < int(Threads.size()) && tid != CurrentThread){
      if(Threads[tid].ECStack.size() ||
         !pso_threads[tid].all_buffers_empty()){
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
  if(isFence(I) && !pso_threads[CurrentThread].all_buffers_empty()){
    pso_threads[CurrentThread].awaiting_buffer_flush = PSOThread::BFL_FULL;
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
    Option<SymAddrSize> mr = TryGetSymAddrSize(Ptr,static_cast<llvm::LoadInst&>(I).getType());
    if (!mr) return false; /* Let it execute and segfault */
    if(!pso_threads[CurrentThread].readable(*mr)){
      /* Block until this store buffer entry has disappeared from
       * the buffer.
       */
      pso_threads[CurrentThread].awaiting_buffer_flush = PSOThread::BFL_PARTIAL;
      pso_threads[CurrentThread].buffer_flush_ml = *mr;
      TB.refuse_schedule();
      return true;
    }
  }
  return Interpreter::checkRefuse(I);
}

void PSOInterpreter::visitLoadInst(llvm::LoadInst &I){
  llvm::ExecutionContext &SF = ECStack()->back();
  llvm::GenericValue SRC = getOperandValue(I.getPointerOperand(), SF);
  llvm::GenericValue *Ptr = (llvm::GenericValue*)GVTOP(SRC);
  llvm::GenericValue Result;

  Option<SymAddrSize> ml = GetSymAddrSize(Ptr,I.getType());
  if (!ml) return;
  if(!TB.load(*ml)){
    abort();
    return;
  }

  if(DryRun && DryRunMem.size()){
    assert(pso_threads[CurrentThread].all_buffers_empty());
    DryRunLoadValueFromMemory(Result, Ptr, *ml, I.getType());
    SetValue(&I, Result, SF);
    return;
  }

  /* Check store buffer for ROWE opportunity. */
  if(pso_threads[CurrentThread].store_buffers.count(ml->addr)){
    uint8_t *blk = new uint8_t[ml->size];
    for(SymAddr b : *ml){
      assert(pso_threads[CurrentThread].store_buffers[b].back().ml == *ml);
      blk[b - ml->addr] = pso_threads[CurrentThread].store_buffers[b].back().val;
    }
    LoadValueFromMemory(Result,(llvm::GenericValue*)blk,I.getType());
    SetValue(&I, Result, SF);
    delete[] blk;
    return;
  }

  /* Load from memory */
  LoadValueFromMemory(Result, Ptr, I.getType());
  SetValue(&I, Result, SF);
}

void PSOInterpreter::visitStoreInst(llvm::StoreInst &I){
  llvm::ExecutionContext &SF = ECStack()->back();
  llvm::GenericValue Val = getOperandValue(I.getOperand(0), SF);
  llvm::GenericValue *Ptr = (llvm::GenericValue *)GVTOP
    (getOperandValue(I.getPointerOperand(), SF));

  Option<SymData> mb = GetSymData(Ptr, I.getOperand(0)->getType(), Val);
  if (!mb) return;

  PSOThread &thr = pso_threads[CurrentThread];

  if(I.getOrdering() == LLVM_ATOMIC_ORDERING_SCOPE::SequentiallyConsistent ||
     0 <= AtomicFunctionCall){
    /* Atomic store */
    assert(thr.all_buffers_empty());
    if(!TB.atomic_store(*mb)){
      abort();
      return;
    }
    if(DryRun){
      DryRunMem.push_back(*mb);
      return;
    }
    StoreValueToMemory(Val, Ptr, I.getOperand(0)->getType());
  }else{
    /* Store to buffer */
    const SymAddrSize &ml = mb->get_ref();
    if(thr.byte_to_aux.count(ml.addr) == 0){
      thr.byte_to_aux[ml.addr] = int(thr.aux_to_byte.size());
      thr.aux_to_byte.push_back(ml.addr);
      thr.aux_to_addr.push_back((uint8_t*)Ptr);
    }

    if(!TB.store(*mb)){
      abort();
      return;
    }
    if(DryRun){
      DryRunMem.push_back(*mb);
      return;
    }
    for(SymAddr b : ml){
      unsigned i = b - ml.addr;
      thr.store_buffers[b].push_back(PendingStoreByte(ml,((uint8_t*)mb->get_block())[i]));
    }
  }
}

void PSOInterpreter::visitFenceInst(llvm::FenceInst &I){
  if(I.getOrdering() == LLVM_ATOMIC_ORDERING_SCOPE::SequentiallyConsistent){
    if(!TB.fence()){
      abort();
      return;
    }
  }
}

void PSOInterpreter::visitAtomicCmpXchgInst(llvm::AtomicCmpXchgInst &I){
  assert(pso_threads[CurrentThread].all_buffers_empty());
  Interpreter::visitAtomicCmpXchgInst(I);
}

void PSOInterpreter::visitAtomicRMWInst(llvm::AtomicRMWInst &I){
  assert(pso_threads[CurrentThread].all_buffers_empty());
  Interpreter::visitAtomicRMWInst(I);
}

void PSOInterpreter::visitInlineAsm(llvm::CallInst &CI, const std::string &asmstr){
  if(asmstr == "mfence"){
    if(!TB.fence()){
      abort();
      return;
    }
  }else if(asmstr == ""){ // Do nothing
  }else{
    throw std::logic_error("Unsupported inline assembly: "+asmstr);
  }
}
