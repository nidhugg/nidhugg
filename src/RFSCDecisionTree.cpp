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
  // Would perhaps be more efficient with a remove_if()
  for( auto iter = work_queue.begin(); iter != work_queue.end(); ) {
    if( prune_node(*iter, blame) )
      iter = work_queue.erase( iter ); // advances iter
    else
      ++iter ; // don't remove
  }
}


void RFSCDecisionTree::backtrack_decision_tree(std::shared_ptr<DecisionNode> *TB_work_item) {
  auto node = *TB_work_item;
  std::shared_ptr<DecisionNode> tmp;
  while (node->get_siblings().empty()) {
    tmp = node;
    node = node->parent;
    if (node->depth == -1) {
      break;
    }
    // tmp->clear_unf_set();
  }
  *TB_work_item = node;
}


void RFSCDecisionTree::get_next_work_task(std::shared_ptr<DecisionNode> *TB_work_item) {

#ifndef __WORK_QUEUE_VECTOR_COMPABILITY_MODE__

/***************************************************************************************/
/* Taking the front of vector (as queue but compatible with vector)
 *
 * This does unfortunately not work as the oracle will find an infinite amount 
 * of Leafs and reset() requires to know what node an event should be placed to 
 * sleep in based on non-evaluated siblings.    */

  auto it = work_queue.begin();
  std::shared_ptr<DecisionNode> node = *it;
  work_queue.erase(it);


#else
/***************************************************************************************/
/* Taking the first sibling (based on backtracking treepath to non-empty sibling-set)
 *
 * This simulates the old decisions vector by taking every sibling in an in-order fashion. 
 * This cannot operate parallelized as it requires that each sibling in a given level 
 * to be evaluated before going forth.    */

  std::shared_ptr<DecisionNode> node = *TB_work_item;
  auto it = node->get_siblings().begin();
  for (auto i = work_queue.begin(); i != work_queue.end(); i++)
  {
    if ((*i)->ID == (*it)->ID) {
      node = *i;
      work_queue.erase(i);
      break;
    }
  }
  
#endif

  // node->get_unf_set().erase(node->unfold_node);
  node->get_siblings().erase(node);
  *TB_work_item = std::move(node);
}


std::shared_ptr<DecisionNode> RFSCDecisionTree::new_decision_node(std::shared_ptr<DecisionNode> parent, const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf) {
  auto decision = std::make_shared<DecisionNode>(parent);
  decision->place_decision_into_sleepset(unf);
  return decision;
}


void RFSCDecisionTree::construct_sibling(std::shared_ptr<DecisionNode> decision, const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf, Leaf l) {
  work_queue.push_back(std::move(decision->make_sibling(unf, l)));
}


bool RFSCDecisionTree::work_queue_empty() {
  return work_queue.empty();
}


void RFSCDecisionTree::print_wq() {
  for (auto node : work_queue) {
    llvm::dbgs() <<  node->name << "\n";
  }
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


std::shared_ptr<DecisionNode> DecisionNode::make_sibling(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf, Leaf l) {
  parent->children_unf_set.insert(unf);
  auto sibling = std::make_shared<DecisionNode>(parent, unf, l);
  get_siblings().insert(sibling);

  return sibling;
}


void DecisionNode::clear_unf_set() {
  if (depth >= 0)
    parent->children_unf_set.clear();
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
