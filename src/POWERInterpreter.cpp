// For the parts of the code originating in LLVM:
//===- Interpreter.cpp - Top-Level LLVM Interpreter Implementation --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the top-level functionality for the LLVM interpreter.
// This interpreter is designed to be a very simple, portable, inefficient
// interpreter.
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

#include "POWERInterpreter.h"

#include <llvm/CodeGen/IntrinsicLowering.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>

#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

/// Create a new interpreter object.
///
std::unique_ptr<POWERInterpreter> POWERInterpreter::
create(llvm::Module *M, POWERARMTraceBuilder &TB, const Configuration &conf,
       std::string *ErrStr) {
  // Tell this Module to materialize everything and release the GVMaterializer.
  if (llvm::Error Err = M->materializeAll()) {
    std::string Msg;
    handleAllErrors(std::move(Err), [&](llvm::ErrorInfoBase &EIB) {
      Msg = EIB.message();
    });
    if (ErrStr)
      *ErrStr = Msg;
    // We got an error, just return 0
    return nullptr;
  }

  return std::unique_ptr<POWERInterpreter>(new POWERInterpreter(M,TB,conf));
}

//===----------------------------------------------------------------------===//
// Interpreter ctor - Initialize stuff
//
POWERInterpreter::POWERInterpreter(llvm::Module *M, POWERARMTraceBuilder &TB, const Configuration &conf)
  : DPORInterpreter(M), TD(M), conf(conf), TB(TB) {
  Threads.emplace_back(CPid());
  CurrentThread = 0;
  memset(&ExitValue.Untyped, 0, sizeof(ExitValue.Untyped));

  // Initialize the "backend"
  initializeExecutionEngine();
  emitGlobals();

  IL = new llvm::IntrinsicLowering(TD);

#if LLVM_VERSION_MAJOR >= 18
  throw std::logic_error("POWER interpreter is not available for LLVM >= 18; "
                         "consider configuring Nidhugg with an earlier LLVM");
#else
  llvm::Value *V0 = llvm::ConstantInt::get(llvm::Type::getInt32Ty(M->getContext()),0);
  llvm::Value *P0_32 = llvm::ConstantPointerNull::get(llvm::Type::getInt32PtrTy(M->getContext()));
  dummy_store = new llvm::StoreInst(V0,P0_32,/*IsVolatile=*/false,/*Align=*/{},
                                    /*InsertBefore=*/(llvm::Instruction*)nullptr);
  llvm::Value *P0_8 = llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(M->getContext()));
  llvm::Type *I8 = llvm::Type::getInt8Ty(M->getContext());
  dummy_load8 = new llvm::LoadInst(I8, P0_8,"",/*IsVolatile=*/false,/*Align=*/{},
                                   /*InsertBefore=*/(llvm::Instruction*)nullptr);
#endif
}

POWERInterpreter::~POWERInterpreter() {
  delete IL;
  /* Remove module from ExecutionEngine's list of modules. This is to
   * avoid the ExecutionEngine's destructor deleting the module before
   * we are done with it.
   */
  assert(Modules.size() == 1);
  /* First release all modules. */
  for(auto it = Modules.begin(); it != Modules.end(); ++it){
    it->release();
  }
  Modules.clear();
  for(unsigned i = 0; i < Threads.size(); ++i){
    delete Threads[i].status;
  }
  delete dummy_store;
  delete dummy_load8;
}

void POWERInterpreter::runAtExitHandlers () {
  while (!AtExitHandlers.empty()) {
    callFunction(AtExitHandlers.back(), std::vector<llvm::Value*>());
    AtExitHandlers.pop_back();
    run();
  }
}

/// run - Start execution with the specified function and arguments.
///
llvm::GenericValue
POWERInterpreter::runFunction(llvm::Function *F,
                              llvm::ArrayRef<llvm::GenericValue> ArgValues) {
  assert (F && "Function *F was null at entry to run()");

  if (Blocked) return llvm::GenericValue();

  // Try extra hard not to pass extra args to a function that isn't
  // expecting them.  C programmers frequently bend the rules and
  // declare main() with fewer parameters than it actually gets
  // passed, and the interpreter barfs if you pass a function more
  // parameters than it is declared to take. This does not attempt to
  // take into account gratuitous differences in declared types,
  // though.
  std::vector<llvm::GenericValue> ActualArgs;
  const unsigned ArgCount = F->getFunctionType()->getNumParams();
  for (unsigned i = 0; i < ArgCount; ++i)
    ActualArgs.push_back(ArgValues[i]);

  // Set up the function call.
  callFunction(F, ActualArgs);

  // Start executing the function.
  run();

  return ExitValue;
}
