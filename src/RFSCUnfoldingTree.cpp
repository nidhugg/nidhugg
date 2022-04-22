/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo,
 * Copyright (C) 2019 Alexis Remmers and Nodari Kankava
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

#include "Debug.h"
#include "RFSCUnfoldingTree.h"

SeqnoRoot RFSCUnfoldingTree::unf_ctr_root{};
thread_local Seqno RFSCUnfoldingTree::unf_ctr{unf_ctr_root};

RFSCUnfoldingTree::NodePtr RFSCUnfoldingTree::
get_or_create(UnfoldingNodeChildren &parent_list,
              const NodePtr &parent, const NodePtr &read_from) {
  for (unsigned ci = 0; ci < parent_list.size();) {
    NodePtr c = parent_list[ci].lock();
    if (!c) {
      /* Delete the null element and continue */
      std::swap(parent_list[ci], parent_list.back());
      parent_list.pop_back();
      continue;
    }

    if (c->read_from == read_from) {
      assert(parent == c->parent);
      return c;
    }
    ++ci;
  }

  /* Did not exist, create it. */
  NodePtr c = std::make_shared<UnfoldingNode>(parent, read_from);
  parent_list.push_back(c);
  return c;
}

RFSCUnfoldingTree::NodePtr RFSCUnfoldingTree::
find_unfolding_node(const CPid &cpid,
                    const NodePtr &parent,
                    const NodePtr &read_from) {
  if (parent) {
    std::lock_guard<std::mutex> lock(parent->mutex);
    return get_or_create(parent->children, parent, read_from);
  } else {
    UnfoldingRoot &root = get_unfolding_root(cpid);
    std::lock_guard<std::mutex> lock(root.mutex);
    return get_or_create(root.children, parent, read_from);
  }
}


auto RFSCUnfoldingTree::get_unfolding_root(const CPid &cpid) -> UnfoldingRoot& {
  {
    std::shared_lock<std::shared_timed_mutex> rlock(unfolding_tree_mutex);
    auto it = first_events.find(cpid);
    if (it != first_events.end()) {
      return it->second;
    }
  }
  std::lock_guard<std::shared_timed_mutex> wlock(unfolding_tree_mutex);
  return first_events[cpid];
}
