/* Copyright (C) 2012-2017 Carl Leonardsson
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

template<class T, class Compare>
template<typename ITER>
VecSet<T, Compare>::VecSet(ITER begin, ITER end){
  for(; begin != end; ++begin){
    if(vec.size() && lt(vec.back(), *begin)){
      vec.push_back(*begin);
    }else{
      insert(*begin);
    }
  }
}

template<class T, class Compare>
VecSet<T,Compare>::VecSet(std::initializer_list<T> il){
  for(auto it = il.begin(); it != il.end(); ++it){
    if(vec.size() && lt(vec.back(), *it)){
      vec.push_back(*it);
    }else{
      insert(*it);
    }
  }
}

template<class T, class Compare>
VecSet<T,Compare>::VecSet(VecSet &&S)
  : vec(std::move(S.vec))
{
}

template<class T, class Compare>
VecSet<T,Compare> &VecSet<T,Compare>::operator=(VecSet &&S){
  if(this != &S){
    vec = std::move(S.vec);
  }
  return *this;
}

template<class T, class Compare>
int VecSet<T, Compare>::find_geq(const T &t) const{
  /* Use binary search */
  int a = 0;
  int b = vec.size();
  while(a < b){
    int c = (a+b)/2;
    if(lt(vec[c], t)){
      a = c+1;
    }else if(!lt(t, vec[c])){
      return c;
    }else{
      b = c;
    }
  }
  return a;
}

template<class T, class Compare>
int VecSet<T, Compare>::find(const T &t) const{
  int i = find_geq(t);
  if(i == int(vec.size()) || !(vec[i] == t)){
    return -1;
  }else{
    return i;
  }
}

template<class T, class Compare>
int VecSet<T, Compare>::count(const T &t) const{
  if(find(t) >= 0){
    return 1;
  }else{
    return 0;
  }
}

template<class T, class Compare>
std::pair<int,bool> VecSet<T,Compare>::insert(const T &t){
  int i = find_geq(t);
  if(i < int(vec.size()) && !lt(t, vec[i])){
    assert(!lt(vec[i], t)); /* vec[i] must compare equal to t */
    /* t is already in the set */
    return std::pair<int,bool>(i,false);
  }else{
    if(vec.size() == 0){
      vec.push_back(t);
    }else{
      vec.push_back(vec.back());
      for(int j = vec.size()-2; j > i; j--){
        vec[j] = vec[j-1];
      }
      vec[i] = t;
    }
    return std::pair<int,bool>(i,true);
  }
}

template<class T, class Compare>
std::pair<int,bool> VecSet<T,Compare>::insert_geq(const T &t){
  int i = vec.size();
  assert(!i || !lt(t, vec.back()));
  if(i && vec.back() == t){
    /* t is already in the set */
    return std::pair<int,bool>(i-1,false);
  }else{
    vec.push_back(t);
    return std::pair<int,bool>(i,true);
  }
}

template<class T, class Compare>
void VecSet<T,Compare>::insert_gt(const T &t){
  assert(vec.size() == 0 || lt(vec.back(), t));
  vec.push_back(t);
}

template<class T, class Compare>
int VecSet<T,Compare>::insert(const VecSet &s){
  if(s.size() == 0){
    return 0;
  }else if(s.size() == 1){
    return (insert(s[0]).second ? 1 : 0);
  }else{
    /* Count the number of elements that are in s but not in this set */
    int count = 0;

    int a = 0; // pointer into this set
    int b = 0; // pointer into s
    while(a < size() && b < s.size()){
      if(lt(vec[a], s.vec[b])){
        ++a;
      }else if(!lt(s.vec[b], vec[a])){
        ++a;
        ++b;
      }else{
        ++count;
        ++b;
      }
    }
    if(b < s.size()){
      assert(a == size());
      count += s.size() - b;
    }

    if(count > 0){
      a = int(vec.size())-1;
      b = int(s.vec.size())-1;
      vec.resize(vec.size() + count,s.vec[0]);
      /* Insert one-by-one the largest remaining element from this union s */
      for(int i = int(vec.size())-1; i >= 0; i--){
        assert(i >= a);
        if(a < 0 || (b >= 0 && lt(vec[a], s.vec[b]))){
          vec[i] = s.vec[b];
          --b;
        }else if(a >= 0 && b >= 0 && !lt(s.vec[b], vec[a])){
          vec[i] = vec[a];
          --a;
          --b;
        }else{
          assert(a >= 0);
          assert(b < 0 || lt(s.vec[b], vec[a]));
          vec[i] = vec[a];
          --a;
        }
      }
      assert(a == -1 && b == -1);
    }

    return count;
  }
}

template<class T, class Compare>
int VecSet<T,Compare>::erase(const T &t){
  int i = find_geq(t);
  if(int(vec.size()) <= i || lt(t, vec[i])) return 0;

  erase_at(i);

  return 1;
}

template<class T, class Compare>
void VecSet<T,Compare>::erase_at(int p){
  for(int j = p; j < int(vec.size())-1; ++j){
    vec[j] = vec[j+1];
  }
  vec.resize(vec.size()-1,vec[0]);
}

template<class T, class Compare>
int VecSet<T,Compare>::erase(const VecSet &S){
  if(vec.empty()) return 0;
  if(S.vec.empty()) return 0;
  int a = 0;
  int b = 0;
  int ains = 0;
  int erase_count = 0;
  while(a < size() && b < S.size()){
    if(lt(vec[a], S.vec[b])){
      if(ains != a) vec[ains] = vec[a];
      ++a;
      ++ains;
    }else if(lt(S.vec[b], vec[a])){
      ++b;
    }else{
      ++a;
      ++b;
      ++erase_count;
    }
  }
  if(ains != a){
    while(a < size()){
      vec[ains] = vec[a];
      ++a;
      ++ains;
    }
    vec.resize(ains,vec[0]);
  }
  return erase_count;
}

template<class T, class Compare>
bool VecSet<T,Compare>::check_invariant() const{
  for(unsigned i = 1; i < vec.size(); ++i){
    if(!(lt(vec[i-1], vec[i]))){
      return false;
    }
  }
  return true;
}

template<class T, class Compare>
std::string VecSet<T,Compare>::to_string_one_line(std::function<std::string(const T&)> f) const{
  std::string s = "{";
  for(unsigned i = 0; i < vec.size(); ++i){
    if(i != 0) s += ", ";
    s += f(vec[i]);
  }
  s += "}";
  return s;
}

template<class T, class Compare>
bool VecSet<T,Compare>::subset_of(const VecSet &s) const{
  if(vec.size() > s.vec.size()){
    return false;
  }
  int a = 0; // pointer into vec
  int b = 0; // pointer into s.vec
  while(a < int(vec.size())){
    if((int(vec.size()) - a) > (int(s.vec.size()) - b)){
      return false;
    }
    if(lt(vec[a], s.vec[b])){
      return false;
    }else if(!lt(s.vec[b], vec[a])){
      ++a;
      ++b;
    }else{
      ++b;
    }
  }
  return true;
}

template<class T, class Compare>
bool VecSet<T,Compare>::intersects(const VecSet &s) const{
  int a = 0; // pointer into vec
  int b = 0; // pointer into s.vec
  while(a < int(vec.size()) && b < int(s.vec.size())){
    if(lt(vec[a], s.vec[b])){
      ++a;
    }else if(!lt(s.vec[b], vec[a])){
      return true;
    }else{
      ++b;
    }
  }
  return false;
}
