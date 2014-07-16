/* Copyright (C) 2014 Carl Leonardsson
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

#include "Trace.h"

#include <fstream>
#include <locale>
#include <set>
#include <sstream>

#include <llvm/DebugInfo.h>
#ifdef LLVM_INCLUDE_IR
#include <llvm/IR/Type.h>
#include <llvm/IR/Constants.h>
#else
#include <llvm/Type.h>
#include <llvm/Constants.h>
#endif
#include <llvm/Support/Dwarf.h>

Trace::Trace(const std::vector<IID<CPid> > &cmp,
             const std::vector<const llvm::MDNode*> &cmpmd,
             const std::vector<Error*> &errors)
  : computation(cmp), computation_md(cmpmd),
    errors(errors), sleep_set_blocked(false) {
};

Trace::~Trace(){
  for(unsigned i = 0; i < errors.size(); ++i){
    delete errors[i];
  }
};

Trace::Trace(const Trace &t)
  : computation(t.computation),
    computation_md(t.computation_md),
    sleep_set_blocked(t.sleep_set_blocked)
{
  for(unsigned i = 0; i < t.errors.size(); ++i){
    errors.push_back(t.errors[i]->clone());
  }
};

Trace &Trace::operator=(const Trace &t){
  if(&t != this){
    computation = t.computation;
    computation_md = t.computation_md;
    for(unsigned i = 0; i < errors.size(); ++i){
      delete errors[i];
    }
    errors.clear();
    for(unsigned i = 0; i < t.errors.size(); ++i){
      errors.push_back(t.errors[i]->clone());
    }
    sleep_set_blocked = t.sleep_set_blocked;
  }
  return *this;
};

std::string Trace::computation_to_string(int _ind) const{
  std::string s;
  std::string ind;
  assert(_ind >= 0);
  while(_ind){
    ind += " ";
    --_ind;
  }

  /* Setup individual indentation per process */
  std::map<CPid,std::string> cpind;
  {
    std::set<CPid> cpids;
    for(unsigned i = 0; i < computation.size(); ++i){
      cpids.insert(computation[i].get_pid());
    }

    std::string s = "";
    for(auto it = cpids.begin(); it != cpids.end(); ++it){
      cpind[*it] = s;
      s += ind;
    }
  }

  /* Identify error locations */
  std::map<IID<CPid>,Error*> error_locs;
  {
    for(unsigned i = 0; i < errors.size(); ++i){
      error_locs[errors[i]->get_location()] = errors[i];
    }
  }

  for(unsigned i = 0; i < computation.size(); ++i){
    std::string iid_str = ind + cpind[computation[i].get_pid()] + computation[i].to_string();
    s += iid_str;
    {
      int ln;
      std::string fname, dname;
      if(get_location(computation_md[i],&ln,&fname,&dname)){
        std::stringstream ss;
        ss << " " << basename(fname) << ":" << ln;
        ss << ": " << get_src_line_verbatim(computation_md[i]);
        s += ss.str();
      }
      s += "\n";
    }
    if(error_locs.count(computation[i])){
      // Indentation
      {
        int sz = iid_str.size();
        while(sz--){
          s += " ";
        }
      }
      // Error
      std::string errstr = error_locs[computation[i]]->to_string();
      if(errstr.find("\n") == std::string::npos){
        s += " Error: " + errstr;
      }else{
        s += " Error";
      }
      s += "\n";
    }
  }
  return s;
};

Error *AssertionError::clone() const{
  return new AssertionError(loc,condition);
};

std::string AssertionError::to_string() const{
  return "Assertion violation at "+loc.to_string()+": ("+condition+")";
};

Error *PthreadsError::clone() const{
  return new PthreadsError(loc,msg);
};

std::string PthreadsError::to_string() const{
  return "Pthreads error at "+loc.to_string()+": "+msg;
};

Error *SegmentationFaultError::clone() const{
  return new SegmentationFaultError(loc);
};

std::string SegmentationFaultError::to_string() const{
  return "Segmentation fault at "+loc.to_string();
};

Error *RobustnessError::clone() const{
  return new RobustnessError(loc);
};

std::string RobustnessError::to_string() const{
  return "The trace contains a happens-before cycle.";
};

bool Trace::get_location(const llvm::MDNode *m,
                         int *lineno,
                         std::string *fname,
                         std::string *dname){
  if(!m){
    return false;
  }
  llvm::DILocation loc(m);
  *lineno = loc.getLineNumber();
  *fname = loc.getFilename();
  *dname = loc.getDirectory();
  if(*fname == "" && *dname == ""){
    /* Failed to get file name and directory name.
     *
     * This may be caused by misformed metadata. Perform a brute-force
     * search through the metadata tree and try to find the names.
     */
    std::vector<const llvm::MDNode*> stack;
    std::set<const llvm::MDNode*> visited;
    stack.push_back(m);
    visited.insert(m);
    llvm::APInt tag_file_type(32,llvm::LLVMDebugVersion | llvm::dwarf::DW_TAG_file_type);
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
  return (lineno >= 0) && fname->size() && dname->size();
};

std::string Trace::get_src_line_verbatim(const llvm::MDNode *m){
  int lineno;
  std::string fname, dname;
  if(!get_location(m,&lineno,&fname,&dname)){
    return "";
  }
  std::string fullname;
  if(is_absolute_path(fname)){
    fullname = fname;
  }else{
    fullname = dname;
    if(fullname.back() != '/') fullname += "/";
    fullname += fname;
  }
  std::ifstream ifs(fullname);
  int cur_line = 0;
  std::string ln;
  while(ifs.good() && cur_line < lineno){
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
};

std::string Trace::basename(const std::string &fname){
  std::size_t i = fname.find_last_of('/');
  if(i == std::string::npos) return fname;
  if(i == fname.size()-1){
    return basename(fname.substr(0,fname.size()-1));
  }
  return fname.substr(i+1);
};

bool Trace::is_absolute_path(const std::string &fname){
  return fname.size() && fname.front() == '/';
};
