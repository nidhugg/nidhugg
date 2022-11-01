/* Copyright (C) 2022 Magnus Lång
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

#ifndef __LLVM_UTILS_H__
#define __LLVM_UTILS_H__

#include <llvm/IR/Type.h>

namespace LLVMUtils {
  llvm::Type* getPthreadTType(llvm::PointerType *PthreadTPtr);
};


#endif /* !defined(__LLVM_UTILS_H__) */
