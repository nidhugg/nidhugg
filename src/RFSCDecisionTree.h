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

#include <config.h>
#ifndef __RFSC_DECISION_TREE_H__
#define __RFSC_DECISION_TREE_H__

#define UNF_LEAF_PAIR std::__detail::_Node_iterator<std::pair<const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode>, Leaf>, false, false>

#include "SymEv.h"
#include "SaturatedGraph.h"
#include "RFSCUnfoldingTree.h"
// #include "RFSCTraceBuilder.h"

#include <unordered_map>
#include <unordered_set>
#include <queue>

struct DecisionNode;

struct Branch {
public:
  Branch(int pid, int size, std::shared_ptr<DecisionNode> decision, int decision_depth, bool pinned, SymEv sym)
    : pid(pid), size(size), decision_ptr(decision), decision_depth(decision_depth), pinned(pinned),
      sym(std::move(sym)) {}
  Branch() : Branch(-1, 0, nullptr, -1, false, {}) {}
  int pid, size, decision_depth;
  std::shared_ptr<DecisionNode> decision_ptr;
  bool pinned;
  SymEv sym;
};

// struct Branch {
// public:
//   Branch(int pid, int size, std::shared_ptr<DecisionNode> decision_ptr, int decision_d, bool pinned, SymEv sym)
//     : pid(pid), size(size), decision_ptr(decision_ptr), decision_depth(decision_d), pinned(pinned),
//       sym(std::move(sym)) {}
//   // Branch() : Branch(-1, 0, -1, false, {}) {}
//   int pid, size, decision_depth;
//   std::shared_ptr<DecisionNode> decision_ptr;
//   bool pinned;
//   SymEv sym;
// };

struct Leaf {
public:
  /* Construct a bottom-leaf. */
  Leaf() : prefix() {}
  /* Construct a prefix leaf. */
  Leaf(std::vector<Branch> prefix) : prefix(prefix) {}
  std::vector<Branch> prefix;

  bool is_bottom() const { return prefix.empty(); }
};

typedef unsigned int DecisionNodeID;

static DecisionNodeID decision_id;
static size_t decision_count;

struct DecisionNode {
public:
  // Empty constructor should only be used to construct the root
  DecisionNode() : parent(nullptr), siblings(), depth(-1) { decision_id = 0;}
  DecisionNode(std::shared_ptr<DecisionNode> decision)
        : parent(decision), depth(decision->depth+1), ID(++decision_id) {
      decision_count++;
    };
  DecisionNode(std::shared_ptr<DecisionNode> decision, std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> unf, Leaf l)
        : parent(decision), depth(decision->depth+1), ID(++decision_id), unfold_node(std::move(unf)), leaf(l) {
      decision_count++;
  };
  ~DecisionNode() { decision_count--; };

  int depth;
  unsigned int ID;

  bool unf_is_known(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf);
  void alloc_unf(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf);
  

  // std::unordered_map<std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode>, Leaf> &
  std::unordered_set<std::shared_ptr<DecisionNode>> &
  get_siblings() {return parent->siblings;};
  // Decided to move this to DecisionTree
  // void sibling_emplace(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf, Leaf l);
  
  void sleep_emplace(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf);
  void temporary_clear_sleep();

  std::shared_ptr<DecisionNode> make_sibling(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf, Leaf l);

  std::pair<const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode>, Leaf>
  get_next_sibling();

  SaturatedGraph &get_saturated_graph();

  std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> unfold_node;
  Leaf leaf;

  std::shared_ptr<DecisionNode> parent;
  std::unordered_set<std::shared_ptr<DecisionNode>> siblings;
  std::unordered_set<std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode>> children_unf_set;
  // std::unordered_map<DecisionNodeID, std::unordered_set<std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode>>> children_unf_map;

protected:



  // std::shared_ptr<DecisionNode> parent;

  // std::unordered_map<std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode>, Leaf> siblings;
  // Should be able to be used when each sibling is its own decisionNode
  // std::unordered_set<std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode>> sleep;
  // std::unordered_map<DecisionNodeID, std::unordered_set<std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode>>> children_unf_map;
  SaturatedGraph graph_cache;

  std::unordered_map<DecisionNodeID, SaturatedGraph> temporary_graph_cache;
};



class RFSCDecisionTree final {
public:
  RFSCDecisionTree() : root(std::make_shared<DecisionNode>()) {
    // root = std::make_shared<DecisionNode>();
  };

  /* Using the last decision that caused a failure, and then
   * prune all later decisions. */
  void prune_decisions(int blame);
  void clear_unrealizable_siblings(std::shared_ptr<DecisionNode> *TB_work_item);
  
  size_t size() {return decisions.size();};
  std::shared_ptr<DecisionNode> get(int decision) {return decisions[decision];}
  void place_decision_into_sleepset(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &decision);


  // std::shared_ptr<DecisionNode> get_next_sibling(std::shared_ptr<DecisionNode> *TB_work_item);
  void get_next_sibling(std::shared_ptr<DecisionNode> *TB_work_item);

  void erase_sibling(std::pair<const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode>, Leaf> sit);
  // SaturatedGraph &get_saturated_graph(unsigned i);
  // SaturatedGraph &get_saturated_graph(std::shared_ptr<DecisionNode> decision);


  // Make a new decicion node for this execution
  // int new_decision_node();
  std::shared_ptr<DecisionNode> new_decision_node(std::shared_ptr<DecisionNode> parent);
  // Make a new decision node that could operate in parallel
  void construct_sibling(std::shared_ptr<DecisionNode>decision, const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf, Leaf l);


  std::shared_ptr<DecisionNode> get_root() {return root;};
  bool work_queue_empty();

protected:

  std::shared_ptr<DecisionNode> root;

  std::vector<std::shared_ptr<DecisionNode>> decisions;
  std::vector<std::shared_ptr<DecisionNode>> work_queue;

};


#endif