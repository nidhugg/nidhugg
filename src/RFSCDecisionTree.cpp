/* Copyright (C) 2018 Alexis Remmers and Nodari Kankava
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

static bool prune_node(std::shared_ptr<DecisionNode> node, const std::shared_ptr<DecisionNode> &blame) {
  while (node->parent != nullptr) {
    if (node.get() == blame.get()) {
      return true;
    }
    node = node->parent;
  }
  return false;
}


void RFSCDecisionTree::prune_decisions(std::shared_ptr<DecisionNode> blame) {
  std::lock_guard<std::mutex> lock(this->queue_mutex);
  // Would perhaps be more efficient with a remove_if()
  for( auto iter = work_queue.begin(); iter != work_queue.end(); ) {
    if( prune_node(*iter, blame) )
      iter = work_queue.erase( iter ); // advances iter
    else
      ++iter ; // don't remove
  }
}


std::shared_ptr<DecisionNode> RFSCDecisionTree::get_next_work_task() {
  std::lock_guard<std::mutex> lock(this->queue_mutex);

  auto it = work_queue.begin();
  std::shared_ptr<DecisionNode> node = *it;
  work_queue.erase(it);

  return std::move(node);
}


std::shared_ptr<DecisionNode> RFSCDecisionTree::new_decision_node(std::shared_ptr<DecisionNode> parent, const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf) {
  auto decision = std::make_shared<DecisionNode>(parent);
  decision->place_decision_into_sleepset(unf);
  return decision;
}


void RFSCDecisionTree::construct_sibling(std::shared_ptr<DecisionNode> decision, const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf, Leaf l, std::vector<int> iid_map) {
  std::lock_guard<std::mutex> lock(this->queue_mutex);
  work_queue.push_back(std::move(decision->make_sibling(unf, l, iid_map)));
}


bool RFSCDecisionTree::work_queue_empty() {
  return work_queue.empty();
}


std::shared_ptr<DecisionNode> RFSCDecisionTree::find_ancestor(std::shared_ptr<DecisionNode> node, int wanted) {
  assert(node->depth >= wanted);
  while (node->depth != wanted) {
    node = node->parent;
  }
  return node;
}




/*************************************************************************************************************
 * 
 *      DecisionNode
 * 
*************************************************************************************************************/

void DecisionNode::set_name() {
    name = parent->name + '-' + parent->name_index;
}
void DecisionNode::set_sibling_name() {
  char index = parent->name_index.back();
  if (index == 'Z') {
    parent->name_index.push_back('A');
  }
  else {
    parent->name_index.pop_back();
    parent->name_index.push_back(index+1);
  }
  name = parent->name + '-' + parent->name_index;
}

void DecisionNode::place_decision_into_sleepset(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf) {
  if (depth != -1) {
    parent->children_unf_set.emplace(unf);
  }
  else {
    children_unf_set.emplace(unf);
  }
}


std::shared_ptr<DecisionNode> DecisionNode::make_sibling(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf, Leaf l, std::vector<int> iid_map) {
  parent->children_unf_set.insert(unf);
  return std::make_shared<DecisionNode>(parent, unf, l, iid_map);
}


SaturatedGraph &DecisionNode::get_saturated_graph() {
  assert(depth != -1);  
  SaturatedGraph &g = parent->graph_cache;
  if (g.size() || depth == 0) return g;
  auto node = parent;
  do {
    if (node->graph_cache.size()) {
      /* Reuse subgraph */
      g = node->graph_cache;
      break;
    }
    node = node->parent;
    
  } while (node->depth != -1);

  return g;
}


bool DecisionNode::unf_is_known(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf) {
  return parent->children_unf_set.count(unf);
}

void DecisionNode::alloc_unf(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf) {
  parent->children_unf_set.insert(unf);
}
