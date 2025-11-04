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

#include "TraceUtil.h"

#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/Type.h>

#include <fstream>
#include <locale>
#include <set>
#include <utility>
#include <vector>

bool TraceUtil::get_location(const llvm::MDNode *m,
                             int *lineno,
                             std::string *fname,
                             std::string *dname){
  if(!m){
    return false;
  }
  const llvm::DILocation &loc = static_cast<const llvm::DILocation&>(*m);
  *lineno = loc.getLine();
  *fname = std::string(loc.getFilename());
  *dname = std::string(loc.getDirectory());
  return (*lineno >= 0) && fname->size() && dname->size();
}

namespace {
  bool is_absolute_path(const std::string &fname){
    return fname.size() && fname.front() == '/';
  }
}

std::string TraceUtil::get_src_line_verbatim(const llvm::MDNode *m){
  int lineno;
  std::string fname, dname;
  if(!get_location(m,&lineno,&fname,&dname)){
    return "";
  }
  return get_src_line_verbatim(SrcLocVector::LocRef(lineno, fname, dname));
}

std::string TraceUtil::get_src_line_verbatim(const SrcLocVector::LocRef &loc) {
  std::string fullname;
  if(is_absolute_path(loc.file)){
    fullname = loc.file;
  }else{
    fullname = loc.dir;
    if(fullname.back() != '/') fullname += "/";
    fullname += loc.file;
  }
  std::ifstream ifs(fullname);
  unsigned int cur_line = 0;
  std::string ln;
  while(ifs.good() && cur_line < loc.line){
    std::getline(ifs,ln);
    ++cur_line;
  }

  /* Strip whitespace */
  while(ln.size() && std::isspace(ln.front())){
    ln = ln.substr(1);
  }
  while(ln.size() && std::isspace(ln.back())){
    ln = ln.substr(0,ln.size()-1);
  }

  return ln;
}

std::string TraceUtil::basename(const std::string &fname){
  std::size_t i = fname.find_last_of('/');
  if(i == std::string::npos) return fname;
  if(i == fname.size()-1){
    return basename(fname.substr(0,fname.size()-1));
  }
  return fname.substr(i+1);
}

SrcLocVector SrcLocVectorBuilder::build() {
  SrcLocVector ret(std::move(vector));
  vector.string_table.clear();
  vector.locations.clear();
  lookup.clear();
  return ret;
}

void SrcLocVectorBuilder::push_from(const llvm::MDNode *md) {
  int lineno;
  std::string file_name, dir_name;
  if (!TraceUtil::get_location(md, &lineno, &file_name, &dir_name)) {
    vector.locations.emplace_back();
  } else {
    vector.locations.emplace_back(lineno, intern_string(std::move(file_name)),
                                  intern_string(std::move(dir_name)));
  }
}

unsigned SrcLocVectorBuilder::intern_string(std::string &&s) {
  auto it = lookup.find(s);
  if (it != lookup.end()) {
    return it->second;
  }
  unsigned no = vector.string_table.size();
  vector.string_table.push_back(s);
  lookup.emplace(std::move(s), no);
  return no;
}
