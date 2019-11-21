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
  assert(int(decisions.size()) > blame);
  decisions.resize(blame+1, decisions[0]);
}

void RFSCDecisionTree::clear_unrealizable_siblings() {
  for(; !decisions.empty(); decisions.pop_back()) {
    auto &siblings = decisions.back().get_siblings();
    for (auto it = siblings.begin(); it != siblings.end();) {
      if (it->second.is_bottom()) {
        // this is not realisable and can be moved to sleepset
        decisions.back().sleep_emplace(std::move(it->first));
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
  decisions.back().sleep_emplace(decision);
}


UNF_LEAF_PAIR RFSCDecisionTree::get_next_sibling() {
  return decisions.back().get_siblings().begin();
}

void RFSCDecisionTree::erase_sibling(UNF_LEAF_PAIR sit) {
  decisions.back().get_siblings().erase(sit);
}


SaturatedGraph &RFSCDecisionTree::get_saturated_graph(unsigned i) {
  SaturatedGraph &g = decisions[i].get_saturated_graph();
  if (g.size() || i == 0) return g;
  for (unsigned j = i-1; j != 0; --j) {
    if (decisions[j].get_saturated_graph().size()) {
      /* Reuse subgraph */
      g = decisions[j].get_saturated_graph();
      break;
    }
  }
  return g;
}

// std::shared_ptr<DecisionNode> RFSCDecisionTree::new_decision_node(std::shared_ptr<DecisionNode> parent) {
//   return std::make_shared<DecisionNode>( DecisionNode(parent) );
// }

int RFSCDecisionTree::new_decision_node() {
  int decision = decisions.size();
  decisions.emplace_back();
  return decision;
}


void RFSCDecisionTree::construct_sibling(DecisionNode &decision, const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf, Leaf l) {
  decision.get_siblings().emplace(unf, l);
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
  sleep.emplace(unf);
}

SaturatedGraph &DecisionNode::get_saturated_graph() {
  // assert(depth != -1);
  // SaturatedGraph &g = parent->children_graph_cache;
  // if (g.size() || depth == 0) {
  //   return g;
  // }
  // return parent->get_graph_cache();
  return graph_cache;
}


bool DecisionNode::unf_is_known(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> unf) {
  return this->get_siblings().count(unf) + this->get_sleep().count(unf);
}