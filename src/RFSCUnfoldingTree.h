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
#ifndef __RFSC_UNFOLDING_TREE_H__
#define __RFSC_UNFOLDING_TREE_H__

#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_set>
#include <utility>

#include "TSOPSOTraceBuilder.h"
#include "Seqno.h"

/* Representation of a set of program events from any number of
 * executions where each event is uniquely identified by its thread,
 * program-order predecessor event (parent), and possibly a write event
 * that is read from (read_from).
 *
 * Each program event is represented by a unique UnfoldingNode in
 * memory, so that pointer comparison suffices as an equality check
 * between events.
 *
 * Nodes are memory-managed with reference counting and lookup with
 * find_unfolding_node is thread-safe.
 */
class RFSCUnfoldingTree final {
public:
  RFSCUnfoldingTree() {}

  struct UnfoldingNode;
  typedef std::shared_ptr<const UnfoldingNode> NodePtr;

 private:
  friend struct UnfoldingNode;
  typedef llvm::SmallVector<std::weak_ptr<const UnfoldingNode>,1> UnfoldingNodeChildren;

 public:
  static SeqnoRoot unf_ctr_root;
  static thread_local Seqno unf_ctr;

  /* Represents uniquely a program event. Must be created by calling
   * RFSCUnfoldingTree::find_unfolding_node() */
  struct UnfoldingNode {
    NodePtr parent, read_from;
    /* Do not call directly, use
     * RFSCUnfoldingTree::find_unfolding_node() */
    UnfoldingNode(NodePtr parent, NodePtr read_from)
      : parent(std::move(parent)), read_from(std::move(read_from)),
        seqno(++RFSCUnfoldingTree::unf_ctr) {}
  private:
    /* The children of this node. Used by get_or_create. */
    mutable UnfoldingNodeChildren children;
    /* Protects children, the only mutable field */
    mutable std::mutex mutex;
    friend class RFSCUnfoldingTree;
  public:
    /* Only for the purpose of nicer debug printing, do not use for
     * anything else. */
    unsigned seqno;
  };

  /* Returns the node representing the event uniquely identified by
   * (cpid, parent, read_from). parent, if non-null, must also be of
   * thread cpid. */
  NodePtr find_unfolding_node
    (const CPid &cpid, const NodePtr &parent, const NodePtr &read_from);

 private:
  NodePtr get_or_create(UnfoldingNodeChildren &parent_list,
                        const NodePtr &parent, const NodePtr &read_from);

  /* The first events of some thread */
  struct UnfoldingRoot {
    UnfoldingNodeChildren children;
    std::mutex mutex;
  };

  UnfoldingRoot &get_unfolding_root(const CPid &cpid);

  std::map<CPid,UnfoldingRoot> first_events;
  std::shared_timed_mutex unfolding_tree_mutex;
};
#endif
