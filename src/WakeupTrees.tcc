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

template <typename Branch>
WakeupTreeRef<Branch> WakeupTreeRef<Branch>::put_child(Branch b) {
  auto pair = node->children.emplace(std::move(b), new WakeupTree<Branch>());
  assert(pair.second);
  return WakeupTreeRef(*pair.first->second);
}

template <typename Branch>
bool WakeupTreeRef<Branch>::has_child(const Branch &b) const {
  return node->children.count(b);
}

template <typename Branch, typename Event>
void WakeupTreeExplorationBuffer<Branch,Event>::delete_last() {
  assert(len());
  typename WakeupTree<Branch>::children_type &child_list
    = parent_at(len()-1)->children;
  assert(child_list.size());
  child_list.erase(child_list.find(lastbranch()));
  prefix.pop_back();
}

template <typename Branch, typename Event>
void WakeupTreeExplorationBuffer<Branch,Event>::enter_first_child(Event event) {
  assert(parent_at(len()).size());
  const Branch       &branch = parent_at(len()).begin().branch();
  WakeupTreeRef<Branch> node = parent_at(len()).begin().node();
  prefix.push_back({std::move(event), branch, node});
}

template <typename Branch, typename Event>
void WakeupTreeExplorationBuffer<Branch,Event>::push
(Branch branch, Event event) {
  WakeupTreeRef<Branch> node = parent_at(len());
  assert(node.size() == 0);
  WakeupTreeRef<Branch> new_node = node.put_child(branch);
  prefix.push_back({std::move(event), std::move(branch),
        WakeupTreeRef<Branch>(new_node)});
}

template <typename Branch, typename Event>
void WakeupTreeExplorationBuffer<Branch,Event>::set_last_branch(Branch b){
  WakeupTreeRef<Branch> par = parent_at(len()-1);
  assert(par.size() == 1 || b == lastbranch());
  auto iter = par->children.find(prefix.back().branch);
  prefix.back().branch = b;
  std::unique_ptr<WakeupTree<Branch>> node = std::move(iter->second);
  par->children.erase(iter);
  auto pair = par->children.emplace(std::move(b), std::move(node));
  assert(pair.second);
  prefix.back().node = WakeupTreeRef<Branch>(*pair.first->second);
}
