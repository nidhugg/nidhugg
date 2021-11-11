/* Copyright (C) 2012-2017 Carl Leonardsson, 2021 Magnus Lång
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

#ifndef __VECSET_H__
#define __VECSET_H__

#include <cassert>
#include <functional>
#include <initializer_list>
#include <string>
#include <vector>

/* A set, implemented as a sorted vector */
template<class T, class Compare = std::less<T>> class VecSet{
public:
  /* An empty set */
  VecSet() {};
  /* A set consisting of the values in v.
   *
   * Pre: v is sorted and distinct.
   */
  VecSet(const std::vector<T> &v)
    : vec(v) {
    assert(check_invariant());
  };
  /* A set consisting of the values in v.
   *
   * Pre: v is sorted and distinct.
   */
  VecSet(std::vector<T> &&v)
    : vec(std::move(v)) {
    assert(check_invariant());
  };
  /* A set consisting of the values of [begin,end). Each element is
   * inserted using a separate call to insert.
   */
  template<typename ITER>
  VecSet(ITER begin, ITER end);
  VecSet(std::initializer_list<T> il);
  VecSet(const VecSet &) = default;
  VecSet(VecSet &&);
  VecSet &operator=(const VecSet&) = default;
  VecSet &operator=(VecSet&&);
  virtual ~VecSet() {};
  /* Returns a set which is the singleton {t}. */
  static VecSet singleton(const T &t){
    VecSet vs;
    vs.vec.push_back(t);
    return vs;
  };
  /* Reserve space in the vector for at least n elements. */
  void reserve(int n) { vec.reserve(n); };
  /* Insert t into this set.
   *
   * Return (i,b) where i is the index of t in the vector and b is
   * true iff t was not previously in this set. */
  std::pair<int,bool> insert(const T &t);
  /* Insert t into this set under the assumption that t is >= all
   * elements in this set.
   *
   * Return (i,b) where i is the index of t in the vector and b is
   * true iff t was not previously in this set. */
  std::pair<int,bool> insert_geq(const T &t);
  /* Insert t into this set under the assumption that t is > all
   * elements in this set. */
  void insert_gt(const T &t);
  /* Sets this set to the union of this set and s.
   *
   * Return the number of elements that were inserted into this set
   * and was not already in this set.
   */
  int insert(const VecSet &s);
  /* Erase the value t from the set.
   *
   * Return the number of elements that were erased (0 or 1).
   */
  int erase(const T &t);
  /* Erase the value at position p from the set.
   *
   * Invalidates all iterators, returns a new iterator to the element
   * after p, or the end iterator if there is no such element.
   */
  void erase_at(int p);
  /* Erase all elements of S from this set.
   *
   * Return the number of elements that were erased.
   */
  int erase(const VecSet &S);
  /* Return 1 if t is in this set, 0 otherwise. */
  int count(const T &t) const;
  /* Return the index of t in the vector.
   * Return -1 if t is not in this set.
   */
  int find(const T &t) const;
  /* Return the number of elements in this set. */
  int size() const { return vec.size(); };
  bool empty() const { return vec.empty();};
  /* Empties this set */
  void clear() { vec.clear(); };
  /* Returns true iff all elements in this set are also members of s. */
  bool subset_of(const VecSet &s) const;
  /* Returns true iff there is some element that occurs both in this set and in s. */
  bool intersects(const VecSet &s) const;
  /* Returns the i:th smallest element in the set. */
  const T &operator[](int i) const { return vec[i]; };
  /* Returns the largest element in the set. */
  const T &back() const { return vec.back(); };
  /* Removes the largest element from the set.
   *
   * Pre: the set is non-empty
   */
  void pop_back() { vec.pop_back(); };
  typedef typename std::vector<T>::const_iterator const_iterator;
  const_iterator begin() const { return vec.cbegin(); };
  const_iterator end() const { return vec.cend(); };
  const std::vector<T> &get_vector() const { return vec; };
  // std::vector<T> &&get_vector() && { return vec; };
  bool operator==(const VecSet &s) const { return vec == s.vec; };
  bool operator<(const VecSet &s) const { return vec < s.vec; };
  bool operator>(const VecSet &s) const { return vec > s.vec; };
  bool operator<=(const VecSet &s) const { return vec <= s.vec; };
  bool operator!=(const VecSet &s) const { return vec != s.vec; };
  bool operator>=(const VecSet &s) const { return vec >= s.vec; };
  /* Produces a string representation of the set with each element t
   * represented as f(t) without any new lines between the elements.
   */
  std::string to_string_one_line(std::function<std::string(const T&)> f) const;
private:
  /* The set consists of the elements in vec.
   *
   * Invariant: vec is sorted and distinct.
   */
  std::vector<T> vec;
  /* Comparer */
  Compare lt = {};
  /* Return the index of the least element in the set which is greater than or equal to t.
   * Return vec.size() if there is no such element in the set.
   */
  int find_geq(const T &t) const;
  /* Returns true iff vec is sorted and distinct.
   *
   * (Used for debugging.) */
  bool check_invariant() const;
};

#include "vecset.tcc"

#endif
