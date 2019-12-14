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

void RFSCDecisionTree::prune_decisions(int blame) {
  // assert(int(decisions.size()) > blame);
  // decisions.resize(blame+1, decisions[0]);
}

void RFSCDecisionTree::clear_unrealizable_siblings(std::shared_ptr<DecisionNode> *TB_work_item) {
  auto node = decisions.back();
  while (node->get_siblings().empty()) {
    decisions.back()->temporary_clear_sleep();
    decisions.pop_back();
    if (decisions.empty()) {
      return;
    }
    node = decisions.back();
  }
  *TB_work_item = node;
}

void RFSCDecisionTree::place_decision_into_sleepset(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &decision) {
  decisions.back()->sleep_emplace(decision);
}



/***************************************************************************************/
/*    Taking the first sibling (simulate decisions vector)    */

// std::shared_ptr<DecisionNode>
void
RFSCDecisionTree::get_next_sibling(std::shared_ptr<DecisionNode> *TB_work_item) {

  std::shared_ptr<DecisionNode> node = *TB_work_item;
  auto it = node->get_siblings().begin();

  // node = *it; // temp workaround when subsequent info is commented
  for (auto i = work_queue.begin(); i != work_queue.end(); i++) {
    if ((*i)->ID == (*it)->ID) {
      node = *i;
      work_queue.erase(i);
      break;
    }
  }


  // node->get_next_sibling();
  node->parent->children_unf_set.erase(node->unfold_node);
  node->get_siblings().erase(node);

  decisions[decisions.size()-1] = node;
  // return node;
  *TB_work_item = std::move(node);
}


void RFSCDecisionTree::erase_sibling(std::pair<const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode>, Leaf> sit) {
  // decisions.back()->get_siblings().erase(sit.first);
}



std::shared_ptr<DecisionNode> RFSCDecisionTree::new_decision_node(std::shared_ptr<DecisionNode> parent) {
  auto decision = std::make_shared<DecisionNode>(parent);
  decisions.push_back(decision);
  return decision;
}



void RFSCDecisionTree::construct_sibling(std::shared_ptr<DecisionNode> decision, const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf, Leaf l) {
  auto sibling = decision->make_sibling(unf, l);
  work_queue.push_back(sibling);
}

bool RFSCDecisionTree::work_queue_empty() {
  if (decisions.empty() && !work_queue.empty()) {
    printf("ERROR:  Decisions and work_queue do not match!\n");
    abort();
  }
  if (work_queue.empty() && !decisions.empty()) {
    printf("ERROR: work_queue and Decisions do not match!\n");
    abort();
  }

  return decisions.empty();
  // return work_queue.empty();

  }



/*************************************************************************************************************
 * 
 *      DecisionNode
 * 
*************************************************************************************************************/

// Decided to move this to DecisionTree
// void DecisionNode::sibling_emplace(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf, Leaf l) {
//   siblings.emplace(unf, l);
// }




void DecisionNode::sleep_emplace(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf) {
  // Should be able to be used when each sibling is its own decisionNode
  // parent->sleep.emplace(unf);
  // parent->children_unf_map[this->ID].emplace(unf);
  parent->children_unf_set.emplace(unf);
}

// void DecisionNode::make_sibling(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf, Leaf l) {
std::shared_ptr<DecisionNode> DecisionNode::make_sibling(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf, Leaf l) {
  parent->children_unf_set.insert(unf);
  auto sibling = std::make_shared<DecisionNode>(parent, unf, l);
  get_siblings().insert(sibling);

  return sibling;
}

// std::pair<const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode>, Leaf> 
// DecisionNode::get_next_sibling() {
//   auto node = this;
//   while (node->get_siblings().empty())
//   {
//     node = node->parent.get();
//   }
  
//   auto it = node->get_siblings().begin();
//   auto pair = std::pair<const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode>, Leaf>(it->first, it->second);
//   node->parent->children_unf_map[node->ID].erase(it->first);
//   // parent->children_unf_map[this->ID].insert(it->first);
//   node->get_siblings().erase(it);
//   return pair;

// }

void DecisionNode::temporary_clear_sleep() {
  // Should be able to be used when each sibling is its own decisionNode
  // if (depth >= 0) {
  //   parent->sleep.clear(); }
  if (depth >= 0)
    parent->children_unf_set.clear();
}


SaturatedGraph &DecisionNode::get_saturated_graph() {
  assert(depth != -1);  
  SaturatedGraph &g = parent->temporary_graph_cache[this->ID];
  if (g.size() || depth == 0) return g;
  auto node = parent;
  do {
    if (node->temporary_graph_cache[node->ID].size()) {
      /* Reuse subgraph */
      g = node->temporary_graph_cache[node->ID];
      break;
    }
    node = node->parent;
    
  } while (node->depth != -1);

  return g;
}


bool DecisionNode::unf_is_known(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf) {
  // return parent->children_unf_map[this->ID].count(unf); // + parent->siblings.count(unf);
  return parent->children_unf_set.count(unf);
  // bool res =  parent->children_unf_map[this->ID].count(unf);
  // if (res) {
  //   return true;
  // }
  // parent->children_unf_map[this->ID].insert(unf);
  // return false;
}

void DecisionNode::alloc_unf(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf) {
  // parent->children_unf_map[this->ID].insert(unf);
  parent->children_unf_set.insert(unf);
}