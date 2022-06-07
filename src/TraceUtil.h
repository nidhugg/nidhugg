/* Copyright (C) 2014-2017 Carl Leonardsson,
 * Copyright (C) 2020 Magnus Lång
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

/* This file declares classes that are used to describe a particular
 * execution of a program: the Trace class and a number of Error
 * classes.
 */

#include <config.h>

#ifndef __TRACE_UTIL_H__
#define __TRACE_UTIL_H__

#include <map>
#include <string>

#include "Trace.h"
#if defined(HAVE_LLVM_IR_METADATA_H)
#include <llvm/IR/Metadata.h>
#elif defined(HAVE_LLVM_METADATA_H)
#include <llvm/Metadata.h>
#endif

namespace TraceUtil {
  /* Attempt to find the directory, file name and line number
   * corresponding to the metadata m.
   *
   * On success, return true and set *lineno, *file_name, *dir_name to
   * line number, file name and directory correspondingly. On failure
   * return false.
   */
  bool get_location(const llvm::MDNode *m,
                    int *lineno,
                    std::string *file_name,
                    std::string *dir_name);
  /* Attempt to find the location (directory, file name, line number)
   * described by m, read and return the corresponding line of source
   * code.
   *
   * On success, returns the code line, whitespace stripped. On
   * failure return "".
   */
  std::string get_src_line_verbatim(const llvm::MDNode *m);
  std::string get_src_line_verbatim(const SrcLocVector::LocRef &loc);
  std::string basename(const std::string &fname);
}  // namespace TraceUtil

class SrcLocVectorBuilder {
public:
  /* Clears this object, returning the built SrcLocVector. */
  SrcLocVector build();
  /* Push a location entry by parsing an LLVM MDNode */
  void push_from(const llvm::MDNode *md);
private:
  unsigned intern_string(std::string &&s);
  SrcLocVector vector;
  std::map<std::string,unsigned> lookup;
};

#endif
