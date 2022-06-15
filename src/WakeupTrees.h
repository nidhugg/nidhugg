/* Copyright (C) 2017 Magnus Lång
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

#ifndef __WAKEUP_TREES_H__
#define __WAKEUP_TREES_H__

#include "Debug.h"

#include <deque>
#include <map>
#include <memory>
#include <utility>
#include <vector>

/* Constraints on template argument type Branch:
 *  * Needs to define operator<(const Branch &), defining a total order.
 *  * Also needs to define operator==(const Branch &), of the same order.
 */

template <typename Branch> class WakeupTree;
template <typename Branch> class WakeupTreeRef;
template <typename Branch, typename Event> class WakeupTreeExplorationBuffer;

template <typename Branch>
class WakeupTree {
  friend class WakeupTreeRef<Branch>;
  template <typename Branch_, typename Event>
  friend class WakeupTreeExplorationBuffer;
private:
  typedef std::deque<std::pair<Branch,std::unique_ptr<WakeupTree>>> children_type;
  children_type children;
};

template <typename Branch>
class WakeupTreeRef {
  template <typename Branch_, typename Event>
  friend class WakeupTreeExplorationBuffer;
public:
  std::size_t size() const noexcept { return (*this)->children.size(); }

  WakeupTreeRef(const WakeupTreeRef &) = default;
  WakeupTreeRef &operator=(const WakeupTreeRef&) = default;

  /* Does not implement the full iterator API, for now. */
  class iterator {
  public:
    const Branch &branch() { return iter->first; }
    WakeupTreeRef<Branch> node() { return {*iter->second}; }
    bool operator<(const iterator &it) const { return iter < it.iter; }
    bool operator==(const iterator &it) const { return iter == it.iter; }
    bool operator>(const iterator &it) const { return iter > it.iter; }
    bool operator<=(const iterator &it) const { return iter <= it.iter; }
    bool operator!=(const iterator &it) const { return iter != it.iter; }
    bool operator>=(const iterator &it) const { return iter >= it.iter; }
    iterator operator++(){ iter++; return *this; }
    iterator(typename WakeupTree<Branch>::children_type::iterator iter)
      : iter(iter) {}
  private:
    typename WakeupTree<Branch>::children_type::iterator iter;
  };

  iterator begin() { return iterator((*this)->children.begin()); }
  iterator end()   { return iterator((*this)->children.end()); }

  WakeupTreeRef put_child(Branch b);
  bool has_child(const Branch &b) const;

private:
  const WakeupTree<Branch> *operator->(void) const { return node; }
  WakeupTree<Branch> *operator->(void) { return node; }
  WakeupTreeRef(WakeupTree<Branch> &node) : node(&node) {}
  WakeupTree<Branch> *node;
};

/* A WakupTreeExplorationBuffer<Branch, Event> associates a wakeup tree with an
 * *execution state*, which is a stack of *events* explored in the current
 * execution.
 */
template <typename Branch, typename Event>
class WakeupTreeExplorationBuffer {
private:
  struct ExplorationNode {
    Event event;
    WakeupTreeRef<Branch> node;
  };
  WakeupTree<Branch> tree;
  WakeupTreeRef<Branch> rootref;
  std::vector<ExplorationNode> prefix;

public:
  WakeupTreeExplorationBuffer() : rootref(tree) {}
  std::size_t len() const noexcept { return prefix.size(); }
  Event &operator[](std::size_t i) { assert(i < len()); return prefix[i].event; }
  const Event &operator[](std::size_t i) const { assert(i < len()); return prefix[i].event; }
  const Branch &branch(std::size_t i) const {
    return parent_at(i)->children.front().first;
  }
  Branch &branch(std::size_t i) {
    return parent_at(i)->children.front().first;
  }
  WakeupTreeRef<Branch> node(std::size_t i) { assert(i < len()); return prefix[i].node; }
  WakeupTreeRef<Branch> parent_at(std::size_t i) {
    assert(i <= len());
    if (i == 0) return rootref;
    return prefix[i-1].node;
  }
  const WakeupTreeRef<Branch> parent_at(std::size_t i) const {
    assert(i <= len());
    if (i == 0) return rootref;
    return prefix[i-1].node;
  }
  std::size_t children_after(std::size_t pos) const {
    assert(pos < len());
    return parent_at(pos).size() - 1;
  }
  Event &last() { return prefix.back().event; }
  const Branch &lastbranch() { return parent_at(len()-1)->children.front().first; }
  void set_last_branch(Branch b);
  WakeupTreeRef<Branch> lastnode() { return prefix.back().node; }
  void delete_last();
  const Branch &first_child() {
    assert(parent_at(len()).size());
    return parent_at(len()).begin().branch();
  }
  void enter_first_child(Event event);
  void push(Branch branch, Event event);
};

#include "WakeupTrees.tcc"

#endif // !defined(__WAKEUP_TREES_H__)
