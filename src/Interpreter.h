// For the parts of the code originating in LLVM:
//===-- Interpreter.h ------------------------------------------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LLVMLICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This header file defines the interpreter structure
//
//===----------------------------------------------------------------------===//

/* Copyright (C) 2014-2017 Carl Leonardsson
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

#ifndef __INTERPRETER_H__
#define __INTERPRETER_H__

#include "Configuration.h"
#include "CPid.h"
#include "SymAddr.h"
#include "VClock.h"
#include "Option.h"
#include "TSOPSOTraceBuilder.h"
#include "DPORInterpreter.h"

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
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
#include "AnyCallInst.h"
#include <llvm/Support/DataTypes.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_ostream.h>

#include <map>
#include <memory>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/container/flat_map.hpp>

namespace llvm {

class IntrinsicLowering;
template<typename T> class generic_gep_type_iterator;
class ConstantExpr;
typedef generic_gep_type_iterator<User::const_op_iterator> gep_type_iterator;

typedef std::vector<GenericValue> ValuePlaneTy;

// ExecutionContext struct - This struct represents one stack frame currently
// executing.
//
struct ExecutionContext {
  Function             *CurFunction;// The currently executing function
  BasicBlock           *CurBB;      // The currently executing BB
  BasicBlock::iterator  CurInst;    // The next instruction to execute
  std::map<Value *, GenericValue> Values; // LLVM values used in this invocation
  std::vector<GenericValue>  VarArgs; // Values passed through an ellipsis
  AnyCallInst           Caller;     // Holds the call that called subframes.
                                    // NULL if main func or debugger invoked fn
};

// Interpreter - This class represents the entirety of the interpreter.
//
class Interpreter : public DPORInterpreter, public InstVisitor<Interpreter> {
protected:
  GenericValue ExitValue;          // The return value of the called function
  DataLayout TD;
  IntrinsicLowering *IL;

  /* The TraceBuilder which will provide scheduling for this
   * Interpreter, and which will be notified of any actions that may
   * affect the chronological trace of this execution.
   */
  TSOPSOTraceBuilder &TB;
  const Configuration &conf;

  /* A Thread object keeps track of each running thread. */
  class Thread{
  public:
    Thread() : AssumeBlocked(false), RandEng(42), pending_mutex_lock(0), pending_condvar_awake(0) {}
    /* The complex thread identifier of this thread. */
    CPid cpid;
    /* The runtime stack of executing code. The top of the stack is the
     * current function record.
     */
    std::vector<ExecutionContext> ECStack;
    /* Whether the thread execution has been blocked due to failing an
     * assume-statement
     */
    bool AssumeBlocked;
    /* Contains the IDs (index into Threads) of all other threads
     * which are awaiting the termination of this thread to perform a
     * join.
     */
    std::vector<int> AwaitingJoin;
    /* If this thread has terminated, then RetVal is its return
     * value. Otherwise RetVal is undefined.
     */
    GenericValue RetVal;
    /* A random number generator used to generate a per-thread
     * deterministic sequence of pseudo-random numbers. The sequence
     * will be the same (per thread) in every execution.
     */
    std::minstd_rand RandEng;
    /* If this thread was suspended by calling pthread_cond_wait(cnd, lck),
     * then pendinc_condvar_awake == cnd and pending_mutex_lock == lck.
     * The next instruction will then do a pthread_cond_awake(cnd, lck)
     * (which reacquires lck) in addition to its normal semantics.
     *
     * pending_mutex_lock == 0 and pending_condvar_awake == 0 otherwise.
     */
    void *pending_mutex_lock;
    void *pending_condvar_awake;
    /* Thread local global values are stored here. */
    std::map<GlobalValue*,GenericValue> ThreadLocalValues;
  };
  /* All threads that are or have been running during this execution
   * have an entry in Threads, in the order in which they were
   * created.
   */
  std::vector<Thread> Threads;
  /* The index into Threads of the currently running thread. */
  int CurrentThread;
  /* The CPid System for all threads in this execution. */
  CPidSystem CPS;

  /* For events which may execute in several nondeterministic ways,
   * CurrentAlt determines which alternative should be executed. A
   * value of 0 indicates the default alternative (the only
   * alternative for deterministic events).
   *
   * CurrentAlt is assigned by the scheduling by TB.
   */
  int CurrentAlt;
  /* True if we are currently executing in dry run mode. (See
   * documentation for TraceBuilder.)
   *
   * DryRun is assigned by the scheduling by TB.
   */
  bool DryRun;
  /* True if execution has stopped due to blocking */
  bool Blocked = false;
  /* Keeps temporary changes to memory which are performed during dry
   * runs. Instead of changing the actual memory, during dry runs all
   * memory stores collected in DryRunMem. This allows memory loads
   * occurring later during the same dry run (e.g. a dry run of an
   * atomic function call) to check for updates that would have been
   * visible if the event were executing for real.
   *
   * DryRunMem holds SymDatas corresponding to every store that has
   * been performed during this dry run, in order from older to newer.
   */
  std::vector<SymData> DryRunMem;
  /* AtomicFunctionCall usually holds a negative value. However, while
   * we are executing inside an atomic function (one with a name
   * starting with "__VERIFIER_atomic_"), AtomicFunctionCall will hold
   * the index in ECStack of the stack frame of the outermost,
   * currently executing, atomic function.
   */
  int AtomicFunctionCall;

  /* A PthreadMutex object keeps information about a pthread mutex
   * object.
   */
  class PthreadMutex{
  public:
    PthreadMutex() : owner(-1) {}
    /* The ID of the thread which currently holds the mutex object. If
     * no thread holds the mutex object, then owner is -1.
     */
    int owner;
    /* The set of IDs of threads which are currently waiting (in
     * pthread_mutex_lock) to acquire the mutex object.
     */
    VecSet<int> waiting;
    bool isLocked() const { return 0 <= owner; }
    bool isUnlocked() const { return owner < 0; }
    void lock(int Proc) { assert(owner < 0); owner = Proc; }
    void unlock() { owner = -1; }
  };
  /* The pthread mutex objects which have been initialized, and not
   * destroyed in this execution.
   *
   * The key is the location of the object.
   */
  std::map<void*,PthreadMutex> PthreadMutexes;

  /* The threads that are blocking in an await statement */
  boost::container::flat_map
  <SymAddrSize, boost::container::flat_map<int, AwaitCond>> blocking_awaits;

  /* The runtime stack of executing code. The top of the stack is the
   * current function record.
   *
   * This is the stack of the currently executing thread.
   */
  std::vector<ExecutionContext> *ECStack() { return &Threads[CurrentThread].ECStack; }

  struct SymMBlockSize {
    SymMBlockSize(SymMBlock block, uint32_t size) :
      block(std::move(block)), size(size) {}
    SymMBlock block;
    uint32_t size;
  };

  /* Memory that has been allocated, and that should be freed at the
   * end of the execution.
   *
   * Locations in AllocatedMemHeap are allocated by the analyzed
   * program using malloc. Those in AllocatedMemStack are on the stack
   * of the allocated program.
   */
  std::set<void*> AllocatedMemHeap;
  std::set<void*> AllocatedMemStack;

  std::map<void*,SymMBlockSize> AllocatedMem;
  VClock<int> HeapAllocCount, StackAllocCount;
  /* Memory that has been explicitly freed by a call to free.
   *
   * The key is the freed memory. The value is the iid of the event
   * where free was called.
   *
   * Notice that the call to free has been intercepted, and the memory
   * is not actually freed until destruction of this Interpreter.
   */
  std::map<void*,IID<CPid> > FreedMem;

  // AtExitHandlers - List of functions to call when the program exits,
  // registered with the atexit() library function.
  std::vector<Function*> AtExitHandlers;

