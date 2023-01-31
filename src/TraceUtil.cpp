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

#include <fstream>
#include <locale>
#include <set>
#include <utility>
#include <vector>

#if defined(HAVE_LLVM_DEBUGINFO_H)
#include <llvm/DebugInfo.h>
#elif defined(HAVE_LLVM_IR_DEBUGINFO_H)
#include <llvm/IR/DebugInfo.h>
#elif defined(HAVE_LLVM_ANALYSIS_DEBUGINFO_H)
#include <llvm/Analysis/DebugInfo.h>
#endif
#if defined(HAVE_LLVM_IR_TYPE_H)
#include <llvm/IR/Type.h>
#elif defined(HAVE_LLVM_TYPE_H)
#include <llvm/Type.h>
#endif
#if defined(HAVE_LLVM_IR_CONSTANTS_H)
#include <llvm/IR/Constants.h>
#elif defined(HAVE_LLVM_CONSTANTS_H)
#include <llvm/Constants.h>
#endif
#if defined(HAVE_LLVM_SUPPORT_DWARF_H)
#include <llvm/Support/Dwarf.h>
#elif defined(HAVE_LLVM_BINARYFORMAT_DWARF_H)
#include <llvm/BinaryFormat/Dwarf.h>
#endif

bool TraceUtil::get_location(const llvm::MDNode *m,
                             int *lineno,
                             std::string *fname,
                             std::string *dname){
  if(!m){
    return false;
  }
#ifdef LLVM_DILOCATION_IS_MDNODE
  const llvm::DILocation &loc = static_cast<const llvm::DILocation&>(*m);
#else
  llvm::DILocation loc(m);
#endif
#ifdef LLVM_DILOCATION_HAS_GETLINENUMBER
  *lineno = loc.getLineNumber();
#else
  *lineno = loc.getLine();
#endif
  *fname = std::string(loc.getFilename());
  *dname = std::string(loc.getDirectory());
#if defined(LLVM_MDNODE_OPERAND_IS_VALUE) /* Otherwise, disable fallback and hope that the C compiler produces well-formed metadata. */
  if(*fname == "" && *dname == ""){
    /* Failed to get file name and directory name.
     *
     * This may be caused by malformed metadata. Perform a brute-force
     * search through the metadata tree and try to find the names.
     */
    std::vector<const llvm::MDNode*> stack;
    std::set<const llvm::MDNode*> visited;
    stack.push_back(m);
    visited.insert(m);
#ifdef HAVE_LLVM_LLVMDEBUGVERSION
    llvm::APInt tag_file_type(32,llvm::LLVMDebugVersion | llvm::dwarf::DW_TAG_file_type);
#else
    llvm::APInt tag_file_type(32,llvm::dwarf::DW_TAG_file_type);
#endif
    while(stack.size()){
      const llvm::MDNode *n = stack.back();
      stack.pop_back();
      llvm::Value *tag = n->getOperand(0);
      if(tag->getType()->isIntegerTy(32)){
        const llvm::ConstantInt *tag_i =
          llvm::dyn_cast<llvm::ConstantInt>(tag);
        if(tag_i->getValue() == tag_file_type){
          if(n->getNumOperands() >= 3 &&
             (n->getOperand(1) && llvm::dyn_cast<llvm::MDString>(n->getOperand(1))) &&
             (n->getOperand(2) && llvm::dyn_cast<llvm::MDString>(n->getOperand(2)))){
            *fname = llvm::dyn_cast<llvm::MDString>(n->getOperand(1))->getString();
            *dname = llvm::dyn_cast<llvm::MDString>(n->getOperand(2))->getString();
          }else if(n->getNumOperands() >= 2 &&
                   n->getOperand(1) && llvm::dyn_cast<llvm::MDNode>(n->getOperand(1))){
            const llvm::MDNode *n2 = llvm::dyn_cast<llvm::MDNode>(n->getOperand(1));
            if(n2->getNumOperands() == 2 &&
               n2->getOperand(0) && llvm::dyn_cast<llvm::MDString>(n2->getOperand(0)) &&
               n2->getOperand(1) && llvm::dyn_cast<llvm::MDString>(n2->getOperand(1))){
              *fname = llvm::dyn_cast<llvm::MDString>(n2->getOperand(0))->getString();
              *dname = llvm::dyn_cast<llvm::MDString>(n2->getOperand(1))->getString();
            }
          }
          break;
        }else{
          for(unsigned i = 1; i < n->getNumOperands(); ++i){
            if(n->getOperand(i)){
              const llvm::MDNode *n2 = llvm::dyn_cast<llvm::MDNode>(n->getOperand(i));
              if(n2 && visited.count(n2) == 0){
                stack.push_back(n2);
              }
            }
          }
        }
      }
    }
  }
#endif
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
