/* Copyright (C) 2017 Carl Leonardsson
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

#include <llvm/IR/LLVMContext.h>

#ifndef __GLOBAL_CONTEXT_H__
#define __GLOBAL_CONTEXT_H__

namespace GlobalContext {

  /* Get a global context. The context is initialized on the first
   * call to get.
   */
  // llvm::LLVMContext &get();

  /* Destroy the global context. Does nothing if the global context
   * has not been created.
   */
  inline void destroy() {}
}  // namespace GlobalContext

#endif /* !defined(__GLOBAL_CONTEXT_H__) */
