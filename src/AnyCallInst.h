/* Copyright (C) 2020 Magnus Lång
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

#ifndef __ANYCALLINST_H__
#define __ANYCALLINST_H__

#include <llvm/IR/Instructions.h>
#if defined(HAVE_LLVM_SUPPORT_CALLSITE_H)
#include <llvm/Support/CallSite.h>
#elif defined(HAVE_LLVM_IR_CALLSITE_H)
#include <llvm/IR/CallSite.h>
#endif

#include <cassert>

/* Because older llvms do not have the llvm::CallBase base class, but instead
 * have a helper type llvm::CallSite that wraps any instruction type that is a
 * call, we need a consistent way of handling them. We introduce our own wrapper
 * type, AnyCallInst, with an interface as close to llvm::CallBase as possible.
 */

#ifdef LLVM_HAS_CALLBASE

class AnyCallInst {
  llvm::CallBase *CB;

 public:
  AnyCallInst() : CB(nullptr) {}
  AnyCallInst(llvm::CallBase *CB) : CB(CB) { assert(CB); }
  AnyCallInst(llvm::CallBase &CB) : CB(&CB) {}

  operator bool() const { return (bool)CB; }
  bool operator==(std::nullptr_t) { return !CB; }
  llvm::Instruction* operator &() { return CB; }
  const llvm::Instruction* operator &() const { return CB; }
  llvm::Function *getCalledFunction() const { return CB->getCalledFunction(); }
  llvm::Value *getCalledOperand() const { return CB->getCalledOperand(); }
  auto arg_begin() { return CB->arg_begin(); }
  auto arg_end() { return CB->arg_end(); }
  auto arg_size() { return CB->arg_size(); }
};

#else /* #ifdef LLVM_HAS_CALLBASE */

class AnyCallInst {
  llvm::CallSite CS;

 public:
  AnyCallInst() {}
  AnyCallInst(llvm::CallSite CS) : CS(CS) {}

  operator bool() const { return (bool)CS; }
  bool operator==(std::nullptr_t) { return !CS.getInstruction(); }
  llvm::Instruction* operator &() { return CS.getInstruction(); }
  const llvm::Instruction* operator &() const { return CS.getInstruction(); }
  llvm::Function *getCalledFunction() const { return CS.getCalledFunction(); }
  llvm::Value *getCalledOperand() const { return CS.getCalledValue(); }
  auto arg_begin() { return CS.arg_begin(); }
  auto arg_end() { return CS.arg_end(); }
  auto arg_size() { return CS.arg_size(); }
};

#endif /* ! #ifdef LLVM_HAS_CALLBASE */

#endif // __ANYCALLINST_H__
