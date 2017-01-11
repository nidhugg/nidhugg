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

#ifndef __SPIN_ASSUME_PASS_H__
#define __SPIN_ASSUME_PASS_H__

#include <llvm/Pass.h>
#include <llvm/Analysis/LoopPass.h>

/* The DeclareAssumePass checks that __VERIFIER_assume is correctly
 * declared in the module. If they are incorrectly declared, an
 * error is raised. If they are not declared, then their (correct)
 * declaration is added to the module.
 *
 * This pass is a prerequisite for SpinAssumePass.
 */
class DeclareAssumePass : public llvm::ModulePass {
public:
  static char ID;
  DeclareAssumePass() : llvm::ModulePass(ID) {};
  virtual bool runOnModule(llvm::Module &M);
};

/* The SpinAssumePass identifies side-effect-free spin loops and
 * replaces them with a single, non-looping, call to
 * __VERIFIER_assume, while maintaining reachability for the module.
 *
 * This transformation can often drastically reduce the number of
 * traces that have to be explored.
 *
 * This pass may create dead basic blocks.
 *
 * Example:
 *
 * A spin loop like this:
 *
 * while(x < 2) {}
 *
 * is replaced by a call like this:
 *
 * __VERIFIER_assume(!(x < 2));
 */
class SpinAssumePass : public llvm::LoopPass{
public:
  static char ID;
  SpinAssumePass() : llvm::LoopPass(ID) {};
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const;
  virtual bool runOnLoop(llvm::Loop *L, llvm::LPPassManager &LPM);
  virtual const char *getPassName() const { return "SpinAssumePass"; };
protected:
  bool is_spin(const llvm::Loop *l) const;
  bool is_assume(llvm::Instruction &I) const;
  bool assumify_loop(llvm::Loop *l, llvm::LPPassManager &LPM);
  /* Remove basic blocks that have become disconnected in l and
   * parent loops of l. */
  void remove_disconnected(llvm::Loop *l);
  llvm::Function *F_assume;
};

#endif
