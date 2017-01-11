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

#include "BVClock.h"
#include "Debug.h"
#include "FBVClock.h"

#include <algorithm>

BVClock BVClock::operator+(const BVClock &vc) const{
  Debug::warn("BVClock")
    << "WARNING: BVClock can be implemented more efficiently with bitwise operations.\n";
  BVClock rv;
  if(vec.size() < vc.vec.size()){
    rv.vec = vc.vec;
  }else{
    rv.vec = vec;
  }
  const unsigned m = std::min(vc.vec.size(),vec.size());
  for(unsigned i = 0; i < m; ++i){
    rv.vec[i] = vec[i] || vc.vec[i];
  }
  return rv;
}

std::string BVClock::to_string() const{
  if(vec.size()){
    std::string s;
    s.resize(2*vec.size()+1,',');
    s[0] = '[';
    s[s.size()-1] = ']';
    for(unsigned i = 0; i < vec.size(); ++i){
      s[2*i+1] = vec[i] ? '1' : '0';
    }
    return s;
  }
  return "[]";
}

bool BVClock::leq(const BVClock &vc) const{
  const int m = std::min(vec.size(),vc.vec.size());
  for(int i = 0; i < m; ++i){
    if(vec[i] && !vc.vec[i]) return false;
  }
  for(unsigned i = vc.vec.size(); i < vec.size(); ++i){
    if(vec[i]) return false;
  }
  return true;
}

BVClock &BVClock::operator=(FBVClock &vc){
  const int sz = vc.size();
  vec.resize(sz,false);
  for(int i = 0; i < sz; ++i){
    vec[i] = vc[i];
  }
  return *this;
}

BVClock &BVClock::operator+=(FBVClock &vc){
  const unsigned sz = vc.size();
  if(vec.size() < sz){
    vec.resize(sz,false);
  }
  for(unsigned i = 0; i < sz; ++i){
    vec[i] = vec[i] || vc[i];
  }
  return *this;
}