public:
  explicit Interpreter(Module *M, TSOPSOTraceBuilder &TB,
                       const Configuration &conf = Configuration::default_conf);
  virtual ~Interpreter();

  /// create - Create an interpreter ExecutionEngine. This can never fail.
  ///
  static std::unique_ptr<Interpreter>
  create(Module *M, TSOPSOTraceBuilder &TB,
         const Configuration &conf = Configuration::default_conf,
         std::string *ErrorStr = 0);

  /// run - Start execution with the specified function and arguments.
  ///
#ifdef LLVM_EXECUTION_ENGINE_RUN_FUNCTION_VECTOR
  virtual GenericValue runFunction(Function *F,
                                   const std::vector<GenericValue> &ArgValues);
#else
  virtual GenericValue runFunction(Function *F,
                                   llvm::ArrayRef<GenericValue> ArgValues);
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

  /* Returns true iff this trace contains any happens-before cycle.
   *
   * If there is a happens-before cycle, and conf.check_robustness is
   * set, then a corresponding error is added to errors.
   *
   * Call this method only at the end of execution.
   */
  virtual bool checkForCycles() const { return TB.check_for_cycles(); }

  /// runAtExitHandlers - Run any functions registered by the program's calls to
  /// atexit(3), which we intercept and store in AtExitHandlers.
  ///
  virtual void runAtExitHandlers();

  /// recompileAndRelinkFunction - For the interpreter, functions are always
  /// up-to-date.
  ///
  virtual void *recompileAndRelinkFunction(Function *F) {
    return getPointerToFunction(F);
  }

  /// freeMachineCodeForFunction - The interpreter does not generate any code.
  ///
  virtual void freeMachineCodeForFunction(Function *F) { }

  // Methods used to execute code:
  // Place a call on the stack
  virtual void callFunction(Function *F, const std::vector<GenericValue> &ArgVals);
  virtual void run();                // Execute instructions until nothing left to do

  // Opcode Implementations
  virtual void visitReturnInst(ReturnInst &I);
  virtual void visitBranchInst(BranchInst &I);
  virtual void visitSwitchInst(SwitchInst &I);
  virtual void visitIndirectBrInst(IndirectBrInst &I);

  virtual void visitBinaryOperator(BinaryOperator &I);
  virtual void visitICmpInst(ICmpInst &I);
  virtual void visitFCmpInst(FCmpInst &I);
  virtual void visitAllocaInst(AllocaInst &I);
  virtual void visitLoadInst(LoadInst &I);
  virtual void visitStoreInst(StoreInst &I);
  virtual void visitGetElementPtrInst(GetElementPtrInst &I);
  virtual void visitPHINode(PHINode &PN) {
    llvm_unreachable("PHI nodes already handled!");
  }
  virtual void visitTruncInst(TruncInst &I);
  virtual void visitZExtInst(ZExtInst &I);
  virtual void visitSExtInst(SExtInst &I);
  virtual void visitFPTruncInst(FPTruncInst &I);
  virtual void visitFPExtInst(FPExtInst &I);
  virtual void visitUIToFPInst(UIToFPInst &I);
  virtual void visitSIToFPInst(SIToFPInst &I);
  virtual void visitFPToUIInst(FPToUIInst &I);
  virtual void visitFPToSIInst(FPToSIInst &I);
  virtual void visitPtrToIntInst(PtrToIntInst &I);
  virtual void visitIntToPtrInst(IntToPtrInst &I);
  virtual void visitBitCastInst(BitCastInst &I);
  virtual void visitSelectInst(SelectInst &I);

  virtual void visitAnyCallInst(AnyCallInst CI);
