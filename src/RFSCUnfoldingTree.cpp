/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
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


unsigned RFSCUnfoldingTree::unf_ctr = 0;

std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> RFSCUnfoldingTree::
find_unfolding_node(UnfoldingNodeChildren &parent_list,
                    const std::shared_ptr<UnfoldingNode> &parent,
                    const std::shared_ptr<UnfoldingNode> &read_from) {
  std::lock_guard<std::mutex> lock(this->unfolding_tree_mutex);
  for (unsigned ci = 0; ci < parent_list.size();) {
    std::shared_ptr<UnfoldingNode> c = parent_list[ci].lock();
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
  std::shared_ptr<UnfoldingNode> c =
    std::make_shared<UnfoldingNode>(parent, read_from);
  parent_list.push_back(c);
  return c;
}

RFSCUnfoldingTree::UnfoldingNodeChildren *RFSCUnfoldingTree::first_event_parentlist(CPid cpid) {
  std::lock_guard<std::mutex> lock(this->unfolding_tree_mutex);
  return &first_events[cpid];
}
