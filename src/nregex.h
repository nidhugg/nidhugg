/* Copyright (C) 2016-2017 Carl Leonardsson
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

#include <string>

/* This namespace provides wrapper functions for the standard C
 * library's POSIX regular expressions (regex.h).
 *
 * Use this instead of the new regex library of C++11, since it is not
 * supported by older compilers.
 */
namespace nregex{
  /* Replace all substrings of tgt which match the extended POSIX
   * regular expression regex with format, and return the resulting
   * string.
   *
   * format may contain $n for some natural number n. If the match
   * contains at least n+1 groups, then $n is replaced by the n:th
   * group of the match. Otherwise $n is left unchanged as in the
   * format.
   */
  std::string regex_replace(std::string tgt, std::string regex, std::string format);
}