#ifdef LLVM_HAS_CALLBASE
  virtual void visitCallBase(CallBase &CB) { visitAnyCallInst(&CB); }
#else
  virtual void visitCallSite(CallSite CS) { visitAnyCallInst(CS); }
#endif
  virtual void visitUnreachableInst(UnreachableInst &I);

  virtual void visitShl(BinaryOperator &I);
  virtual void visitLShr(BinaryOperator &I);
  virtual void visitAShr(BinaryOperator &I);

  virtual void visitVAArgInst(VAArgInst &I);
  virtual void visitExtractElementInst(ExtractElementInst &I);
  virtual void visitInsertElementInst(InsertElementInst &I);
  virtual void visitShuffleVectorInst(ShuffleVectorInst &I);

  virtual void visitExtractValueInst(ExtractValueInst &I);
  virtual void visitInsertValueInst(InsertValueInst &I);

  virtual void visitFenceInst(FenceInst &I) { /* Do nothing */ }
  virtual void visitAtomicCmpXchgInst(AtomicCmpXchgInst &I);
  virtual void visitAtomicRMWInst(AtomicRMWInst &I);
  virtual void visitInlineAsm(llvm::CallInst &CI, const std::string &asmstr);

  virtual void visitInstruction(Instruction &I) {
    errs() << I << "\n";
    throw std::logic_error("Interpreter: Encountered unsupported instruction.");
  }

  virtual GenericValue callExternalFunction(Function *F,
                                            const std::vector<GenericValue> &ArgVals);
  virtual void exitCalled(GenericValue GV);

  virtual void addAtExitHandler(Function *F) {
    assert(F && "Caller must validate F");
    AtExitHandlers.push_back(F);
  }

  virtual GenericValue *getFirstVarArg () {
    return &(ECStack()->back().VarArgs[0]);
  }

