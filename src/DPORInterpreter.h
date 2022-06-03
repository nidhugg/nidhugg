/* Copyright (C) 2019 Magnus Lång
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

#ifndef __DPOR_INTERPRETER_H__
#define __DPOR_INTERPRETER_H__

#include <llvm/ExecutionEngine/ExecutionEngine.h>

#include <memory>

/* Common base class for all Interpreter instances used in nidhugg */
class DPORInterpreter : public llvm::ExecutionEngine {
  /* True if we have executed a false assume statement.
   */
  bool AssumeBlocked;
protected:
  void setAssumeBlocked(bool value) { AssumeBlocked = value; }
public:
  DPORInterpreter(llvm::Module *M)
#ifdef LLVM_EXECUTIONENGINE_MODULE_UNIQUE_PTR
  : llvm::ExecutionEngine(std::unique_ptr<llvm::Module>(M))
#else
  : llvm::ExecutionEngine(M)
#endif
  {
    AssumeBlocked = false;
  }
  bool assumeBlocked() const { return AssumeBlocked; }
};

#endif
