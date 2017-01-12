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

#include "VClock.h"

VClock<int>::VClock(){}

VClock<int>::VClock(const std::vector<int> &v) : vec(v) {
#ifndef NDEBUG
  for(unsigned i = 0; i < vec.size(); ++i){
    assert(vec[i] >= 0);
  }
#endif
}

VClock<int>::VClock(const VClock &vc) : vec(vc.vec) {}

VClock<int>::VClock(const std::initializer_list<int> &il) : vec(il) {
#ifndef NDEBUG
  for(unsigned i = 0; i < vec.size(); ++i){
    assert(vec[i] >= 0);
  }
#endif
}

VClock<int>::~VClock(){}

VClock<int> &VClock<int>::operator=(const VClock<int> &vc){
  vec = vc.vec;
  return *this;
}

VClock<int> VClock<int>::operator+(const VClock<int> &vc) const{
  VClock<int> vc2;
  vc2.vec.resize(std::max(vec.size(),vc.vec.size()));
  for(unsigned i = 0; i < vc2.vec.size(); ++i){
    vc2.vec[i] = std::max((*this)[i],vc[i]);
  }
  return vc2;
}

VClock<int> &VClock<int>::operator+=(const VClock<int> &vc){
  const unsigned sz = vc.vec.size();
  if(vec.size() < sz){
    vec.resize(vc.vec.size(),0);
  }
  for(unsigned i = 0; i < sz; ++i){
    if(vc.vec[i] > vec[i]){
      vec[i] = vc.vec[i];
    }
  }
  return *this;
}

int VClock<int>::operator[](int i) const{
  assert(i >= 0);
  if(i < int(vec.size())){
    return vec[i];
  }else{
    return 0;
  }
}

int &VClock<int>::operator[](int i){
  assert(i >= 0);
  if(i >= int(vec.size())){
    vec.resize(i+1,0);
  }
  return vec[i];
}

bool VClock<int>::operator==(const VClock<int> &vc) const{
  int m = std::max(vec.size(),vc.vec.size());
  for(int i = 0; i < m; ++i){
    if((*this)[i] != vc[i]) return false;
  }
  return true;
}

bool VClock<int>::operator!=(const VClock<int> &vc) const{
  return !(*this == vc);
}

bool VClock<int>::operator<(const VClock<int> &vc) const{
  int m = std::max(vec.size(),vc.vec.size());
  for(int i = 0; i < m; ++i){
    if((*this)[i] < vc[i]) return true;
    if((*this)[i] > vc[i]) return false;
  }
  return false;
}

bool VClock<int>::operator<=(const VClock<int> &vc) const{
  return !(vc < *this);
}

bool VClock<int>::operator>(const VClock<int> &vc) const{
  return vc < *this;
}

bool VClock<int>::operator>=(const VClock<int> &vc) const{
  return !(*this < vc);
}

bool VClock<int>::lt(const VClock<int> &vc) const{
  int m = std::max(vec.size(),vc.vec.size());
  bool less = false;
  for(int i = 0; i < m; ++i){
    if((*this)[i] > vc[i]) return false;
    less = less || ((*this)[i] < vc[i]);
  }
  return less;
}

bool VClock<int>::leq(const VClock<int> &vc) const{
  unsigned m = std::min(vec.size(),vc.vec.size());
  unsigned i;
  for(i = 0; i < m; ++i){
    if(vc.vec[i] < vec[i]) return false;
  }
  m = vec.size();
  for(; i < m; ++i){
    if(vec[i]) return false;
  }
  return true;
}

bool VClock<int>::geq(const VClock<int> &vc) const{
  int m = std::max(vec.size(),vc.vec.size());
  for(int i = 0; i < m; ++i){
    if((*this)[i] < vc[i]) return false;
  }
  return true;
}

bool VClock<int>::gt(const VClock<int> &vc) const{
  int m = std::max(vec.size(),vc.vec.size());
  bool greater = false;
  for(int i = 0; i < m; ++i){
    if((*this)[i] < vc[i]) return false;
    greater = greater || ((*this)[i] > vc[i]);
  }
  return greater;
}

std::string VClock<int>::to_string() const{
  int m = vec.size() - 1;
  while(0 <= m && vec[m] == 0) --m;
  ++m;
  std::stringstream ss;
  ss << "[";
  for(int i = 0; i < m; ++i){
    if(i != 0) ss << ", ";
    ss << vec[i];
  }
  ss << "]";
  return ss.str();
}
