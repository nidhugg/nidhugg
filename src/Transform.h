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

#include <config.h>

#ifndef __TRANSFORM_H__
#define __TRANSFORM_H__

#include "Configuration.h"

#if defined(HAVE_LLVM_IR_MODULE_H)
#include <llvm/IR/Module.h>
#elif defined(HAVE_LLVM_MODULE_H)
#include <llvm/Module.h>
#endif

namespace Transform {

  /* Read an LLVM module from infile, transform it according to conf,
   * and store the resulting module to outfile.
   *
   * Transformations:
   * - SpinAssume (enabled by conf.transform_spin_assume)
   *   Replace each spin loop with a single call to __VERIFIER_assume.
   * - LoopUnroll (enabled by conf.transform_loop_unroll)
   *   Unroll each loop such that its body can execute at most a given
   *   number of times.
   */
  void transform(std::string infile, std::string outfile, const Configuration &conf);

  /* Transform the LLVM assembly or byte code in src according to
   * conf. Return the transformed code as LLVM assembly. (See above
   * for documentation about transformations.)
   */
  std::string transform(const std::string &src, const Configuration &conf);

  /* Transform mod according to conf. (See above for documentation
   * about transformations.)
   *
   * Return true iff mod was modified.
   */
  bool transform(llvm::Module &mod, const Configuration &conf);

}

#endif

