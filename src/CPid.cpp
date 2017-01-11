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

#include "CPid.h"

#include <cassert>
#include <sstream>

CPid::CPid() : aux_idx(-1) {}

CPid::CPid(const std::vector<int> &pvec){
  assert(pvec.size() > 0);
  assert(pvec[0] == 0);
  auto b = pvec.begin();
  ++b;
  proc_seq = std::vector<int>(b,pvec.end());
  aux_idx = -1;
}

CPid::CPid(const std::initializer_list<int> &il){
  auto b = il.begin();
  assert(b != il.end());
  assert(*b == 0);
  ++b;
  proc_seq = std::vector<int>(b,il.end());
  aux_idx = -1;
}

CPid::CPid(const std::vector<int> &pvec, int i){
  assert(pvec.size() > 0);
  assert(pvec[0] == 0);
  assert(i >= 0);
  auto b = pvec.begin();
  ++b;
  proc_seq = std::vector<int>(b,pvec.end());
  aux_idx = i;
}

CPid CPid::spawn(int pn1) const{
  assert(!is_auxiliary());
  assert(0 <= pn1);
  CPid c = *this;
  c.proc_seq.push_back(pn1);
  return c;
}

CPid CPid::aux(int i) const{
  assert(!is_auxiliary());
  assert(0 <= i);
  CPid c = *this;
  c.aux_idx = i;
  return c;
}

bool CPid::is_auxiliary() const{
  return aux_idx >= 0;
}

std::string CPid::to_string() const{
  std::stringstream ss;
  ss << "<0";
  for(unsigned i = 0; i < proc_seq.size(); ++i){
    ss << "." << proc_seq[i];
  }
  if(is_auxiliary()){
    ss << "/" << aux_idx;
  }
  ss << ">";
  return ss.str();
}

CPid CPid::parent() const{
  assert(has_parent());
  CPid cp(*this);
  if(is_auxiliary()){
    cp.aux_idx = -1;
  }else{
    cp.proc_seq.pop_back();
  }
  return cp;
}

bool CPid::has_parent() const{
  return proc_seq.size() || is_auxiliary();
}

int CPid::compare(const CPid &c) const{
  unsigned i = 0;
  while(i < proc_seq.size() && i < c.proc_seq.size()){
    if(proc_seq[i] < c.proc_seq[i]) return -1;
    if(proc_seq[i] > c.proc_seq[i]) return 1;
    ++i;
  }
  if(i < c.proc_seq.size()) return -1;
  if(i < proc_seq.size()) return 1;
  if(aux_idx == c.aux_idx || (!is_auxiliary() && !c.is_auxiliary())){
    return 0;
  }
  if(aux_idx < c.aux_idx) return -1;
  return 1;
}

int CPid::get_aux_index() const{
  assert(is_auxiliary());
  return aux_idx;
}

CPidSystem::CPidSystem(){
  real_children.push_back({});
  aux_children.push_back({});
  parent.push_back(-1);
  cpids.push_back(CPid());
  identifiers[CPid()] = 0;
}

CPid CPidSystem::spawn(const CPid &c){
  int c_id = identifiers[c];
  CPid c2 = c.spawn(real_children[c_id].size());
  int c2_id = cpids.size();
  real_children[c_id].push_back(c2_id);
  real_children.push_back({});
  aux_children.push_back({});
  parent.push_back(c_id);
  cpids.push_back(c2);
  identifiers[c2] = c2_id;
  return c2;
}

CPid CPidSystem::new_aux(const CPid &c){
  int c_id = identifiers[c];
  CPid c2 = c.aux(aux_children[c_id].size());
  int c2_id = cpids.size();
  aux_children[c_id].push_back(c2_id);
  real_children.push_back({});
  aux_children.push_back({});
  parent.push_back(c_id);
  cpids.push_back(c2);
  identifiers[c2] = c2_id;
  return c2;
}
