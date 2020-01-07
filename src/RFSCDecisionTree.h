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

#include "SymEv.h"
#include "SaturatedGraph.h"
#include "RFSCUnfoldingTree.h"

#include <unordered_set>
#include <mutex>


struct DecisionNode;

struct Branch {
public:
  Branch(int pid, int size, int decision_depth, bool pinned, SymEv sym)
    : pid(pid), size(size), decision_depth(decision_depth), pinned(pinned),
      sym(std::move(sym)) {}
  Branch() : Branch(-1, 0, -1, false, {}) {}
  int pid, size, decision_depth;
  bool pinned;
  SymEv sym;
};


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


struct DecisionNode {
public:
  /* Empty constructor for root. */
  DecisionNode() : parent(nullptr), depth(-1), name("ROOT"), name_index("A") { decision_id = 0;}
  /* Constructor for new nodes during compute_unfolding. */
  DecisionNode(std::shared_ptr<DecisionNode> decision)
        : parent(decision), depth(decision->depth+1), ID(++decision_id), name_index("A") {
      set_name();
    };
  /* Constructor for new siblings during compute_prefixes. */
  DecisionNode(std::shared_ptr<DecisionNode> decision, std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> unf, Leaf l)
        : parent(decision), depth(decision->depth+1), ID(++decision_id), unfold_node(std::move(unf)), leaf(l), name_index("A") {
      set_sibling_name();
  };


  /* The depth in the tree. */
  int depth;

  /* Numerical unique ID for each decision-node. */
  unsigned int ID;

  /* String-represenation of a node, illustrating the entire ancestry from root. */
  std::string name;
  std::string name_index;

  /* The UnfoldingNode of a new sibling. */
  std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> unfold_node;

  /* The Leaf of a new sibling. */
  Leaf leaf;

  /* True if the given UnfoldingNode has previously been allocated by this node or any previous sibling. */
  bool unf_is_known(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf);

  /* Inserts the UnfoldingNode into the known set*/
  void alloc_unf(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf);


  /* Places an UnfoldingNode into the known unfolding-set. */
  void place_decision_into_sleepset(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf);
  

  /* Constructs a sibling and inserts in in the sibling-set. */
  std::shared_ptr<DecisionNode> make_sibling(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf, Leaf l);

  /* Returns a given nodes SaturatedGraph, or reuses an ancestors graph if none exist. */
  SaturatedGraph &get_saturated_graph();


  /* These are exposed to be operated by RFSCDecisionTree, should not be used externally. */

  std::shared_ptr<DecisionNode> parent;


  std::unordered_set<std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode>> & get_unf_set() {
    return parent->children_unf_set;
  };
  

protected:

  /* Initialize a nodes' string representation */
  void set_name();
  void set_sibling_name();

  // The following fields are held by a parent to be accessed by every child.
  
  /* Set of all known UnfoldingNodes from every child nodes' evaluation. */
  std::unordered_set<std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode>> children_unf_set;
  
  SaturatedGraph graph_cache;
  std::mutex decision_node_mutex;
};

class RFSCDecisionTree final {
public:
  RFSCDecisionTree() : root(std::make_shared<DecisionNode>()) {};

  /* Using the last decision that caused a failure, and then
   * prune all later decisions. */
  void prune_decisions(std::shared_ptr<DecisionNode> blame);

  /* Backtracks a TraceBuilders DecisionNode up to an ancestor with not yet evaluated sibling. */
  void backtrack_decision_tree(std::shared_ptr<DecisionNode> *TB_work_item);

  /* Replace the current work item with a new DecisionNode */
  std::shared_ptr<DecisionNode> get_next_work_task();  

  /* Constructs an empty Decision node. */
  std::shared_ptr<DecisionNode> new_decision_node(std::shared_ptr<DecisionNode> parent, const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf);

  /* Constructs a sibling Decision node and add it to work queue. */
  void construct_sibling(std::shared_ptr<DecisionNode>decision, const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf, Leaf l);

  /* Returns the root of the global decision tree. */
  std::shared_ptr<DecisionNode> get_root() {return root;};

  /* True if no unevaluated siblings have been found. */
  bool work_queue_empty();

  /* Given a DecisionNode whose depth >= to wanted, returns a parent with the wanted depth. */
  static std::shared_ptr<DecisionNode> find_ancestor(std::shared_ptr<DecisionNode> node, int wanted);


protected:

  std::shared_ptr<DecisionNode> root;

  std::vector<std::shared_ptr<DecisionNode>> work_queue;
  static std::mutex decision_tree_mutex;
};


#endif