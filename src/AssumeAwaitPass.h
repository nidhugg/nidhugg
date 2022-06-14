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

#include <config.h>

#ifndef __ASSUME_AWAIT_PASS_H__
#define __ASSUME_AWAIT_PASS_H__

#include <llvm/Analysis/LoopPass.h>
#include <llvm/Pass.h>

/* The AssumeAwaitPass identifies calls to __VERIFIER_assume with simple
 * conditions and replaces them with
 */
class AssumeAwaitPass final : public llvm::FunctionPass{
public:
  static char ID;
  AssumeAwaitPass() : llvm::FunctionPass(ID) {}
  bool doInitialization(llvm::Module &M) override;
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  bool runOnFunction(llvm::Function &F) override;
#ifdef LLVM_PASS_GETPASSNAME_IS_STRINGREF
  llvm::StringRef getPassName() const override { return "AssumeAwaitPass"; }
#else
  const char *getPassName() const override { return "AssumeAwaitPass"; }
#endif
private:
  static const unsigned no_sizes = 4;
  static unsigned sizes[no_sizes];
  llvm::Value *F_load_await[no_sizes];
  llvm::Value *F_xchg_await[no_sizes];
  bool tryRewriteAssume(llvm::Function *F, llvm::BasicBlock *BB, llvm::Instruction *I) const;
  bool tryRewriteAssumeCmpXchg(llvm::Function *F, llvm::BasicBlock *BB, llvm::CallInst *I) const;
  llvm::Value *getAwaitFunction(llvm::Instruction *Load) const;
};

#endif
