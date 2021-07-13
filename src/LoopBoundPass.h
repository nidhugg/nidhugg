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

#ifndef __LOOP_BOUND_PASS_H__
#define __LOOP_BOUND_PASS_H__

#include <llvm/Pass.h>
#include <llvm/Analysis/LoopPass.h>

#include "Debug.h"

class LoopBoundPass : public llvm::LoopPass{
public:
  static char ID;
  LoopBoundPass(int depth) : llvm::LoopPass(ID), unroll_depth(depth) {
    if(unroll_depth < 0){
      Debug::warn("loopboundpass:negative depth")
        << "WARNING: Negative depth given to loop bounding pass. Defaulting to depth zero.\n";
      unroll_depth = 0;
    }
  };
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const;
  virtual bool runOnLoop(llvm::Loop *L, llvm::LPPassManager &LPM);
#ifdef LLVM_PASS_GETPASSNAME_IS_STRINGREF
  virtual llvm::StringRef getPassName() const { return "LoopBoundingPass"; };
#else
  virtual const char *getPassName() const { return "LoopBoundingPass"; };
#endif
protected:
  int unroll_depth;
};

#endif
