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


void RFSCDecisionTree::clear_unrealizable_siblings(std::shared_ptr<DecisionNode> *TB_work_item) {
  auto node = *TB_work_item;
  std::shared_ptr<DecisionNode> tmp;
  while (node->get_siblings().empty()) {
    tmp = node;
    node = node->parent;
    if (node->depth == -1) {
      break;
    }
    tmp->temporary_clear_sleep();
  }
  *TB_work_item = node;
  // decisions.resize(node->depth+1, decisions[0]);
}

// void RFSCDecisionTree::place_decision_into_sleepset(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &decision) {
//   decisions.back()->sleep_emplace(decision);
// }



void
RFSCDecisionTree::get_next_sibling(std::shared_ptr<DecisionNode> *TB_work_item) {



// Possible way of operating through nodes

/***************************************************************************************/
/*    Taking the end of vector    */

  // std::shared_ptr<DecisionNode> node = work_queue.back();
  // node->get_siblings().erase(node);
  // work_queue.pop_back();
  
  // *TB_work_item = std::move(node);

/***************************************************************************************/
/*    Taking the front of vector (queue)    */

  // auto it = work_queue.begin();
  // // if (it == work_queue.end()) {
  // //   printf("ERROR:  Work queue is empty!\n");
  // //   abort();
  // // }
  
  // std::shared_ptr<DecisionNode> node = *it;
  // // if (node->get_siblings().count(node) == 0)
  // // {
  // //   printf("Cant find itself in siblings\n");
  // //   abort();
  // // }

  // // node->get_next_sibling();
  // node->parent->children_unf_set.erase(node->unfold_node);
  // size_t amount = node->get_siblings().erase(node);
  // // if (amount != 1) {
  // //   printf("ERROR: get_siblings().erase() erased wrong amount (should be 1): %ld\n", amount);
  // //   abort();
  // // }
  // work_queue.erase(it);
  // *TB_work_item = std::move(node);


/***************************************************************************************/
/*    Taking the first sibling (simulate decisions vector)    */

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
  
  node->parent->children_unf_set.erase(node->unfold_node);
  node->get_siblings().erase(node);
  *TB_work_item = std::move(node);



/***************************************************************************************/
/*    Old version still running from decisions-vector    */

  // std::shared_ptr<DecisionNode> node = decisions.back();
  // auto it = node->get_siblings().begin();
  // decisions[decisions.size()-1] = *it;


  // for (auto i = work_queue.begin(); i != work_queue.end(); i++)
  // {
  //   if ((*i)->ID == (*it)->ID) {
  //     work_queue.erase(i);
  //     break;
  //   }
  // }

  // node->get_siblings().erase(it);
  // // TODO: For now this does not erase the new decisions unf from children_unf_set,
  // // making it possible for it to find older versions of unf and possibly introduce a bug to the compute_prefixes
  // // (*it)->get_next_sibling();

  // return decisions.back(); //->get_next_sibling();
}


void RFSCDecisionTree::erase_sibling(std::pair<const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode>, Leaf> sit) {
  // decisions.back()->get_siblings().erase(sit.first);
  return;
}



std::shared_ptr<DecisionNode> RFSCDecisionTree::new_decision_node(std::shared_ptr<DecisionNode> parent) {
  auto decision = std::make_shared<DecisionNode>(parent);
  // decisions.push_back(decision);
  return decision;
}



void RFSCDecisionTree::construct_sibling(std::shared_ptr<DecisionNode> decision, const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf, Leaf l) {
  auto sibling = decision->make_sibling(unf, l);
  // work_queue.insert(sibling);
  work_queue.push_back(sibling);
}

bool RFSCDecisionTree::work_queue_empty() {
  // if (decisions.empty() && !work_queue.empty()) {
  //   printf("ERROR:  Decisions and work_queue do not match!\n");
  //   abort();
  // }
  // if (work_queue.empty() && !decisions.empty()) {
  //   printf("ERROR: work_queue and Decisions do not match!\n");
  //   abort();
  // }

  // return decisions.empty();
  return work_queue.empty();

}

void RFSCDecisionTree::print_wq() {
  for (auto node : work_queue) {
    llvm::dbgs() <<  node->name << "\n";
  }
}




/*************************************************************************************************************
 * 
 *      DecisionNode
 * 
*************************************************************************************************************/


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

void DecisionNode::get_next_sibling() {
  parent->children_unf_set.erase(unfold_node);
}

void DecisionNode::temporary_clear_sleep() {
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
  return parent->children_unf_set.count(unf);
}

void DecisionNode::alloc_unf(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf) {
  parent->children_unf_set.insert(unf);
}


std::shared_ptr<DecisionNode> find_ancester(std::shared_ptr<DecisionNode> node, int wanted) {
  while (node->depth != wanted) {
    node = node->parent;
  }
  return node;
}