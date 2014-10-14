/* Copyright (C) 2014 Carl Leonardsson
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

#ifndef __ADD_LIB_PASS_H__
#define __ADD_LIB_PASS_H__

#include <vector>

#include <llvm/Pass.h>
#include <llvm/Analysis/LoopPass.h>

/* AddLibPass adds source code definitions for some library functions
 * to the module.
 *
 * The purpose of this is to replace external function calls by
 * internal function calls, thereby avoiding full memory conflicts.
 */
class AddLibPass : public llvm::ModulePass{
public:
  static char ID;
  AddLibPass() : llvm::ModulePass(ID) {};
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const;
  virtual bool runOnModule(llvm::Module &M);
  virtual const char *getPassName() const { return "AddLibPass"; };
protected:
  /* If a function named name is already defined in M, then return
   * false. Otherwise search for a definition in the LLVM assembly
   * codes in src. If the function is declared in M, then the function
   * type in the definition should match that of the declaration. If
   * no appropriate definition can be found, a warning is printed and
   * the function remains undefined. If a definition was added, return
   * true, otherwise return false.
   */
  virtual bool optAddFunction(llvm::Module &M,
                              std::string name,
                              const std::vector<std::string> &src);
};

#endif

