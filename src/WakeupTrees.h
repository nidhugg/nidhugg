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

#include <memory>
#include <vector>
#include <map>
#include "Debug.h"

/* Constraints on template argument type Branch:
 *  * Needs to define operator<(const Branch &), defining a total order.
 *  * Also needs to define operator==(const Branch &), of the same order.
 */

template <typename Branch> class WakeupTree;
template <typename Branch, typename Event> class WakeupTreeExplorationBuffer;

template <typename Branch>
class WakeupTreeRef {
  friend class WakeupTree<Branch>;
  template <typename Branch_, typename Event>
  friend class WakeupTreeExplorationBuffer;
public:
  const WakeupTree<Branch> &operator *(void) const { return node; }
  WakeupTree<Branch> &operator *(void) { return node; }
  const WakeupTree<Branch> *operator->(void) const { return &node; }
  WakeupTree<Branch> *operator->(void) { return &node; }
  std::size_t size() const noexcept { return node.children.size(); }

private:
  WakeupTreeRef(WakeupTree<Branch> &node) : node(node) {}
  WakeupTree<Branch> &node;
};

template <typename Branch>
class WakeupTree {
  template <typename Branch_, typename Event>
  friend class WakeupTreeExplorationBuffer;
private:
  typedef std::map<Branch,std::unique_ptr<WakeupTree>> children_type;
  children_type children;
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
    const Branch branch;
    WakeupTreeRef<Branch> node;
  };
  WakeupTree<Branch> tree;
  std::vector<ExplorationNode> prefix;
  typename WakeupTree<Branch>::children_type &children_at(std::size_t pos) {
    if (pos == 0) return tree.children;
    return prefix[pos-1].node->children;
  }
  const typename WakeupTree<Branch>::children_type &children_at(std::size_t pos)
    const {
    if (pos == 0) return tree.children;
    return prefix[pos-1].node->children;
  }
  WakeupTree<Branch> &parent_at(std::size_t pos) {
    assert(pos <= len());
    if (pos == 0) return tree;
    return *prefix[pos-1].node;
  }
  const WakeupTree<Branch> &parent_at(std::size_t pos) const {
    assert(pos <= len());
    if (pos == 0) return tree;
    return *prefix[pos-1].node;
  }
public:
  std::size_t len() const noexcept { return prefix.size(); }
  Event &operator[](std::size_t i) { return prefix[i].event; }
  const Event &operator[](std::size_t i) const { return prefix[i].event; }
  const Branch &branch(std::size_t i) const { return prefix[i].branch; }
  std::size_t children_after(std::size_t pos) const {
    assert(pos < len());
    return children_at(pos).size() - 1;
  }
  Event &last() { return prefix.back().event; }
  const Branch &lastbranch() { return prefix.back().branch; }
  WakeupTreeRef<Branch> lastnode() { return prefix.back().node; }
  void put_branch(std::size_t pos, Branch b);
  bool has_branch(std::size_t pos, const Branch &b) const;
  void delete_last();
  const Branch &first_child() {
    assert(children_at(len()).size());
    return children_at(len()).begin()->first;
  }
  void enter_first_child(Event event);
  void push(Branch branch, Event event);
};

#include "WakeupTrees.tcc"

#endif // !defined(__WAKEUP_TREES_H__)
