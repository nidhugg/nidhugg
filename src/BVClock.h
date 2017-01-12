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

#include <config.h>

#ifndef __BVCLOCK_H__
#define __BVCLOCK_H__

#include <ostream>
#include <string>
#include <vector>

#include <llvm/Support/raw_ostream.h>

class FBVClock;

/* A BVClock is a vector clock (similar to VClock<int>) where each
 * clock element is just a single bit.
 */
class BVClock{
public:
  /* Create a vector clock where each clock is initialized to 0. */
  BVClock(){};
  BVClock(const BVClock &vc) : vec(vc.vec) {};
  BVClock(BVClock &&vc) : vec(std::move(vc.vec)) {};
  BVClock &operator=(const BVClock &vc) { vec = vc.vec; return *this; };
  BVClock &operator=(BVClock &&vc) { vec = std::move(vc.vec); return *this; };
  BVClock &operator=(FBVClock &vc);
  /* A vector clock v such that the clock of d in v takes the value
   * max((*this)[d],vc[d]) for all d.
   */
  BVClock operator+(const BVClock &vc) const;
  /* Assign this vector clock to (*this + vc). */
  BVClock &operator+=(const BVClock &vc){
    if(vec.size() < vc.vec.size()){
      vec.resize(vc.vec.size(),false);
    }
    for(unsigned i = 0; i < vc.vec.size(); ++i){
      vec[i] = vec[i] || vc.vec[i];
    }
    return *this;
  };
  /* Assign this vector clock to (*this + vc). */
  BVClock &operator+=(BVClock &&vc);
  /* Assign this vector clock to (*this + vc). */
  BVClock &operator+=(FBVClock &vc);
  /* The value of the clock of d. */
  bool operator[](int d) const{
    if(d < int(vec.size())) return vec[d];
    return false;
  };
  void set(int d) {
    if(d >= int(vec.size())){
      vec.resize(d+1,false);
    }
    vec[d] = true;
  };
  /* Assign 0 to all clocks */
  void clear() { vec.clear(); };

  /* *** Partial order comparisons ***
   *
   * A vector clock u is considered strictly less than a vector clock
   * v iff for all d in DOM, it holds that u[d] <= v[d], and there is
   * at least one d such that u[d] < v[d].
   */
  bool lt(const BVClock &vc) const;
  bool leq(const BVClock &vc) const;
  bool gt(const BVClock &vc) const;
  bool geq(const BVClock &vc) const;

  /* *** Total order comparisons *** */
  bool operator==(const BVClock &vc) const;
  bool operator!=(const BVClock &vc) const;
  bool operator<(const BVClock &vc) const;
  bool operator<=(const BVClock &vc) const;
  bool operator>(const BVClock &vc) const;
  bool operator>=(const BVClock &vc) const;

  std::string to_string() const;
private:
  std::vector<bool> vec;
};

inline std::ostream &operator<<(std::ostream &os, const BVClock &vc){
  return os << vc.to_string();
}

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const BVClock &vc){
  return os << vc.to_string();
}

#endif
