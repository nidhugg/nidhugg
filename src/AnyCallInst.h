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

#include <cassert>

/* Because llvms < 8.0.0 do not have the llvm::CallBase base class, but instead
 * have a helper type llvm::CallSite that wraps any instruction type that is a
 * call, we need a consistent way of handling them. We introduce our own wrapper
 * type, AnyCallInst, with an interface as close to llvm::CallBase as possible.
 */

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

#endif // __ANYCALLINST_H__
