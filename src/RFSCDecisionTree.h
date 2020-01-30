/* Copyright (C) 2019 Alexis Remmers and Nodari Kankava
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
#include <queue>
#include <atomic>


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


struct DecisionNode {
public:
  /* Empty constructor for root. */
  DecisionNode() : parent(nullptr), depth(-1), pruned_subtree(false) {}
  /* Constructor for new nodes during compute_unfolding. */
  DecisionNode(std::shared_ptr<DecisionNode> decision)
        : parent(std::move(decision)), depth(decision->depth+1),
          pruned_subtree(false) {}
  /* Constructor for new siblings during compute_prefixes. */
  DecisionNode(std::shared_ptr<DecisionNode> decision,
               std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> unf, Leaf l)
        : parent(std::move(decision)), depth(decision->depth+1),
          unfold_node(std::move(unf)), leaf(l), pruned_subtree(false) {}

  /* The depth in the tree. */
  int depth;

  /* The UnfoldingNode of a new sibling. */
  std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> unfold_node;

  /* The Leaf of a new sibling. */
  Leaf leaf;

  /* Tries to allocate a given UnfoldingNode.
   * Returns false if it previously been allocated by this node or any previous
   * sibling. */
  bool try_alloc_unf(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf);

  /* Constructs a sibling and inserts in in the sibling-set. */
  std::shared_ptr<DecisionNode> make_sibling
  (const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf, Leaf l);

  /* Returns a given nodes SaturatedGraph, or reuses an ancestors graph if none exist. */
  SaturatedGraph &get_saturated_graph(bool &complete);

  static const std::shared_ptr<DecisionNode> &get_ancestor
  (const DecisionNode * node, int wanted);

  /* Using the last decision that caused a failure, and then
   * prune all later decisions. */
  void prune_decisions();

  /* True if node is part of a pruned subtree. */
  bool is_pruned();

private:

  std::shared_ptr<DecisionNode> parent;

  /* Defines if the subtree should be evaluated or not.
   * Set to true if prune_decision noted that all leafs with this node as
   * ancestor should not be explored. */
  std::atomic_bool pruned_subtree;


  // The following fields are held by a parent to be accessed by every child.

  /* Set of all known UnfoldingNodes from every child nodes' evaluation. */
  std::unordered_set<std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode>>
  children_unf_set;

  SaturatedGraph graph_cache;

  /* mutex to ensure exclusive access to a nodes' unfolding-set and saturated graph. */
  std::mutex decision_node_mutex;
};


/* Comparator to define RFSCDecisionTree priority queue ordering.
 * This is operated in a depth-first ordering, meaning it will prioritise to
 * exhaust the exploration of the lowest subtrees first so that they could be
 * garbage-collected faster. */
class DecisionCompare {
public:
  bool operator()(const std::shared_ptr<DecisionNode> &a,
                  const std::shared_ptr<DecisionNode> &b) const {
    return a->depth < b->depth;
  }
};


class RFSCDecisionTree final {
public:
  RFSCDecisionTree() {
    // Initiallize the work_queue with a "root"-node
    work_queue.push(std::make_shared<DecisionNode>());
  };


  /* Backtracks a TraceBuilders DecisionNode up to an ancestor with not yet
   * evaluated sibling. */
  void backtrack_decision_tree(std::shared_ptr<DecisionNode> *TB_work_item);

  /* Replace the current work item with a new DecisionNode */
  std::shared_ptr<DecisionNode> get_next_work_task();

  /* Constructs an empty Decision node. */
  std::shared_ptr<DecisionNode> new_decision_node
  (const std::shared_ptr<DecisionNode> &parent,
   const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf);

  /* Constructs a sibling Decision node and add it to work queue. */
  void construct_sibling
  (const std::shared_ptr<DecisionNode> &decision,
   const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf, Leaf l);


  /* True if no unevaluated siblings have been found. */
  bool work_queue_empty();

  /* Given a DecisionNode whose depth >= to wanted, returns a parent with the wanted depth. */
  static const std::shared_ptr<DecisionNode> &find_ancestor
  (const std::shared_ptr<DecisionNode> &node, int wanted);

private:

  /* Exclusive access to the work_queue. */
  static std::mutex work_queue_mutex;

  /* Work queue of leaf nodes to explore.
   * The ordering is determined by DecisionCompare. */
  std::priority_queue<std::shared_ptr<DecisionNode>,
                      std::vector<std::shared_ptr<DecisionNode>>,
                      DecisionCompare> work_queue;
};


#endif
