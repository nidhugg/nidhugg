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

template <typename Branch, typename Event>
void WakeupTreeExplorationBuffer<Branch,Event>::put_branch
(std::size_t pos, Branch b) {
  assert(pos < len());
  assert(children_at(pos).count(b) == 0);
  assert(!has_branch(pos, b));
  children_at(pos).emplace(std::move(b), new WakeupTree<Branch>());
}

template <typename Branch, typename Event>
bool WakeupTreeExplorationBuffer<Branch,Event>::has_branch
(std::size_t pos, const Branch &b) const {
  assert(pos < len());
  return children_at(pos).count(b);
}

template <typename Branch, typename Event>
void WakeupTreeExplorationBuffer<Branch,Event>::delete_last() {
  assert(len());
  typename WakeupTree<Branch>::children_type &child_list = children_at(len()-1);
  assert(child_list.size());
  child_list.erase(child_list.find(lastbranch()));
  prefix.pop_back();
}

template <typename Branch, typename Event>
void WakeupTreeExplorationBuffer<Branch,Event>::enter_first_child(Event event) {
  assert(children_at(len()).size());
  const Branch     &branch =  children_at(len()).begin()->first;
  WakeupTree<Branch> &node = *children_at(len()).begin()->second;
  prefix.push_back({std::move(event), branch, WakeupTreeRef<Branch>(node)});
}

template <typename Branch, typename Event>
void WakeupTreeExplorationBuffer<Branch,Event>::push
(Branch branch, Event event) {
  WakeupTree<Branch> &node = parent_at(len());
  assert(node.size() == 0);
  WakeupTree<Branch> &new_node =
    *node.children.emplace(branch,
                           new WakeupTree<Branch>()).first->second;
  prefix.push_back({std::move(event), std::move(branch),
        WakeupTreeRef<Branch>(new_node)});
}
