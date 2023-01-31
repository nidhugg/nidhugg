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

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>

#include <string>

namespace Debug {

  /* Returns a reference to an ostream for warnings. (Same as
   * llvm::errs().)
   */
  llvm::raw_ostream &warn();

  /* Returns a reference to an ostream for warnings. If warn(wid) is
   * called multiple times with the same warning identifier wid, then
   * a visible ostream is returned only the first time. On later
   * calls, a dummy ostream is returned which will quietly swallow
   * anything that is sent to it.
   */
  llvm::raw_ostream &warn(const std::string &wid);

}  // namespace Debug

#endif
