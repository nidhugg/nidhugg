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
#include "RFSCDecisionTree.h"

void WorkQueue::push(UNF_LEAF_PAIR const & item)
{
    std::lock_guard<std::mutex> lock(this->queue_mutex);
    this->q.push(std::make_shared<UNF_LEAF_PAIR>(item));
    // this->condition.notify_all();
}

std::shared_ptr<UNF_LEAF_PAIR> WorkQueue::wait_and_pop()
{
    std::lock_guard<std::mutex> lock(this->queue_mutex);
    // this->condition.wait(lock, [&]{ return !this->q.empty();});
    if (!this->q.empty()) {
      auto res = this->q.front();
      this->q.pop();
      return res;
    }
    return std::shared_ptr<UNF_LEAF_PAIR>(nullptr);
}

void RFSCDecisionTree::prune_decisions(int blame) {
  assert(int(decisions.size()) > blame);
  decisions.resize(blame+1, decisions[0]);
}

void RFSCDecisionTree::clear_unrealizable_siblings() {
  for(; !decisions.empty(); decisions.pop_back()) {
    auto &siblings = decisions.back().siblings;
    for (auto it = siblings.begin(); it != siblings.end();) {
      if (it->second.is_bottom()) {
        // this is not realisable and can be moved to sleepset
        decisions.back().sleep.emplace(std::move(it->first));
        it = siblings.erase(it);
      } else {
        ++it;
      }
    }
    if(!siblings.empty()){
      break;
    }
  }
}

void RFSCDecisionTree::place_decision_into_sleepset(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &decision) {
  decisions.back().sleep.emplace(decision);
}


UNF_LEAF_PAIR RFSCDecisionTree::get_next_sibling() {
  return decisions.back().siblings.begin();
}

void RFSCDecisionTree::erase_sibling(UNF_LEAF_PAIR sit) {
  decisions.back().siblings.erase(sit);
}

int RFSCDecisionTree::new_decision_node() {
  int decision = decisions.size();
  decisions.emplace_back();
  return decision;
}

SaturatedGraph &RFSCDecisionTree::get_saturated_graph(unsigned i) {
  SaturatedGraph &g = decisions[i].graph_cache;
  if (g.size() || i == 0) return g;
  for (unsigned j = i-1; j != 0; --j) {
    if (decisions[j].graph_cache.size()) {
      /* Reuse subgraph */
      g = decisions[j].graph_cache;
      break;
    }
  }
  return g;
}

void RFSCDecisionTree::add_to_wq() {
    // TODO add note to workqueue and add node in decision tree
}
