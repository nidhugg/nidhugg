// For the parts of the code originating in LLVM:
//===-- Interpreter.h ------------------------------------------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This header file defines the interpreter structure
//
//===----------------------------------------------------------------------===//

/* Copyright (C) 2015-2017 Carl Leonardsson
 * (For the modifications made in this file to the code from LLVM.)
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

#include <config.h>

#ifndef __POWER_INTERPRETER_H__
#define __POWER_INTERPRETER_H__

#include "Configuration.h"
#include "CPid.h"
#include "POWERARMTraceBuilder.h"
#include "vecset.h"

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#if defined(HAVE_LLVM_SUPPORT_CALLSITE_H)
#include <llvm/Support/CallSite.h>
#elif defined(HAVE_LLVM_IR_CALLSITE_H)
#include <llvm/IR/CallSite.h>
#endif
#if defined(HAVE_LLVM_IR_DATALAYOUT_H)
#include <llvm/IR/DataLayout.h>
#elif defined(HAVE_LLVM_DATALAYOUT_H)
#include <llvm/DataLayout.h>
#endif
#if defined(HAVE_LLVM_IR_FUNCTION_H)
#include <llvm/IR/Function.h>
#elif defined(HAVE_LLVM_FUNCTION_H)
#include <llvm/Function.h>
#endif
#if defined(HAVE_LLVM_INSTVISITOR_H)
#include <llvm/InstVisitor.h>
#elif defined(HAVE_LLVM_IR_INSTVISITOR_H)
#include <llvm/IR/InstVisitor.h>
#elif defined(HAVE_LLVM_SUPPORT_INSTVISITOR_H)
#include <llvm/Support/InstVisitor.h>
#endif
#include <llvm/Support/DataTypes.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_ostream.h>

namespace llvm{
  class IntrinsicLowering;
  struct FunctionInfo;
  template<typename T> class generic_gep_type_iterator;
  class ConstantExpr;
}

// Interpreter - This class represents the entirety of the interpreter.
//
class POWERInterpreter : public llvm::ExecutionEngine, public llvm::InstVisitor<POWERInterpreter> {
public:
  typedef llvm::generic_gep_type_iterator<llvm::User::const_op_iterator> gep_type_iterator;


  // AllocaHolder - Object to track all of the blocks of memory allocated by
  // alloca.  When the function returns, this object is popped off the execution
  // stack, which causes the dtor to be run, which frees all the alloca'd memory.
  //
  class AllocaHolder {
    std::vector<void *> Allocations;

  public:
    AllocaHolder() {}

    // Make this type move-only. Define explicit move special members for MSVC.
    AllocaHolder(AllocaHolder &&RHS) : Allocations(std::move(RHS.Allocations)) {}
    AllocaHolder &operator=(AllocaHolder &&RHS) {
      Allocations = std::move(RHS.Allocations);
      return *this;
    }

    ~AllocaHolder() {
      for (void *Allocation : Allocations)
        free(Allocation);
    }

    void add(void *Mem) { Allocations.push_back(Mem); }
  };

  typedef std::vector<llvm::GenericValue> ValuePlaneTy;

  class FetchedInstruction {
  public:
    FetchedInstruction(llvm::Instruction &I)
      : I(I),
        Committed(false), EventIndex(0),
        IsBranch(false) {};
    llvm::Instruction &I;
    struct Operand{
      Operand() : Available(false), IsAddrOf(-1), IsDataOf(-1) {};
      Operand(const llvm::GenericValue &Value) : Value(Value), Available(true), IsAddrOf(-1), IsDataOf(-1) {};
      llvm::GenericValue Value;
      bool Available;
      int IsAddrOf; // This operand holds the address for the IsAddrOf:th access of this instruction. -1 if not an address.
      int IsDataOf; // This operand holds the data for the IsDataOf:th access of this instruction. -1 if not data for an access.
      bool isAddr() const { return 0 <= IsAddrOf; };
      bool isData() const { return 0 <= IsDataOf; };
    };
    std::vector<Operand> Operands;
    struct Dependent{
      /* FI is a fetched instruction which is dependent on another
       * fetched instruction for providing the value of its op_idx:th
       * operand.
       */
      std::shared_ptr<FetchedInstruction> FI;
      int op_idx;
      bool operator<(const Dependent &D) const {
        return FI < D.FI || (FI == D.FI && op_idx < D.op_idx);
      };
      bool operator==(const Dependent &D) const{
        return FI == D.FI && op_idx == D.op_idx;
      };
    };
    VecSet<Dependent> Dependents; // The instructions which depend on this instruction
    VecSet<int> AddrDeps;
    VecSet<int> DataDeps;
    llvm::GenericValue Value;
    bool Committed;
    int EventIndex; // 0 if not an event
    bool IsBranch; // This instruction should be interpreted as branching
    bool isEvent() const { return EventIndex; };
    bool committable() const {
      for(unsigned i = 0; i < Operands.size(); ++i){
        if(!Operands[i].Available) return false;
      }
      return true;
    };
  };

  // ExecutionContext struct - This struct represents one stack frame currently
  // executing.
  //
  struct ExecutionContext {
    llvm::Function              *CurFunction;// The currently executing function
    llvm::BasicBlock            *CurBB;      // The currently executing BB
    llvm::BasicBlock            *PrevBB;     // The previously executing BB
    llvm::BasicBlock::iterator  CurInst;    // The next instruction to execute
    std::shared_ptr<FetchedInstruction> Caller; // The function call to the currently executing function
    std::map<llvm::Value *, std::shared_ptr<FetchedInstruction> > Values; // LLVM values used in this invocation
    std::vector<llvm::GenericValue>  VarArgs; // Values passed through an ellipsis

    ExecutionContext() : CurFunction(nullptr), CurBB(nullptr), PrevBB(nullptr), CurInst(nullptr) {}

    ExecutionContext(ExecutionContext &&O) noexcept
      : CurFunction(O.CurFunction), CurBB(O.CurBB), PrevBB(O.PrevBB), CurInst(O.CurInst),
        Caller(O.Caller), Values(std::move(O.Values)),
        VarArgs(std::move(O.VarArgs)) {}

    ExecutionContext &operator=(ExecutionContext &&O) {
      CurFunction = O.CurFunction;
      CurBB = O.CurBB;
      PrevBB = O.PrevBB;
      CurInst = O.CurInst;
      Caller = O.Caller;
      Values = std::move(O.Values);
      VarArgs = std::move(O.VarArgs);
      return *this;
    }
  };
