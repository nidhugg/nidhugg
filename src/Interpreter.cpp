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
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#ifdef HAVE_VALGRIND_VALGRIND_H
#include <valgrind/valgrind.h>
#endif

#include <Interpreter.h>
#include <llvm/CodeGen/IntrinsicLowering.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Module.h>

#include <cstring>

using llvm::Interpreter;
using llvm::GenericValue;

/// create - Create a new interpreter object.  This can never fail.
///
std::unique_ptr<Interpreter> Interpreter::
create(llvm::Module *M, TSOPSOTraceBuilder &TB, const Configuration &C,
       std::string* ErrStr) {
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
#elif defined LLVM_MODULE_MATERIALIZE_LLVM_ALL_ERROR
  if (Error Err = M->materializeAll()) {
    std::string Msg;
    handleAllErrors(std::move(Err), [&](ErrorInfoBase &EIB) {
      Msg = EIB.message();
    });
    if (ErrStr)
      *ErrStr = Msg;
    // We got an error, just return 0
    return nullptr;
  }
#else
  if(std::error_code EC = M->materializeAll()){
    // We got an error, just return 0
    if(ErrStr) *ErrStr = EC.message();
    return 0;
  }
#endif

  return std::unique_ptr<Interpreter>(new Interpreter(M,TB,C));
}

//===----------------------------------------------------------------------===//
// Interpreter ctor - Initialize stuff
//
Interpreter::Interpreter(Module *M, TSOPSOTraceBuilder &TB,
                         const Configuration &C)
  : DPORInterpreter(M), TD(M), TB(TB), conf(C) {
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

  // Assign symbolic names to all global variables
  int glbl_ctr = 0;
  for (auto git = M->global_begin(); git != M->global_end(); ++git) {
    if (GlobalVariable *gv = dyn_cast<GlobalVariable>(&*git)) {
#ifdef LLVM_EXECUTIONENGINE_DATALAYOUT_PTR
      const DataLayout &DL = *getDataLayout();
#else
      const DataLayout &DL = getDataLayout();
#endif
      size_t GVSize = (size_t)(DL.getTypeAllocSize(gv->getValueType()));
      void *GVPtr = getPointerToGlobal(gv);
      SymMBlock mb = SymMBlock::Global(++glbl_ctr);
      AllocatedMem.emplace(GVPtr, SymMBlockSize(std::move(mb), GVSize));
    }
  }

  IL = new IntrinsicLowering(TD);
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
  for(void *ptr : AllocatedMemHeap){
    free(ptr);
  }
  for(void *ptr : AllocatedMemStack){
    free(ptr);
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

  if (Blocked) {
    return GenericValue();
  }

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

  // Run on main thread
  assert(CurrentThread == 0);
  TB.mark_available(0);

  // Set up the function call.
  assert(ECStack()->size() == 0);
  callFunction(F, ActualArgs);

  // Start executing the function.
  run();

  return ExitValue;
}
