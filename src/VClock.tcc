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

#include <algorithm>
#include <cassert>
#include <sstream>

template<typename DOM>
VClock<DOM>::VClock(){}

template<typename DOM>
VClock<DOM>::VClock(const std::map<DOM,int> &m) : clocks(m) {
#ifndef NDEBUG
  for(auto it = clocks.begin(); it != clocks.end(); ++it){
    assert(it->second >= 0);
  }
#endif
}

template<typename DOM>
VClock<DOM>::VClock(const VClock<DOM> &vc) : clocks(vc.clocks) {}

template<typename DOM>
VClock<DOM>::VClock(const std::initializer_list<std::pair<DOM,int> > &il) {
  for(auto it = il.begin(); it != il.end(); ++it){
    assert(it->second >= 0);
    if(it->second != 0){
      clocks[it->first] = it->second;
    }
  }
}

template<typename DOM>
VClock<DOM>::VClock(const VClock<int> &vc, const std::vector<DOM> &t){
  for(unsigned i = 0; i < t.size(); ++i){
    assert(vc[i] >= 0);
    if(vc[i] != 0){
      clocks[t[i]] = vc[i];
    }
  }
}

template<typename DOM>
VClock<DOM>::~VClock(){}

template<typename DOM>
int VClock<DOM>::get_nonzero_count() const{
  int c = 0;
  for(auto it = clocks.begin(); it != clocks.end(); ++it){
    if(it->second > 0){
      ++c;
    }
  }
  return c;
}

template<typename DOM>
VClock<DOM> &VClock<DOM>::operator=(const VClock<DOM> &vc){
  clocks = vc.clocks;
  return *this;
}

template<typename DOM>
VClock<DOM> VClock<DOM>::operator+(const VClock<DOM> &vc) const{
  VClock<DOM> vc2(*this);
  for(auto it = vc.clocks.begin(); it != vc.clocks.end(); ++it){
    vc2[it->first] = std::max(vc2[it->first],it->second);
  }
  return vc2;
}

template<typename DOM>
VClock<DOM> &VClock<DOM>::operator+=(const VClock<DOM> &vc){
  *this = *this + vc;
  return *this;
}

template<typename DOM>
int VClock<DOM>::operator[](const DOM &c) const{
  auto it = clocks.find(c);
  if(it == clocks.end()){
    return 0;
  }else{
    return it->second;
  }
}

template<typename DOM>
int &VClock<DOM>::operator[](const DOM &c){
  return clocks[c];
}

template<typename DOM>
bool VClock<DOM>::operator==(const VClock<DOM> &vc) const{
  auto a_it = clocks.begin();
  auto b_it = vc.clocks.begin();
  while(a_it != clocks.end() || b_it != vc.clocks.end()){
    if(a_it == clocks.end() ||
       (b_it != vc.clocks.end() && b_it->first < a_it->first)){
      if(b_it->second != 0) return false;
      ++b_it;
    }else if(b_it == vc.clocks.end() ||
             (a_it != clocks.end() && a_it->first < b_it->first)){
      if(a_it->second != 0) return false;
      ++a_it;
    }else{
      assert(a_it != clocks.end() && b_it != vc.clocks.end());
      assert(a_it->first == b_it->first);
      if(a_it->second != b_it->second) return false;
      ++a_it;
      ++b_it;
    }
  }
  return true;
}

template<typename DOM>
bool VClock<DOM>::operator!=(const VClock<DOM> &vc) const{
  return !(*this == vc);
}

template<typename DOM>
bool VClock<DOM>::operator<(const VClock<DOM> &vc) const{
  /* Lexicographic order, ignoring 0-clocks */
  auto a_it = clocks.begin();
  auto b_it = vc.clocks.begin();
  while(a_it != clocks.end() || b_it != vc.clocks.end()){
    while(a_it != clocks.end() && a_it->second == 0) ++a_it;
    while(b_it != vc.clocks.end() && b_it->second == 0) ++b_it;
    if(a_it == clocks.end()){
      return b_it != vc.clocks.end();
    }
    if(b_it == vc.clocks.end()){
      return false;
    }
    if(a_it->first < b_it->first) return true;
    if(a_it->first > b_it->first) return false;
    if(a_it->second < b_it->second) return true;
    if(a_it->second > b_it->second) return false;
    ++a_it;
    ++b_it;
  }
  return false;
}