private:
  llvm::GenericValue ExitValue;          // The return value of the called function
  llvm::DataLayout TD;
  llvm::IntrinsicLowering *IL;

  const Configuration &conf;
  POWERARMTraceBuilder &TB;

  class Thread{
  public:
    Thread(const CPid &cpid)
      : cpid(cpid), status(new uint8_t()) {
      *status = 0;
    };
    Thread(Thread &&T) noexcept
      : cpid(T.cpid), ECStack(std::move(T.ECStack)),
        CommittableEvents(T.CommittableEvents),
        CreateEvent(T.CreateEvent),
        Allocas(std::move(T.Allocas)),
        status(T.status) {};
    CPid cpid;
    // The runtime stack of executing code.  The top of the stack is the current
    // function record.
    std::vector<ExecutionContext> ECStack;
    VecSet<std::shared_ptr<FetchedInstruction> > CommittableEvents;
    /* CreateEvent is the call to pthread_create which created this
     * thread. For the initial thread, CreateEvent is null.
     */
    IID<int> CreateEvent;
    AllocaHolder Allocas;            // Track memory allocated by alloca
    /* status points to a thread-unique memory location used by
     * pthread_create and pthread_join to communicate the current
     * status of the thread. Value interpretation:
     *
     * - 0: Not started
     * - 1: Started, running
     * - 2: Terminated
     */
    uint8_t *status;
  };
  std::vector<Thread> Threads;
  int CurrentThread;
  CPidSystem CPS;
  std::vector<std::shared_ptr<FetchedInstruction> > CommittableLocal;
  std::shared_ptr<FetchedInstruction> CurInstr;

  // AtExitHandlers - List of functions to call when the program exits,
  // registered with the atexit() library function.
  std::vector<llvm::Function*> AtExitHandlers;

  /* A dummy store of the value 0 (int32) to null. */
  llvm::Instruction *dummy_store;
  /* A dummy load from null (int8*). */
  llvm::Instruction *dummy_load8;

