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

static bool prune_node(DecisionNode *node, const DecisionNode *blame) {
  while (node->parent != nullptr) {
    if (node == blame) {
      return true;
    }
    node = node->parent.get();
  }
  return false;
}

std::mutex RFSCDecisionTree::decision_tree_mutex;

void RFSCDecisionTree::prune_decisions(const std::shared_ptr<DecisionNode> &blame) {
  // std::lock_guard<std::mutex> lock(decision_tree_mutex);
  // // Would perhaps be more efficient with a remove_if()
  // for( auto iter = work_queue.begin(); iter != work_queue.end(); ) {
  //   if( prune_node(iter->get(), blame.get()) )
  //     iter = work_queue.erase( iter ); // advances iter
  //   else
  //     ++iter ; // don't remove
  // }
  blame->pruned_subtree.store(true, std::memory_order_release);
}


std::shared_ptr<DecisionNode> RFSCDecisionTree::get_next_work_task() {
  std::lock_guard<std::mutex> lock(decision_tree_mutex);
  std::shared_ptr<DecisionNode> node = work_queue.top();
  work_queue.pop();

  return std::move(node);
}


std::shared_ptr<DecisionNode> RFSCDecisionTree::new_decision_node(const std::shared_ptr<DecisionNode> &parent, const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf) {
  // std::lock_guard<std::mutex> lock(decision_tree_mutex);
  // auto decision = std::make_shared<DecisionNode>(parent);
  // decision->place_decision_into_sleepset(unf);
  // return std::move(decision);

  auto decision = std::make_shared<DecisionNode>(parent);
  {
  std::lock_guard<std::mutex> lock(decision_tree_mutex);
  decision->place_decision_into_sleepset(unf);
  }
  return std::move(decision);
}


void RFSCDecisionTree::construct_sibling(const std::shared_ptr<DecisionNode> &decision, const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf, Leaf l) {
  std::lock_guard<std::mutex> lock(decision_tree_mutex);
  work_queue.push(std::move(decision->make_sibling(unf, l)));
}


bool RFSCDecisionTree::work_queue_empty() {
  std::lock_guard<std::mutex> lock(decision_tree_mutex);
  return work_queue.empty();
}


const std::shared_ptr<DecisionNode> &RFSCDecisionTree::find_ancestor(const std::shared_ptr<DecisionNode> &node, int wanted) {
  // std::lock_guard<std::mutex> lock(decision_tree_mutex);
  assert(node->depth >= wanted);

  /* Ugly workaround to traverse the tree without updating the ref_count
   * while at the same time return a shared pointer to correct object. */
  if (node->depth == wanted) {
    return node;
  }
  auto it = node.get();
  while (it->parent->depth != wanted) {
    it = it->parent.get();
  }
  return it->parent;
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


std::shared_ptr<DecisionNode> DecisionNode::make_sibling(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf, Leaf l) {
  return std::make_shared<DecisionNode>(parent, unf, l);
}


SaturatedGraph &DecisionNode::get_saturated_graph(bool &complete) {
  std::lock_guard<std::mutex> lock(parent->decision_node_mutex);
  assert(depth != -1);
  SaturatedGraph &g = parent->graph_cache;
  if (g.size() || depth == 0) {
    complete = true;
    return g;
  }
  auto node = parent;
  do {
    if (node->graph_cache.size()) {
      /* Reuse subgraph */
      g = node->graph_cache;
      break;
    }
    node = node->parent;
    
  } while (node->depth != -1);

  complete = false;
  return g;
}


bool DecisionNode::try_alloc_unf(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf) {
  std::lock_guard<std::mutex> lock(parent->decision_node_mutex);
  return parent->children_unf_set.insert(unf).second;
}

bool DecisionNode::defined_pruned() {

  for (auto node = this; node->depth != -1; node = node->parent.get()) {
    if (node->pruned_subtree.load(std::memory_order_acquire))
      return true;
  }
  return false;

}