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

#include "Debug.h"

#include "vecset.h"

llvm::raw_ostream &Debug::warn(){
  return llvm::errs();
}

llvm::raw_ostream &Debug::warn(const std::string &wid){
  static VecSet<std::string> seen_wids;
  static std::string dummy_str;
  static llvm::raw_string_ostream dummy_os(dummy_str);
  if(seen_wids.count(wid)){
    dummy_str.clear();
    return dummy_os;
  }else{
    seen_wids.insert(wid);
    return llvm::errs();
  }
}
