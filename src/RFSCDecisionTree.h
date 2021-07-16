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
#include <condition_variable>
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
  Leaf() : prefix() { assert(is_bottom()); }
  /* Construct a prefix leaf. */
  Leaf(std::vector<Branch> prefix) : prefix(std::move(prefix)) {
    /* Ensure we encode a non-bottom leaf. */
    if (is_bottom()) {
      this->prefix.reserve(1);
    }
    assert(!is_bottom());
  }
  std::vector<Branch> prefix;

  bool is_bottom() const { return prefix.capacity() == 0; }
};


struct DecisionNode {
public:
  /* Constructor for root node. */
  DecisionNode() : depth(-1), leaf(std::vector<Branch>()), parent(nullptr),
                   pruned_subtree(false), cache_initialised(true) {}
  /* Constructor for new nodes during compute_unfolding. */
  DecisionNode(std::shared_ptr<DecisionNode> decision)
    : depth(decision->depth+1), pruned_subtree(false),
      cache_initialised(false) {
    parent = std::move(decision);
  }
  /* Constructor for new siblings during compute_prefixes. */
  DecisionNode(std::shared_ptr<DecisionNode> decision,
               std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> unf, Leaf l)
    : depth(decision->depth+1), unfold_node(std::move(unf)), leaf(l),
      pruned_subtree(false), cache_initialised(false) {
    parent = std::move(decision);
  }

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
  /* Allocates the given UnfoldingNode, assuming it has not been allocated before */
  void alloc_unf(std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> unf);

  /* Constructs a sibling and inserts in in the sibling-set. */
  std::shared_ptr<DecisionNode> make_sibling
  (std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> unf, Leaf l) const;

  /* Returns a given nodes SaturatedGraph, or reuses an ancestors graph if none exist. */
  const SaturatedGraph &get_saturated_graph(std::function<void(SaturatedGraph&)>);

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

  /* Wether the graph cache has been initialised */
  std::atomic_bool cache_initialised;

  // The following fields are held by a parent to be accessed by every child.

  /* Set of all known UnfoldingNodes from every child nodes' evaluation. */
  std::unordered_set<std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode>>
  children_unf_set;

  /* Cached SaturatedGraph containing all events up to this one */
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

struct RFSCScheduler {
  virtual ~RFSCScheduler();
  virtual void enqueue(std::shared_ptr<DecisionNode> node) = 0;
  virtual std::shared_ptr<DecisionNode> dequeue() = 0;
  virtual void halt() = 0;
  virtual void register_thread(unsigned tid){}
  std::atomic<uint64_t> outstanding_jobs{0};
};

class PriorityQueueScheduler final : public RFSCScheduler {
public:
  PriorityQueueScheduler();
  ~PriorityQueueScheduler() override = default;
  void enqueue(std::shared_ptr<DecisionNode> node) override;
  std::shared_ptr<DecisionNode> dequeue() override;
  void halt() override;
private:
  /* Exclusive access to the work_queue. */
  std::mutex mutex;
  std::condition_variable cv;

  /* Set to indicate that no more jobs should be dispatched */
  bool halting = false;

  /* Work queue of leaf nodes to explore.
   * The ordering is determined by DecisionCompare. */
  std::priority_queue<std::shared_ptr<DecisionNode>, std::vector<std::shared_ptr<DecisionNode>>, DecisionCompare> work_queue;
};

class WorkstealingPQScheduler final : public RFSCScheduler {
public:
  WorkstealingPQScheduler(unsigned num_threads);
  ~WorkstealingPQScheduler() override = default;
  void enqueue(std::shared_ptr<DecisionNode> node) override;
  std::shared_ptr<DecisionNode> dequeue() override;
  void halt() override;
  void register_thread(unsigned id) override {
    assert(id >= 0 && id < work_queue.size());
    thread_id = id;
  }

private:
  class alignas(64) ThreadWorkQueue {
    std::map<int,std::deque<std::shared_ptr<DecisionNode>>> queue;
  public:
    void push(std::shared_ptr<DecisionNode> ptr) {
      assert(ptr);
      queue[ptr->depth].emplace_back(std::move(ptr));
    }
    std::shared_ptr<DecisionNode> pop();
    bool empty() const { return queue.empty(); }
    bool steal(ThreadWorkQueue &other);
    std::mutex mutex;
  };

  std::mutex mutex;
  std::condition_variable cv;
  std::vector<ThreadWorkQueue> work_queue;
  static thread_local int thread_id;
  std::atomic<bool> halting;
};

class RFSCDecisionTree final {
public:
  RFSCDecisionTree(std::unique_ptr<RFSCScheduler> scheduler)
    : scheduler(std::move(scheduler)) {
    // Initiallize the work_queue with a "root"-node
    this->scheduler->enqueue(std::make_shared<DecisionNode>());
  };


  /* Backtracks a TraceBuilders DecisionNode up to an ancestor with not yet
   * evaluated sibling. */
  void backtrack_decision_tree(std::shared_ptr<DecisionNode> *TB_work_item);

  /* Get a new prefix to execute from the scheduler */
  std::shared_ptr<DecisionNode> get_next_work_task() { return scheduler->dequeue(); }

  /* Constructs an empty Decision node. */
  std::shared_ptr<DecisionNode> new_decision_node
  (std::shared_ptr<DecisionNode> parent,
   std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> unf);

  /* Constructs a sibling Decision node and add it to work queue. */
  void construct_sibling
  (const DecisionNode &decision,
   std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> unf, Leaf l);

  /* Given a DecisionNode whose depth >= to wanted, returns a parent with the wanted depth. */
  static const std::shared_ptr<DecisionNode> &find_ancestor
  (const std::shared_ptr<DecisionNode> &node, int wanted);

  RFSCScheduler &get_scheduler() { return *scheduler; }

private:
  std::unique_ptr<RFSCScheduler> scheduler;
};


#endif
