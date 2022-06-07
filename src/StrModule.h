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

#ifndef __STR_MODULE_H__
#define __STR_MODULE_H__

#include <config.h>

#include "GlobalContext.h"

#include <string>

#if defined(HAVE_LLVM_IR_MODULE_H)
#include <llvm/IR/Module.h>
#elif defined(HAVE_LLVM_MODULE_H)
#include <llvm/Module.h>
#endif

namespace StrModule {

  /* Reads the file infile, interprets its contents as LLVM assembly
   * or bitcode, and creates and returns the corresponding module.
   */
  llvm::Module *read_module(std::string infile, llvm::LLVMContext &context);

  /* Interprets the contents of src as LLVM assembly or bitcode, and
   * creates and returns the corresponding module.
   */
  llvm::Module *read_module_src(const std::string &src, llvm::LLVMContext &context);

  /* Stores mod as assembly to outfile. */
  void write_module(llvm::Module *mod, std::string outfile);

  /* Returns a string representation of mod. */
  std::string write_module_str(llvm::Module *mod);

  /* Rewrites and returns the assembly code in s, to be of the form
   * expected by the version of LLVM against which this tool is
   * compiled.
   */
  std::string portasm(std::string s);
}  // namespace StrModule

#endif

