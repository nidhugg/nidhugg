// For the parts of the code originating in LLVM:
//===- Interpreter.cpp - Top-Level LLVM Interpreter Implementation --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LLVMLICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the top-level functionality for the LLVM interpreter.
// This interpreter is designed to be a very simple, portable, inefficient
// interpreter.
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

#include <csetjmp>
#include <csignal>
#include <malloc.h>
#ifdef HAVE_VALGRIND_VALGRIND_H
#include <valgrind/valgrind.h>
#endif

#include <Interpreter.h>
#include <llvm/CodeGen/IntrinsicLowering.h>
#if defined(HAVE_LLVM_IR_DERIVEDTYPES_H)
#include <llvm/IR/DerivedTypes.h>
#elif defined(HAVE_LLVM_DERIVEDTYPES_H)
#include <llvm/DerivedTypes.h>
#endif
#if defined(HAVE_LLVM_IR_MODULE_H)
#include <llvm/IR/Module.h>
#elif defined(HAVE_LLVM_MODULE_H)
#include <llvm/Module.h>
#endif
#include <cstring>
using namespace llvm;

/// create - Create a new interpreter object.  This can never fail.
///
ExecutionEngine *Interpreter::create(Module *M, TSOPSOTraceBuilder &TB,
                                     const Configuration &C, std::string* ErrStr) {
  // Tell this Module to materialize everything and release the GVMaterializer.
#ifdef LLVM_MODULE_MATERIALIZE_ALL_PERMANENTLY_ERRORCODE_BOOL
  if(std::error_code EC = M->materializeAllPermanently()){
    // We got an error, just return 0
    if(ErrStr) *ErrStr = EC.message();
    return 0;
  }
#elif defined LLVM_MODULE_MATERIALIZE_ALL_PERMANENTLY_BOOL_STRPTR
  if (M->MaterializeAllPermanently(ErrStr)){
    // We got an error, just return 0
    return 0;
  }
#else
  if(std::error_code EC = M->materializeAll()){
    // We got an error, just return 0
    if(ErrStr) *ErrStr = EC.message();
    return 0;
  }
#endif

  return new Interpreter(M,TB,C);
}

//===----------------------------------------------------------------------===//
// Interpreter ctor - Initialize stuff
//
Interpreter::Interpreter(Module *M, TSOPSOTraceBuilder &TB,
                         const Configuration &C)
#ifdef LLVM_EXECUTIONENGINE_MODULE_UNIQUE_PTR
  : ExecutionEngine(std::unique_ptr<Module>(M)),
#else
  : ExecutionEngine(M),
#endif
    TD(M), TB(TB), conf(C) {

  Threads.push_back(Thread());
  Threads.back().cpid = CPid();
  CurrentThread = 0;
  AtomicFunctionCall = -1;
  memset(&ExitValue.Untyped, 0, sizeof(ExitValue.Untyped));
#ifdef LLVM_EXECUTIONENGINE_DATALAYOUT_PTR
  setDataLayout(&TD);
#endif
  // Initialize the "backend"
  initializeExecutionEngine();
  initializeExternalFunctions();
  emitGlobals();

  IL = new IntrinsicLowering(TD);
}

static sigjmp_buf cf_env;

static void cf_handler(int signum){
  siglongjmp(cf_env,1);
}

static void CheckedFree(void *ptr, std::function<void()> &on_error){
  mallopt(M_CHECK_ACTION,2);
  struct sigaction act, orig_act_segv, orig_act_abrt;
  act.sa_handler = cf_handler;
  act.sa_flags = SA_RESETHAND;
  sigemptyset(&act.sa_mask);
  if(sigaction(SIGABRT,&act,&orig_act_abrt)){
    throw std::logic_error("Failed to setup signal handler.");
  }
  if(sigaction(SIGSEGV,&act,&orig_act_segv)){
    throw std::logic_error("Failed to setup signal handler.");
  }
  if(sigsetjmp(cf_env,1)){
    on_error();
  }else{
#ifdef HAVE_VALGRIND_VALGRIND_H
    auto vg_error_count = VALGRIND_COUNT_ERRORS;
    free(ptr);
    if(vg_error_count < VALGRIND_COUNT_ERRORS){
      on_error();
    }
#else
    free(ptr);
#endif
  }
  sigaction(SIGABRT,&orig_act_abrt,0);
  sigaction(SIGSEGV,&orig_act_segv,0);
  mallopt(M_CHECK_ACTION,3);
}

Interpreter::~Interpreter() {
  delete IL;
  /* Remove module from ExecutionEngine's list of modules. This is to
   * avoid the ExecutionEngine's destructor deleting the module before
   * we are done with it.
   */
  assert(Modules.size() == 1);
#ifdef LLVM_EXECUTIONENGINE_MODULE_UNIQUE_PTR
  /* First release all modules. */
  for(auto it = Modules.begin(); it != Modules.end(); ++it){
    it->release();
  }
#endif
  Modules.clear();
  std::function<void()> on_error0 =
    [](){
    throw std::logic_error("Failed to free memory at destruction of Interpreter.");
  };
  for(void *ptr : AllocatedMemHeap){
    if(!FreedMem.count(ptr)) CheckedFree(ptr,on_error0);
  }
  for(void *ptr : AllocatedMemStack){
    if(!FreedMem.count(ptr)) CheckedFree(ptr,on_error0);
  }
  TSOPSOTraceBuilder *TBptr = &TB;
  for(auto it = FreedMem.begin(); it != FreedMem.end(); ++it){
    std::function<void()> on_error =
      [&it,TBptr](){
      TBptr->memory_error("Failure at free.",it->second);
    };
    CheckedFree(it->first,on_error);
  }
}

void Interpreter::runAtExitHandlers () {
  while (!AtExitHandlers.empty()) {
    callFunction(AtExitHandlers.back(), std::vector<GenericValue>());
    AtExitHandlers.pop_back();
    run();
  }
}

/// run - Start execution with the specified function and arguments.
///
#ifdef LLVM_EXECUTION_ENGINE_RUN_FUNCTION_VECTOR
GenericValue
Interpreter::runFunction(Function *F,
                         const std::vector<GenericValue> &ArgValues) {
#else
GenericValue
Interpreter::runFunction(Function *F,
                         ArrayRef<GenericValue> ArgValues) {
#endif
  assert (F && "Function *F was null at entry to run()");

  // Try extra hard not to pass extra args to a function that isn't
  // expecting them.  C programmers frequently bend the rules and
  // declare main() with fewer parameters than it actually gets
  // passed, and the interpreter barfs if you pass a function more
  // parameters than it is declared to take. This does not attempt to
  // take into account gratuitous differences in declared types,
  // though.
  std::vector<GenericValue> ActualArgs;
  const unsigned ArgCount = F->getFunctionType()->getNumParams();
  for (unsigned i = 0; i < ArgCount; ++i)
    ActualArgs.push_back(ArgValues[i]);

  // Set up the function call.
  callFunction(F, ActualArgs);

  // Start executing the function.
  run();

  return ExitValue;
}
