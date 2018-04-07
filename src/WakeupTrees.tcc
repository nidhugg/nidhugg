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
  assert(std::none_of
         (node->children.begin(), node->children.end(),
          [&b](std::pair<Branch,std::unique_ptr<WakeupTree<Branch>>> &e) {
           return e.first == b;
         }));
  node->children.emplace_back
    (std::move(b),
     std::unique_ptr<WakeupTree<Branch>>(new WakeupTree<Branch>()));
  return WakeupTreeRef(*node->children.back().second);
}

template <typename Branch>
bool WakeupTreeRef<Branch>::has_child(const Branch &b) const {
  for (const auto &p : node->children) if (p.first == b) return true;
  return false;
}

template <typename Branch, typename Event>
void WakeupTreeExplorationBuffer<Branch,Event>::delete_last() {
  assert(len());
  typename WakeupTree<Branch>::children_type &child_list
    = parent_at(len()-1)->children;
  assert(child_list.size());
  assert(child_list.front().first == lastbranch());
  child_list.pop_front();
  prefix.pop_back();
}

template <typename Branch, typename Event>
void WakeupTreeExplorationBuffer<Branch,Event>::enter_first_child(Event event) {
  assert(parent_at(len()).size());
  WakeupTreeRef<Branch> node = parent_at(len()).begin().node();
  prefix.push_back({std::move(event), node});
}

template <typename Branch, typename Event>
void WakeupTreeExplorationBuffer<Branch,Event>::push
(Branch branch, Event event) {
  WakeupTreeRef<Branch> node = parent_at(len());
  assert(node.size() == 0);
  WakeupTreeRef<Branch> new_node = node.put_child(std::move(branch));
  prefix.push_back({std::move(event), WakeupTreeRef<Branch>(new_node)});
}

template <typename Branch, typename Event>
void WakeupTreeExplorationBuffer<Branch,Event>::set_last_branch(Branch b){
  WakeupTreeRef<Branch> par = parent_at(len()-1);
  assert(par.size() == 1 || b == lastbranch());
  assert(par->children.front().first == b);
  par->children.front().first = std::move(b);
}