public:
  explicit POWERInterpreter(llvm::Module *M, POWERARMTraceBuilder &TB,
                            const Configuration &conf = Configuration::default_conf);
  ~POWERInterpreter();

  /// runAtExitHandlers - Run any functions registered by the program's calls to
  /// atexit(3), which we intercept and store in AtExitHandlers.
  ///
  void runAtExitHandlers();

  /// Create an interpreter ExecutionEngine.
  ///
  static llvm::ExecutionEngine *create(llvm::Module *M, POWERARMTraceBuilder &TB,
                                       const Configuration &conf = Configuration::default_conf,
                                       std::string *ErrorStr = nullptr);

  /// run - Start execution with the specified function and arguments.
  ///
#ifdef LLVM_EXECUTION_ENGINE_RUN_FUNCTION_VECTOR
  llvm::GenericValue runFunction(llvm::Function *F,
                                 const std::vector<llvm::GenericValue> &ArgValues);
#else
  llvm::GenericValue runFunction(llvm::Function *F,
                                 llvm::ArrayRef<llvm::GenericValue> ArgValues);
#endif

  void *getPointerToNamedFunction(const std::string &Name,
                                  bool AbortOnFailure = true) {
    // FIXME: not implemented.
    return 0;
  }

  void *getPointerToNamedFunction(llvm::StringRef Name,
                                  bool AbortOnFailure = true) {
    // FIXME: not implemented.
    return 0;
  }

  virtual void *getPointerToBasicBlock(llvm::BasicBlock *BB) { return (void*)BB; }
  virtual void *recompileAndRelinkFunction(llvm::Function *F) {
    return getPointerToFunction(F);
  }
  virtual void freeMachineCodeForFunction(llvm::Function *F) { }

  // Methods used to execute code:
  // Place a call on the stack
  void callFunction(llvm::Function *F, const std::vector<llvm::Value*> &ArgVals);
  void callFunction(llvm::Function *F, const std::vector<llvm::GenericValue> &ArgVals);
  void callFunction(llvm::Function *F, const std::vector<std::shared_ptr<FetchedInstruction> > &ArgVals);
  void run();                // Execute instructions until nothing left to do

  // Opcode Implementations
  void visitReturnInst(llvm::ReturnInst &I);
  void visitBranchInst(llvm::BranchInst &I);
  void visitSwitchInst(llvm::SwitchInst &I);
  void visitIndirectBrInst(llvm::IndirectBrInst &I);

  void visitBinaryOperator(llvm::BinaryOperator &I);
  void visitICmpInst(llvm::ICmpInst &I);
  void visitFCmpInst(llvm::FCmpInst &I);
  void visitAllocaInst(llvm::AllocaInst &I);
  void visitLoadInst(llvm::LoadInst &I);
  void visitStoreInst(llvm::StoreInst &I);
  void visitGetElementPtrInst(llvm::GetElementPtrInst &I);
  void visitPHINode(llvm::PHINode &PN) {
    llvm_unreachable("PHI nodes already handled!");
  }
  void visitTruncInst(llvm::TruncInst &I);
  void visitZExtInst(llvm::ZExtInst &I);
  void visitSExtInst(llvm::SExtInst &I);
  void visitFPTruncInst(llvm::FPTruncInst &I);
  void visitFPExtInst(llvm::FPExtInst &I);
  void visitUIToFPInst(llvm::UIToFPInst &I);
  void visitSIToFPInst(llvm::SIToFPInst &I);
  void visitFPToUIInst(llvm::FPToUIInst &I);
  void visitFPToSIInst(llvm::FPToSIInst &I);
  void visitPtrToIntInst(llvm::PtrToIntInst &I);
  void visitIntToPtrInst(llvm::IntToPtrInst &I);
  void visitBitCastInst(llvm::BitCastInst &I);
  void visitSelectInst(llvm::SelectInst &I);


  void visitCallSite(llvm::CallSite CS);
  void visitCallInst(llvm::CallInst &I) { visitCallSite (llvm::CallSite (&I)); }
  void visitInvokeInst(llvm::InvokeInst &I) { visitCallSite (llvm::CallSite (&I)); }
  void visitUnreachableInst(llvm::UnreachableInst &I);

  void visitShl(llvm::BinaryOperator &I);
  void visitLShr(llvm::BinaryOperator &I);
  void visitAShr(llvm::BinaryOperator &I);

  void visitVAArgInst(llvm::VAArgInst &I);
  void visitExtractElementInst(llvm::ExtractElementInst &I);
  void visitInsertElementInst(llvm::InsertElementInst &I);
  void visitShuffleVectorInst(llvm::ShuffleVectorInst &I);

  void visitExtractValueInst(llvm::ExtractValueInst &I);
  void visitInsertValueInst(llvm::InsertValueInst &I);
  void visitInlineAsm(llvm::CallSite &CS, const std::string &asmstr);

  void visitInstruction(llvm::Instruction &I) {
    llvm::errs() << I << "\n";
    llvm_unreachable("Instruction not interpretable yet!");
  }

  void exitCalled(llvm::GenericValue GV);

  void addAtExitHandler(llvm::Function *F) {
    AtExitHandlers.push_back(F);
  }

  llvm::GenericValue *getFirstVarArg () {
    return &(Threads[CurrentThread].ECStack.back ().VarArgs[0]);
  }