template<typename DOM>
bool VClock<DOM>::operator<=(const VClock<DOM> &vc) const{
  /* Lexicographic order, ignoring 0-clocks */
  auto a_it = clocks.begin();
  auto b_it = vc.clocks.begin();
  while(a_it != clocks.end() || b_it != vc.clocks.end()){
    while(a_it != clocks.end() && a_it->second == 0) ++a_it;
    while(b_it != vc.clocks.end() && b_it->second == 0) ++b_it;
    if(a_it == clocks.end()){
      return true;
    }
    if(b_it == vc.clocks.end()){
      return false;
    }
    if(a_it->first < b_it->first) return true;
    if(a_it->first > b_it->first) return false;
    if(a_it->second < b_it->second) return true;
    if(a_it->second > b_it->second) return false;
    ++a_it;
    ++b_it;
  }
  return true;
}

template<typename DOM>
bool VClock<DOM>::operator>(const VClock<DOM> &vc) const{
  return vc < *this;
}

template<typename DOM>
bool VClock<DOM>::operator>=(const VClock<DOM> &vc) const{
  return vc <= *this;
}

template<typename DOM>
bool VClock<DOM>::lt(const VClock<DOM> &vc) const{
  auto a_it = clocks.begin();
  auto b_it = vc.clocks.begin();
  bool strict = false;
  while(a_it != clocks.end() || b_it != vc.clocks.end()){
    if(a_it == clocks.end() ||
       (b_it != vc.clocks.end() && b_it->first < a_it->first)){
      if(b_it->second > 0) strict = true;
      ++b_it;
    }else if(b_it == vc.clocks.end() ||
             (a_it != clocks.end() && a_it->first < b_it->first)){
      if(a_it->second > 0) return false;
      ++a_it;
    }else{
      assert(a_it != clocks.end() && b_it != vc.clocks.end());
      assert(a_it->first == b_it->first);
      if(a_it->second > b_it->second) return false;
      if(a_it->second < b_it->second) strict = true;
      ++a_it;
      ++b_it;
    }
  }
  return strict;
}

template<typename DOM>
bool VClock<DOM>::leq(const VClock<DOM> &vc) const{
  auto a_it = clocks.begin();
  auto b_it = vc.clocks.begin();
  while(a_it != clocks.end() || b_it != vc.clocks.end()){
    if(a_it == clocks.end() ||
       (b_it != vc.clocks.end() && b_it->first < a_it->first)){
      ++b_it;
    }else if(b_it == vc.clocks.end() ||
             (a_it != clocks.end() && a_it->first < b_it->first)){
      if(a_it->second > 0) return false;
      ++a_it;
    }else{
      assert(a_it != clocks.end() && b_it != vc.clocks.end());
      assert(a_it->first == b_it->first);
      if(a_it->second > b_it->second) return false;
      ++a_it;
      ++b_it;
    }
  }
  return true;
}

template<typename DOM>
bool VClock<DOM>::gt(const VClock<DOM> &vc) const{
  return vc.lt(*this);
}

template<typename DOM>
bool VClock<DOM>::geq(const VClock<DOM> &vc) const{
  return vc.leq(*this);
}

template<typename DOM>
std::string VClock<DOM>::to_string() const{
  std::stringstream ss;
  ss << "[";
  bool first = true;
  for(auto it = clocks.begin(); it != clocks.end(); ++it){
    if(it->second != 0){
      if(!first) ss << ", ";
      first = false;
      ss << it->first << ":" << it->second;
    }
  }
  ss << "]";
  return ss.str();
}
