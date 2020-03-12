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

#include "Debug.h"
#include "RFSCDecisionTree.h"

std::shared_ptr<DecisionNode> RFSCDecisionTree::new_decision_node
(const std::shared_ptr<DecisionNode> &parent,
 const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf) {
  auto decision = std::make_shared<DecisionNode>(parent);
  decision->try_alloc_unf(unf);
  return std::move(decision);
}


void RFSCDecisionTree::construct_sibling
(const std::shared_ptr<DecisionNode> &decision,
 const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf, Leaf l) {
  scheduler->enqueue(decision->make_sibling(unf, l));
}


const std::shared_ptr<DecisionNode> &RFSCDecisionTree::find_ancestor
(const std::shared_ptr<DecisionNode> &node, int wanted) {
  assert(node->depth >= wanted);

  /* Ugly workaround to traverse the tree without updating the ref_count
   * while at the same time return a shared pointer to correct object. */
  if (node->depth == wanted) {
    return node;
  }
  return DecisionNode::get_ancestor(node.get(), wanted);
}

RFSCScheduler::~RFSCScheduler() = default;

PriorityQueueScheduler::PriorityQueueScheduler() : RFSCScheduler() {};

void PriorityQueueScheduler::enqueue(std::shared_ptr<DecisionNode> node) {
  outstanding_jobs.fetch_add(1, std::memory_order_relaxed);
  std::lock_guard<std::mutex> lock(mutex);
  work_queue.emplace(std::move(node));
  cv.notify_one();
}

std::shared_ptr<DecisionNode> PriorityQueueScheduler::dequeue() {
  std::unique_lock<std::mutex> lock(mutex);
  while (!halting && work_queue.empty()) {
    cv.wait(lock);
  }
  if (halting) return nullptr;
  std::shared_ptr<DecisionNode> node = work_queue.top();
  work_queue.pop();
  return node;
}

void PriorityQueueScheduler::halt() {
  std::lock_guard<std::mutex> lock(mutex);
  halting = true;
  cv.notify_all();
}

/******************************************************************************
 *
 *      DecisionNode
 *
 ******************************************************************************/

std::shared_ptr<DecisionNode> DecisionNode::make_sibling
(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf, Leaf l) {
  return std::make_shared<DecisionNode>(parent, unf, l);
}


const SaturatedGraph &DecisionNode::get_saturated_graph(std::function<void(SaturatedGraph&)> construct) {
  SaturatedGraph &g = parent->graph_cache;
  if (parent->cache_initialised.load(std::memory_order_acquire)) {
    assert(g.size() || depth == 0);
    return g;
  }
  std::lock_guard<std::mutex> lock(parent->decision_node_mutex);
  // SaturatedGraph &g = parent->graph_cache;
  if (parent->cache_initialised.load(std::memory_order_relaxed)) {
    assert(g.size() || depth == 0);
    return g;
  }
  assert(depth > 0 && !g.size());
  auto node = parent;
  do {
    /* TODO: This graph could be initialising; if so, we should block on
     * the lock instead of proceeding upwards.
     */
    if (node->cache_initialised.load(std::memory_order_acquire)) {
      /* Reuse subgraph */
      g = node->graph_cache.clone();
      break;
    }
    node = node->parent;
    assert(node);
  } while (node->depth != -1);

  construct(g);

  parent->cache_initialised.store(true, std::memory_order_release);
  return g;
}


bool DecisionNode::try_alloc_unf
(const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &unf) {
  std::lock_guard<std::mutex> lock(parent->decision_node_mutex);
  return parent->children_unf_set.insert(unf).second;
}


void DecisionNode::prune_decisions() {
  pruned_subtree.store(true, std::memory_order_release);
}


bool DecisionNode::is_pruned() {
  for (auto node = this; node->depth != -1; node = node->parent.get()) {
    if (node->pruned_subtree.load(std::memory_order_acquire))
      return true;
  }
  return false;
}


const std::shared_ptr<DecisionNode> &
DecisionNode::get_ancestor(const DecisionNode * node, int wanted) {
  while (node->parent->depth != wanted) {
    node = node->parent.get();
  }
  return node->parent;
}
