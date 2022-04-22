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
#include <numeric>

std::shared_ptr<DecisionNode> RFSCDecisionTree::new_decision_node
(std::shared_ptr<DecisionNode> parent,
 RFSCUnfoldingTree::NodePtr unf) {
  auto decision = std::make_shared<DecisionNode>(std::move(parent));
  decision->alloc_unf(std::move(unf));
  return decision;
}


void RFSCDecisionTree::construct_sibling
(const DecisionNode &decision,
 RFSCUnfoldingTree::NodePtr unf, Leaf l) {
  scheduler->enqueue(decision.make_sibling(std::move(unf), l));
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

PriorityQueueScheduler::PriorityQueueScheduler() : RFSCScheduler() {}

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

thread_local int WorkstealingPQScheduler::thread_id = 0;
WorkstealingPQScheduler::WorkstealingPQScheduler(unsigned num_threads)
  : work_queue(num_threads), halting(false) {}

void WorkstealingPQScheduler::enqueue(std::shared_ptr<DecisionNode> node) {
  outstanding_jobs.fetch_add(1, std::memory_order_relaxed);
  std::lock_guard<std::mutex> lock(work_queue[thread_id].mutex);
  work_queue[thread_id].push(std::move(node));
  /* Ouch, after going through the effort of fine-grained locking? */
  cv.notify_one();
}

std::shared_ptr<DecisionNode> WorkstealingPQScheduler::dequeue() {
  assert(thread_id >= 0 && std::size_t(thread_id) < work_queue.size());
  {
    std::lock_guard<std::mutex> lock(work_queue[thread_id].mutex);
    if (halting.load(std::memory_order_relaxed)) return nullptr;
    if (!work_queue[thread_id].empty())
      return work_queue[thread_id].pop();
  }

  /* We don't need to hold our own lock, because as long as we hold the
   * big lock, nobody else will access our queue. */
  std::unique_lock<std::mutex> lock(mutex);
  while (1) {
    if (halting.load(std::memory_order_relaxed)) return nullptr;
    if (!work_queue[thread_id].empty()) {
      return work_queue[thread_id].pop();
    }

    /* Simulate work-stealing algorithm */
    /* Generate array of other threads in random order */
    std::vector<int> others(work_queue.size()-1);
    std::iota(others.begin(), others.end(), 0);
    if (std::size_t(thread_id) != others.size()) others[thread_id] = others.size();
    std::random_shuffle(others.begin(), others.end());
    for(int other : others) {
      std::lock_guard<std::mutex> other_lock(work_queue[other].mutex);
      assert(other != thread_id);
      if (work_queue[thread_id].steal(work_queue[other])) {
        assert(!work_queue[thread_id].empty());
        return work_queue[thread_id].pop();
      }
    }
    cv.wait(lock);
  }
}

void WorkstealingPQScheduler::halt() {
  std::lock_guard<std::mutex> lock(mutex);
  halting.store(true, std::memory_order_relaxed);
  cv.notify_all();
}

std::shared_ptr<DecisionNode> WorkstealingPQScheduler::ThreadWorkQueue::pop() {
  assert(!empty());
  auto it = queue.end();
  --it;
  auto &depth_queue = it->second;
  assert(!depth_queue.empty());
  std::shared_ptr<DecisionNode> res = std::move(depth_queue.front());
  depth_queue.pop_front();
  if (depth_queue.empty()) queue.erase(it);
  return res;
}

bool WorkstealingPQScheduler::ThreadWorkQueue::steal(ThreadWorkQueue &other) {
  assert(empty());
  if (other.empty()) return false;
  auto oqit = other.queue.begin();
  /* Take half of the elements */
  auto &other_depth_queue = oqit->second;
  std::size_t count = (other_depth_queue.size()+1)/2;
  assert(count > 0);
  assert(count <= other_depth_queue.size());
  auto &my_depth_queue = queue[oqit->first];
  while (count--) {
    my_depth_queue.emplace_front(std::move(other_depth_queue.back()));
    other_depth_queue.pop_back();
  }
  if (other_depth_queue.empty())
    other.queue.erase(oqit);
  return true;
}

/******************************************************************************
 *
 *      DecisionNode
 *
 ******************************************************************************/

std::shared_ptr<DecisionNode> DecisionNode::make_sibling
(RFSCUnfoldingTree::NodePtr unf, Leaf l) const {
  return std::make_shared<DecisionNode>(parent, std::move(unf), l);
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
(const RFSCUnfoldingTree::NodePtr &unf) {
  std::lock_guard<std::mutex> lock(parent->decision_node_mutex);
  return parent->children_unf_set.insert(unf).second;
}


void DecisionNode::alloc_unf(RFSCUnfoldingTree::NodePtr unf) {
  std::lock_guard<std::mutex> lock(parent->decision_node_mutex);
#ifndef NDEBUG
  auto res =
#endif
      parent->children_unf_set.insert(std::move(unf));
  assert(res.second);
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
