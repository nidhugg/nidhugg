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
  // auto  node = decisions.back();
  // while (node->get_siblings().empty())
  // {
  //   decisions.back()->temporary_clear_sleep();
  //   decisions.pop_back();
  //   if (decisions.empty())
  //   {
  //     return;
  //   }
  //   node = decisions.back();
  // }
  // *TB_work_item = node;


  auto root = get_root();
  auto node = *TB_work_item;
  while (node->get_siblings().empty())
  {
    node->temporary_clear_sleep();
    node = node->parent;
    if (node->depth == -1)
    {
      break;
    }
  }
  *TB_work_item = node;
  // decisions.resize(node->depth+1, decisions[0]);
}



// std::pair<const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode>, Leaf>
std::shared_ptr<DecisionNode>
RFSCDecisionTree::get_next_sibling(std::shared_ptr<DecisionNode> *TB_work_item) {

  int res = 0;
  int *sum = &res;
  std::for_each(decisions.begin(), decisions.end(),
    [sum](std::shared_ptr<DecisionNode> node){*sum += node->get_siblings().size();}
  );
  // printf("DEBUG: siblings: %d, work_items: %ld\n", res, work_queue.size());
  // if (res != work_queue.size()) {
  //   printf("ERROR:  Mismatch! siblings: %d, work_items: %ld\n", res, work_queue.size());
  //   abort();
  // }


// Possible way of operating through nodes
  // std::shared_ptr<DecisionNode> node = work_queue.back();
  // node->get_siblings().erase(node);
  // work_queue.pop_back();
  // decisions[decisions.size()-1] = node;

  // auto it = work_queue.begin();
  // std::shared_ptr<DecisionNode> node = *it;

  // if (node->get_siblings().count(*it) == 0)
  // {
  //   printf("Cant find itself in siblings\n");
  //   abort();
  // }
  
  // node->get_siblings().erase(*it);
  // work_queue.erase(it);
  // decisions[decisions.size()-1] = node;
  // return node;


  std::shared_ptr<DecisionNode> node = *TB_work_item;
  auto it = node->get_siblings().begin();
  int iter = 0;
  for (auto i = work_queue.begin(); i != work_queue.end(); i++)
  {
    if ((*i)->ID == (*it)->ID) {
      // printf("DEBUG: work-item iter: %d\n", iter);
      node = *i;
      work_queue.erase(i);
      break;
    }
    ++iter;
  }


  if (node->get_siblings().count(node) == 0)
  {
    printf("Cant find itself in siblings\n");
    abort();
  }
  
  node->get_siblings().erase(node);
  // decisions[decisions.size()-1] = node;
  return node;
}


//   std::shared_ptr<DecisionNode> node = decisions.back();
//   // while (node->get_siblings().empty())
//   // {
//   //   // decisions.back()->temporary_clear_sleep();
//   //   decisions.pop_back();
//   //   node = decisions.back();
//   // }
//   auto it = node->get_siblings().begin();
//   decisions[decisions.size()-1] = *it;


//   bool found = false;
//   for (auto i = work_queue.begin(); i != work_queue.end(); i++)
//   {
//     if ((*i)->ID == (*it)->ID) {
//       found = true;
//       work_queue.erase(i);
//       break;
//     }
//   }
//   if(!found) {
//     printf("Could not find next work task in work queue!\n");
//     abort();
//   }

//   node->get_siblings().erase(it);
//   // TODO: For now this does not erase the new decisions unf from children_unf_set,
//   // making it possible for it to find older versions of unf and possibly introduce a bug to the compute_prefixes
//   // printf("unf: %p\n", (*it)->unfold_node.get());
//   // (*it)->get_next_sibling();

//   if (decision_id > 80)
//   {
//     abort();
//   }
//   return decisions.back(); //->get_next_sibling();
// }

void RFSCDecisionTree::erase_sibling(std::pair<const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode>, Leaf> sit) {
  // decisions.back()->get_siblings().erase(sit.first);
}

std::shared_ptr<DecisionNode> RFSCDecisionTree::new_decision_node(std::shared_ptr<DecisionNode> parent) {
  auto decision = std::make_shared<DecisionNode>(parent);
  // decisions.push_back(decision);
  return decision;
}



void RFSCDecisionTree::construct_sibling(std::shared_ptr<DecisionNode> decision, const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf, Leaf l) {
  auto sibling = decision->make_sibling(unf, l);
  work_queue.insert(sibling);
}

bool RFSCDecisionTree::work_queue_empty() {

  // int res = 0;
  // int *sum = &res;
  // std::for_each(decisions.begin(), decisions.end(),
  //   [sum](std::shared_ptr<DecisionNode> node){*sum += node->get_siblings().size();}
  // );
  // if (res != work_queue.size()) {
  //   printf("ERROR:  Mismatch! siblings: %d, work_items: %ld\n", res, work_queue.size());
  //   abort();
  // }


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
  // printf("unf: %p\n", unf.get());
  parent->children_unf_set.emplace(unf);
}

std::shared_ptr<DecisionNode> DecisionNode::make_sibling(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf, Leaf l) {
  parent->children_unf_set.insert(unf);

  // get_siblings().emplace(unf, l);
  auto sibling = std::make_shared<DecisionNode>(parent, unf, l);
  get_siblings().insert(sibling);
  return sibling;

}

// std::pair<const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode>, Leaf> 
void
DecisionNode::get_next_sibling() {

  // auto pair = std::pair<const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode>, Leaf>(unfold_node, leaf);
  parent->children_unf_set.erase(unfold_node);
  // return NULL;
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