/* Copyright (C) 2015-2017 Carl Leonardsson
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

#include "POWERARMTraceBuilder.h"
#include "POWERARMTraceBuilder.tcc"

#include <iomanip>
#include <sstream>

template class PATB_impl::TB<PATB_impl::ARM,PATB_impl::CB_ARM,PATB_impl::ARMEvent>;
template class PATB_impl::TB<PATB_impl::POWER,PATB_impl::CB_POWER,PATB_impl::POWEREvent>;

static std::vector<std::string> get_indentation(int n_procs, int ind){
  std::vector<std::string> res;
  std::string sind = "";
  std::string ind1 = "";
  for(int i = 0; i < ind; ++i) ind1+=" ";
  for(int p = 0; p < n_procs; ++p){
    res.push_back(sind);
    sind += ind1;
  }
  return res;
}

std::string PATB_impl::PATrace::to_string(int _ind) const{
  if(string_rep.size()){ // There is already a prepared string representation
    return string_rep;
  }
  std::vector<std::string> ind = get_indentation(cpids.size(),_ind);
  std::stringstream ss;
  for(const Evt &E : events){
    IID<CPid> iid(cpids[E.iid.get_pid()],E.iid.get_index());
    ss << ind[E.iid.get_pid()] << iid << "\n";
  }
  return ss.str();
}

std::string PATB_impl::TraceRecorder::to_string(int ind) const{
  std::stringstream ss;
  for(const Line &L : lines){
    /* Indentation */
    for(int i = 0; i < ind*L.proc; ++i){
      ss << " ";
    }
    /* Line */
    ss << L.ln << "\n";
  }
  return ss.str();
}

void PATB_impl::TraceRecorder::trace_commit(const IID<int> &iid,
                                            const Param &param,
                                            const std::vector<Access> &accesses,
                                            const std::vector<ByteAccess> &baccesses,
                                            std::vector<MBlock> values){
  last_committed.consumed = false;
  last_committed.iid = iid;
  last_committed.param = param;
  last_committed.accesses = accesses;
  last_committed.baccesses = baccesses;
  last_committed.values = std::move(values);
}

bool PATB_impl::TraceRecorder::source_line(int proc, const llvm::MDNode *md, std::string *srcln, int *id_len){
  std::string ln;
  /* Process / IID */
  if(last_committed.consumed){
    ln = "("+(*cpids)[proc].to_string()+"): ";
  }else{
    std::stringstream ss;
    ss << "(" << (*cpids)[proc].to_string() << ","
       << last_committed.iid.get_index() << "): ";
    ln = ss.str();
  }
  *id_len = ln.size();
  bool success = false;
  /* Source code */
  if(md){
    int lineno;
    std::string fname, dname;
    if(get_location(md,&lineno,&fname,&dname)){
      success = true;
      std::stringstream ss;
      ss << basename(fname) << ":" << lineno
         << " " << get_src_line_verbatim(md);
      ln += ss.str();
    }
  }
  *srcln = ln;
  return success;
}

void PATB_impl::TraceRecorder::trace_register_metadata(int proc, const llvm::MDNode *md){
  assert(0 <= proc && proc < int(cpids->size()));
  if(last_committed.consumed) return;
  std::string ln;
  int cpid_indent;
  source_line(proc,md,&ln,&cpid_indent);
  lines.push_back({proc,ln});
  if(!last_committed.consumed){
    last_committed.consumed = true;
    int load_count = 0;
    for(const Access &A : last_committed.accesses){ // One line per access
      ln = "";
      for(int i = 0; i < cpid_indent; ++i) ln += " "; // Indentation
      if(A.type == Access::LOAD){
        ln += "load 0x";
        std::stringstream ss;
        ss << std::setbase(16);
        const MBlock &M = last_committed.values[load_count];
        for(int i = 0; i < M.get_ref().size; ++i){
          ss << std::setw(2) << std::setfill('0')
             << unsigned(((unsigned char*)(M.get_block()))[i]);
        }
        ss << " from [0x"
           << (unsigned long)(A.addr.ref)
           << "] source: ";
        VecSet<IID<int> > src;
        for(unsigned i = 0; i < last_committed.baccesses.size(); ++i){
          if(last_committed.baccesses[i].type == ByteAccess::LOAD &&
             A.addr.includes(last_committed.baccesses[i].addr)){
            src.insert(last_committed.param.choices[i].src);
          }
        }
        ss << std::setbase(10);
        bool first = true;
        for(const IID<int> &s : src){
          if(!first) ss << ", ";
          first = false;
          if(s.is_null()){
            ss << "(initial value)";
          }else{
            ss << "(" << (*cpids)[s.get_pid()] << "," << s.get_index() << ")";
          }
        }
        ln += ss.str();
        ++load_count;
      }else{ // Store
        ln += "store 0x";
        std::stringstream ss;
        ss << std::setbase(16);
        for(int i = 0; i < A.data.get_ref().size; ++i){
          ss << std::setw(2) << std::setfill('0')
             << unsigned(((unsigned char*)(A.data.get_block()))[i]);
        }
        ss << " to [0x"
           << (unsigned long)(A.addr.ref)
           << "]";
        ln += ss.str();
      }
      lines.push_back({proc,ln});
    }
  }
}

void PATB_impl::TraceRecorder::trace_register_external_function_call(int proc, const std::string &fname, const llvm::MDNode *md){
}

void PATB_impl::TraceRecorder::trace_register_function_entry(int proc, const std::string &fname, const llvm::MDNode *md){
  assert(last_committed.consumed);
  if(int(fun_call_stack.size()) <= proc){
    fun_call_stack.resize(proc+1);
  }
  fun_call_stack[proc].push_back(fname);
  std::string ln;
  int cpid_len;
  bool src_found = source_line(proc,md,&ln,&cpid_len);
  if(src_found){
    lines.push_back({proc,ln});
    ln = "";
    for(int i = 0; i < cpid_len; ++i) ln += " ";
    ln += "Entering function "+fname;
    lines.push_back({proc,ln});
  }else{
    lines.push_back({proc,ln+"Entering function "+fname});
  }
}

void PATB_impl::TraceRecorder::trace_register_function_exit(int proc){
  assert(last_committed.consumed);
  assert(proc < int(fun_call_stack.size()));
  assert(fun_call_stack[proc].size());
  std::string ln;
  int cpid_len;
  source_line(proc,0,&ln,&cpid_len);
  lines.push_back({proc,ln+"Returning from "+fun_call_stack[proc].back()});
  fun_call_stack[proc].pop_back();
}

void PATB_impl::TraceRecorder::trace_register_error(int proc, const std::string &err_msg){
  lines.push_back({proc,"Error: "+err_msg});
}
