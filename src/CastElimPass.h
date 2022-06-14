/* Copyright (C) 2021 Magnus Lång
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

#ifndef __CAST_ELIM_PASS_H__
#define __CAST_ELIM_PASS_H__

#include <llvm/Analysis/LoopPass.h>
#include <llvm/Pass.h>

/* The CastElim pass eliminates some unnecessary casts that can
 * complicate later analyses. */
class CastElimPass final : public llvm::FunctionPass{
public:
  static char ID;
  CastElimPass() : llvm::FunctionPass(ID) {}
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  bool runOnFunction(llvm::Function &F) override;
#ifdef LLVM_PASS_GETPASSNAME_IS_STRINGREF
  llvm::StringRef getPassName() const override { return "CastElimPass"; }
#else
  const char *getPassName() const override { return "CastElimPass"; }
#endif
private:
};

#endif