private:  // Helper functions
  llvm::GenericValue executeGEPOperation(llvm::Value *PtrVal,
                                         const std::vector<FetchedInstruction::Operand> &Operands,
                                         gep_type_iterator I,
                                         gep_type_iterator E);

  // SwitchToNewBasicBlock - Start execution in a new basic block and run any
  // PHI nodes in the top of the block.  This is used for intraprocedural
  // control flow.
  //
  void SwitchToNewBasicBlock(llvm::BasicBlock *Dest, ExecutionContext &SF);

  void *getPointerToFunction(llvm::Function *F) { return (void*)F; }

  void initializeExecutionEngine() { }
  void setCurInstrValue(const llvm::GenericValue &Val);
  llvm::GenericValue getConstantExprValue(llvm::ConstantExpr *CE);
  llvm::GenericValue &getOperandValue(int n){
    assert(0 <= n && n < int(CurInstr->Operands.size()));
    assert(CurInstr->Operands[n].Available);
    return CurInstr->Operands[n].Value;
  };
  llvm::GenericValue getConstantOperandValue(llvm::Value *V);
  llvm::GenericValue getConstantValue(llvm::Constant *V);
  llvm::GenericValue executeTruncInst(llvm::Value *SrcVal, const llvm::GenericValue &Src,
                                      llvm::Type *DstTy);
  llvm::GenericValue executeSExtInst(llvm::Value *SrcVal, const llvm::GenericValue &Src,
                                     llvm::Type *DstTy);
  llvm::GenericValue executeZExtInst(llvm::Value *SrcVal, const llvm::GenericValue &Src,
                                     llvm::Type *DstTy);
  llvm::GenericValue executeFPTruncInst(llvm::Value *SrcVal, const llvm::GenericValue &Src,
                                        llvm::Type *DstTy);
  llvm::GenericValue executeFPExtInst(llvm::Value *SrcVal, const llvm::GenericValue &Src,
                                      llvm::Type *DstTy);
  llvm::GenericValue executeFPToUIInst(llvm::Value *SrcVal, const llvm::GenericValue &Src,
                                       llvm::Type *DstTy);
  llvm::GenericValue executeFPToSIInst(llvm::Value *SrcVal, const llvm::GenericValue &Src,
                                       llvm::Type *DstTy);
  llvm::GenericValue executeUIToFPInst(llvm::Value *SrcVal, const llvm::GenericValue &Src,
                                       llvm::Type *DstTy);
  llvm::GenericValue executeSIToFPInst(llvm::Value *SrcVal, const llvm::GenericValue &Src,
                                       llvm::Type *DstTy);
  llvm::GenericValue executePtrToIntInst(llvm::Value *SrcVal, const llvm::GenericValue &Src,
                                         llvm::Type *DstTy);
  llvm::GenericValue executeIntToPtrInst(llvm::Value *SrcVal, const llvm::GenericValue &Src,
                                         llvm::Type *DstTy);
  llvm::GenericValue executeBitCastInst(llvm::Value *SrcVal, const llvm::GenericValue &Src,
                                        llvm::Type *DstTy);
  llvm::GenericValue executeCastOperation(llvm::Instruction::CastOps opcode, llvm::Value *SrcVal,
                                          llvm::Type *Ty, ExecutionContext &SF);
  void popStackAndReturnValueToCaller(llvm::Type *RetTy, llvm::GenericValue Result);

  /* Get the function called by CurInstr->I.
   *
   * Pre: CurInstr->I is a CallInst or InvokeInst.
   */
  llvm::Function *getCallee();
  /* Get the function called by I. Returns null if either I is not a
   * function call, or the callee is non-constant.
   */
  llvm::Function *getCallee(llvm::Instruction &I);
  /* Returns the index of the operand holding the callee of the
   * function call I.
   *
   * Pre: I is a CallInst or InvokeInst.
   */
  static int getCalleeOpIdx(const llvm::Instruction &I);
  /* Get the index of the operand holding the address of the memory
   * access I. Returns -1 if I is not a memory access.
   */
  static int getAddrOpIdx(const llvm::Instruction &I);

  /* Get an MRef for the pointer Ptr, with the size given by Ty for
   * the current data layout.
   */
  MRef GetMRef(void *Ptr, llvm::Type *Ty){
#ifdef LLVM_EXECUTIONENGINE_DATALAYOUT_PTR
    return {Ptr,int(getDataLayout()->getTypeStoreSize(Ty))};
#else
    return {Ptr,int(getDataLayout().getTypeStoreSize(Ty))};
#endif
  };
  ConstMRef GetConstMRef(void const *Ptr, llvm::Type *Ty){
#ifdef LLVM_EXECUTIONENGINE_DATALAYOUT_PTR
    return {Ptr,int(getDataLayout()->getTypeStoreSize(Ty))};
#else
    return {Ptr,int(getDataLayout().getTypeStoreSize(Ty))};
#endif
  };
  /* Get an MBlock associated with the location Ptr, and holding the
   * value Val of type Ty. The size of the memory location will be
   * that of Ty.
   */
  MBlock GetMBlock(void *Ptr, llvm::Type *Ty, const llvm::GenericValue &Val){
#ifdef LLVM_EXECUTIONENGINE_DATALAYOUT_PTR
    uint64_t alloc_size = getDataLayout()->getTypeAllocSize(Ty);
#else
    uint64_t alloc_size = getDataLayout().getTypeAllocSize(Ty);
#endif
    MBlock B(GetMRef(Ptr,Ty),alloc_size);
    StoreValueToMemory(Val,static_cast<llvm::GenericValue*>(B.get_block()),Ty);
    return B;
  };

  void commit();
  void commit(const std::shared_ptr<FetchedInstruction> &FI){
    CurInstr = FI;
    commit();
  };
  void fetchAll();
  std::shared_ptr<FetchedInstruction> fetch(llvm::Instruction &I);
  /* When a thread terminates, it needs to execute a few extra
   * instructions in order to synchronize with any thread trying to
   * join with this thread. This function fetches those extra
   * instructions.
   */
  void fetchThreadExit();
  /* Commit all fetched, uncommitted, local instructions which do not
   * depend (transitively) on any pending event.
   *
   * Returns true iff a Terminator or internal function call was
   * committed.
   */
  bool commitAllLocal();
  void commitLocalAndFetchAll();
  void registerOperand(int proc, FetchedInstruction &FI, int idx);
  /* Returns true iff CS is a call to inline assembly.
   *
   * If CS is a call to inline assembly, then *asmstr is assigned the
   * assembly string.
   */
  bool isInlineAsm(llvm::CallSite &CS, std::string *asmstr);
  bool isInlineAsm(llvm::Instruction &CS, std::string *asmstr);

  /* Force termination from all running threads, thereby terminating
   * the execution.
   */
  void abort();

  /* === External Functions with Special Support ===
   *
   * When executing an external function call to an external function
   * with special support (such as e.g. pthread_create), one of these
   * methods will be called instead of calling the actual function.
   *
   * F should be the called function.
   */
  void callAssertFail(llvm::Function *F);
  void callAssume(llvm::Function *F);
  void callMalloc(llvm::Function *F);
  void callPthreadCreate(llvm::Function *F);
  void callPthreadExit(llvm::Function *F);
  void callPthreadJoin(llvm::Function *F);
  void callPutchar(llvm::Function *F);
};

#endif
