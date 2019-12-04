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
  auto  node = decisions.back();
  while (node->get_siblings().empty())
  {
    decisions.back()->temporary_clear_sleep();
    decisions.pop_back();
    if (decisions.empty())
    {
      return;
    }
    node = decisions.back();
  }
  *TB_work_item = node;
}



// std::pair<const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode>, Leaf>
std::shared_ptr<DecisionNode>
RFSCDecisionTree::get_next_sibling() {

//   std::shared_ptr<DecisionNode> node = work_queue.back();
//   work_queue.pop_back();
//   // return node;

//   bool found = false;
//   for (int i = decisions.size()-1; i >= 0; --i)
//   {
//     // printf("i: %d\n", i);
//     for (auto j = decisions[i]->get_siblings().begin(); j != decisions[i]->get_siblings().end(); j++)
//     {
//       if (node->ID == (*j)->ID) {
//         found = true;
//         decisions[i]->get_siblings().erase(j);
//         break;
//       }
//     }

//   }
//   if(!found) {
//     printf("Could not find next work task in work queue!\n");
//     // abort();
//   }
//   return node;
// }

  std::shared_ptr<DecisionNode> node = decisions.back();
  // while (node->get_siblings().empty())
  // {
  //   // decisions.back()->temporary_clear_sleep();
  //   decisions.pop_back();
  //   node = decisions.back();
  // }
  auto it = node->get_siblings().begin();
  decisions[decisions.size()-1] = *it;


  bool found = false;
  for (auto i = work_queue.begin(); i != work_queue.end(); i++)
  {
    if ((*i)->ID == (*it)->ID) {
      found = true;
      work_queue.erase(i);
      break;
    }
  }
  if(!found) {
    printf("Could not find next work task in work queue!\n");
    abort();
  }

  node->get_siblings().erase(it);
  // TODO: For now this does not erase the new decisions unf from children_unf_set,
  // making it possible for it to find older versions of unf and possibly introduce a bug to the compute_prefixes
  return decisions.back(); //->get_next_sibling();
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

  return work_queue.empty();
  }

// void RFSCDecisionTree::add_to_workqueue(std::shared_ptr<DecisionNode> decision, UNFOLD_PTR unf, Leaf l) {
//   auto node = decision->create_sibling();
//   node->update(unf, l);
//   work_queue.push(std::move(node));
// }


/*************************************************************************************************************
 * 
 *      DecisionNode
 * 
*************************************************************************************************************/


void DecisionNode::place_decision_into_sleepset(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf) {
  sleep_emplace(unf);
}

void DecisionNode::sleep_emplace(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf) {
  parent->children_unf_set.emplace(unf);
}

std::shared_ptr<DecisionNode> DecisionNode::make_sibling(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf, Leaf l) {
  parent->children_unf_set.insert(unf);

  // get_siblings().emplace(unf, l);
  auto sibling = std::make_shared<DecisionNode>(parent, unf, l);
  get_siblings().insert(sibling);
  return sibling;

}

std::pair<const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode>, Leaf> 
DecisionNode::get_next_sibling() {

  auto pair = std::pair<const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode>, Leaf>(unfold_node, leaf);
  parent->children_unf_set.erase(unfold_node);
  return pair;
}

void DecisionNode::temporary_clear_sleep() {
  if (depth >= 0) {
    parent->children_unf_set.clear(); }
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
  // bool res =  parent->children_unf_map[this->ID].count(unf);
  // if (res) {
  //   return true;
  // }
  // parent->children_unf_map[this->ID].insert(unf);
  // return false;
}

void DecisionNode::alloc_unf(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf) {
  parent->children_unf_set.insert(unf);
}