protected:  // Helper functions
  virtual GenericValue executeGEPOperation(Value *Ptr, gep_type_iterator I,
                                           gep_type_iterator E, ExecutionContext &SF);

  // SwitchToNewBasicBlock - Start execution in a new basic block and run any
  // PHI nodes in the top of the block.  This is used for intraprocedural
  // control flow.
  //
  virtual void SwitchToNewBasicBlock(BasicBlock *Dest, ExecutionContext &SF);

  virtual void *getPointerToFunction(Function *F) { return (void*)F; }
  virtual void *getPointerToBasicBlock(BasicBlock *BB) { return (void*)BB; }

  virtual void initializeExecutionEngine() { }
  virtual void initializeExternalFunctions();
  virtual GenericValue getConstantExprValue(ConstantExpr *CE, ExecutionContext &SF);
  virtual GenericValue getOperandValue(Value *V, ExecutionContext &SF);
  virtual GenericValue executeTruncInst(Value *SrcVal, Type *DstTy,
                                        ExecutionContext &SF);
  virtual GenericValue executeSExtInst(Value *SrcVal, Type *DstTy,
                                       ExecutionContext &SF);
  virtual GenericValue executeZExtInst(Value *SrcVal, Type *DstTy,
                                       ExecutionContext &SF);
  virtual GenericValue executeFPTruncInst(Value *SrcVal, Type *DstTy,
                                          ExecutionContext &SF);
  virtual GenericValue executeFPExtInst(Value *SrcVal, Type *DstTy,
                                        ExecutionContext &SF);
  virtual GenericValue executeFPToUIInst(Value *SrcVal, Type *DstTy,
                                         ExecutionContext &SF);
  virtual GenericValue executeFPToSIInst(Value *SrcVal, Type *DstTy,
                                         ExecutionContext &SF);
  virtual GenericValue executeUIToFPInst(Value *SrcVal, Type *DstTy,
                                         ExecutionContext &SF);
  virtual GenericValue executeSIToFPInst(Value *SrcVal, Type *DstTy,
                                         ExecutionContext &SF);
  virtual GenericValue executePtrToIntInst(Value *SrcVal, Type *DstTy,
                                           ExecutionContext &SF);
  virtual GenericValue executeIntToPtrInst(Value *SrcVal, Type *DstTy,
                                           ExecutionContext &SF);
  virtual GenericValue executeBitCastInst(Value *SrcVal, Type *DstTy,
                                          ExecutionContext &SF);
  virtual GenericValue executeCastOperation(Instruction::CastOps opcode, Value *SrcVal,
                                            Type *Ty, ExecutionContext &SF);
  static GenericValue executeICMP_EQ(GenericValue Src1, GenericValue Src2, Type *Ty);
  static GenericValue executeCmpInst(unsigned predicate, GenericValue Src1,
                                     GenericValue Src2, Type *Ty);
  virtual void popStackAndReturnValueToCaller(Type *RetTy, GenericValue Result);
  virtual void returnValueToCaller(Type *RetTy, GenericValue Result);

  /* This method will be called when the TraceBuilder has scheduled an
   * auxiliary thread (proc,aux). The method should be overridden by
   * derived classes implementing interpreters for relaxed memory.
   */
  virtual void runAux(int proc, int aux){
    llvm_unreachable("Interpreter::runAux: No auxiliary threads defined in basic Interpreter.");
  }

  /* Creates a new thread with an empty stack and CPid cpid, and
   * returns its ID (index into Threads).
   */
  virtual int newThread(const CPid &cpid){
    Threads.push_back(Thread());
    Threads.back().cpid = cpid;
    return int(Threads.size())-1;
  }

  /* Set the return value from the current thread to Result of type
   * retTy.
   */
  virtual void terminate(Type *retTy, GenericValue Result);

  /* Force termination from all running threads, thereby terminating
   * the execution.
   */
  virtual void abort();
  /* Empty all stacks. */
  virtual void clearAllStacks();

  /* Get a SymAddr for the pointer Ptr, returning false if the address
   * is unknown.
   * GetSymAddr() additionally reports the error and calls abort()
   * before returning nullptr.
   */
  Option<SymAddr> TryGetSymAddr(void *Ptr);
  Option<SymAddr> GetSymAddr(void *Ptr);
  /* Get a SymAddrSize for the pointer Ptr, with the size given by Ty
   * for the current data layout.
   * GetSymAddrSize() reports the error and calls abort() before returning
   * nullptr.
   */
  Option<SymAddrSize> GetSymAddrSize(void *Ptr, Type *Ty) {
    if (Option<SymAddr> addr = GetSymAddr(Ptr)) {
#ifdef LLVM_EXECUTIONENGINE_DATALAYOUT_PTR
      return {{*addr,getDataLayout()->getTypeStoreSize(Ty)}};
#else
      return {{*addr,getDataLayout().getTypeStoreSize(Ty)}};
#endif
    } else {
      return nullptr;
    }
  }

  Option<SymAddrSize> TryGetSymAddrSize(void *Ptr, Type *Ty){
    if (Option<SymAddr> addr = TryGetSymAddr(Ptr)) {
#ifdef LLVM_EXECUTIONENGINE_DATALAYOUT_PTR
      return {{*addr,getDataLayout()->getTypeStoreSize(Ty)}};
#else
      return {{*addr,getDataLayout().getTypeStoreSize(Ty)}};
#endif
    } else {
      return nullptr;
    }
  }
  /* Get a SymData associated with the location Ptr, and holding the
   * value Val of type Ty. The size of the memory location will be
   * that of Ty.
   */
  Option<SymData> GetSymData(void *Ptr, Type *Ty, const GenericValue &Val){
    Option<SymAddrSize> Ptr_sas = GetSymAddrSize(Ptr,Ty);
    if (!Ptr_sas) return nullptr;
    return GetSymData(*Ptr_sas, Ty, Val);
  }
  /* Get a SymData associated with the location Ptr, and holding the
   * value Val of type Ty. The size of the memory location will be
   * that of Ty.
   */
  SymData GetSymData(SymAddrSize Ptr, Type *Ty, const GenericValue &Val){
#ifdef LLVM_EXECUTIONENGINE_DATALAYOUT_PTR
    uint64_t alloc_size = getDataLayout()->getTypeAllocSize(Ty);
#else
    uint64_t alloc_size = getDataLayout().getTypeAllocSize(Ty);
#endif
    SymData B(Ptr,alloc_size);
    StoreValueToMemory(Val,static_cast<GenericValue*>(B.get_block()),Ty);
    return B;
  }

  /* Checks whether F refers to a valid function, returns true if so, or
   * false if not. If invalid also reports the error in TB and calls
   * abort().
   */
  bool ValidateFunctionPointer(Function *F);

  /* Same as ExecutionEngine::LoadValueFromMemory, but if any of the
   * bytes that should be loaded occur in a memory block in DryRunMem,
   * then load the latest such bytes instead of the corresponding
   * bytes in memory.
   */
  virtual void DryRunLoadValueFromMemory(GenericValue &Val,
                                         GenericValue *Src,
                                         SymAddrSize Src_sas, Type *Ty);

  /* Returns true if I is a call to an unknown intrinsic function, as
   * defined by Interpreter::visitAnyCallInst. Such function calls are
   * replaced by some other sequence of instructions upon execution.
   */
  virtual bool isUnknownIntrinsic(Instruction &I);
  /* Returns true iff I is a call to pthread_join.
   *
   * The ID of the thread which should be joined is read from the
   * arguments of I, and stored into *tid.
   */
  virtual bool isPthreadJoin(Instruction &I, int *tid);
  /* Returns true iff I is a call to pthread_mutex_lock.
   *
   * The pointer to the mutex is read from the arguments of I, and
   * stored to *ptr.
   */
  virtual bool isPthreadMutexLock(Instruction &I, GenericValue **ptr);
  /* Returns true iff I is a call to __VERIFIER_load_await_*.
   *
   * The address is stored to *ptr, and the condition is stored to *cond.
   */
  virtual bool isLoadAwait(Instruction &I, GenericValue **ptr, AwaitCond *cond);
  /* Returns true iff I is a call to __VERIFIER_xchg_await_*.
   *
   * The address is stored to *ptr, and the condition is stored to *cond.
   */
  virtual bool isXchgAwait(Instruction &I, GenericValue **ptr, AwaitCond *cond);
  /* Returns true iff CS is a call to inline assembly.
   *
   * If CS is a call to inline assembly, then *asmstr is assigned the
   * assembly string.
   */
  virtual bool isInlineAsm(AnyCallInst CI, std::string *asmstr);
  /* I should be the next instruction that has been
   * scheduled. CurrentProc and ECStack shoauld be properly setup to
   * execute I.
   *
   * If the scheduling of I should be refused, then it is refused, and
   * true is returned. Otherwise false is returned.
   *
   * The reason for refusing a scheduling is usually that the
   * scheduled instruction is blocked, waiting for something (e.g. a
   * mutex lock or the termination of another thread).
   */
  virtual bool checkRefuse(Instruction &I);

  /* === External Functions with Special Support ===
   *
   * When executing an external function call to an external function
   * with special support (such as e.g. pthread_create), one of these
   * methods will be called instead of calling the actual function.
   *
   * F should be the called function, and ArgVals its arguments.
   */
  virtual void callPthreadCreate(Function *F, const std::vector<GenericValue> &ArgVals);
  virtual void callPthreadJoin(Function *F, const std::vector<GenericValue> &ArgVals);
  virtual void callPthreadSelf(Function *F, const std::vector<GenericValue> &ArgVals);
  virtual void callPthreadExit(Function *F, const std::vector<GenericValue> &ArgVals);
  virtual void callPthreadMutexInit(Function *F, const std::vector<GenericValue> &ArgVals);
  virtual void callPthreadMutexLock(Function *F, const std::vector<GenericValue> &ArgVals);
  virtual void callPthreadMutexLock(void *lck);
  virtual void callPthreadMutexTryLock(Function *F, const std::vector<GenericValue> &ArgVals);
  virtual void callPthreadMutexUnlock(Function *F, const std::vector<GenericValue> &ArgVals);
  virtual void callPthreadMutexDestroy(Function *F, const std::vector<GenericValue> &ArgVals);
  virtual void callPthreadCondInit(Function *F, const std::vector<GenericValue> &ArgVals);
  virtual void callPthreadCondSignal(Function *F, const std::vector<GenericValue> &ArgVals);
  virtual void callPthreadCondBroadcast(Function *F, const std::vector<GenericValue> &ArgVals);
  virtual void callPthreadCondWait(Function *F, const std::vector<GenericValue> &ArgVals);
  virtual void doPthreadCondAwake(void *cnd, void *lck);
  virtual void callPthreadCondDestroy(Function *F, const std::vector<GenericValue> &ArgVals);
  virtual void callNondetInt(Function *F, const std::vector<GenericValue> &ArgVals);
  virtual void callAssume(Function *F, const std::vector<GenericValue> &ArgVals);
  virtual void callMCalloc(Function *F, const std::vector<GenericValue> &ArgVals, bool isCalloc);
  virtual void callFree(Function *F, const std::vector<GenericValue> &ArgVals);
  virtual void callAssertFail(Function *F, const std::vector<GenericValue> &ArgVals);
  virtual void callAtexit(Function *F, const std::vector<GenericValue> &ArgVals);
  virtual void callLoadAwait(Function *F, const std::vector<GenericValue> &ArgVals);
  virtual void callXchgAwait(Function *F, const std::vector<GenericValue> &ArgVals);

private:
  void CheckAwaitWakeup(const GenericValue &Val, const void *ptr, const SymAddrSize &sas);
  bool isAnyAwait(Instruction &I, GenericValue **ptr, AwaitCond *cond,
                  const char *name_prefix, unsigned nargs);
};

}  // namespace llvm

#endif
