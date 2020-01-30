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

#include <unordered_set>
#include <mutex>

#include "TSOPSOTraceBuilder.h"

/* An identifier for a thread. An index into this->threads.
   *
   * Even indexes are for real threads. Odd indexes i are for
   * auxiliary threads corresponding to the real thread at index i-1.
   */
class RFSCUnfoldingTree final {
public:
  RFSCUnfoldingTree() {};

  struct UnfoldingNode;
  typedef llvm::SmallVector<std::weak_ptr<UnfoldingNode>,1> UnfoldingNodeChildren;

  static unsigned unf_ctr;

  std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> find_unfolding_node
  (RFSCUnfoldingTree::UnfoldingNodeChildren &parent_list,
    const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &parent,
    const std::shared_ptr<RFSCUnfoldingTree::UnfoldingNode> &read_from);

  // Workaround to access firstevent before restructure
  UnfoldingNodeChildren * first_event_parentlist(CPid cpid);

  struct UnfoldingNode {
  public:
    UnfoldingNode(std::shared_ptr<UnfoldingNode> parent,
                  std::shared_ptr<UnfoldingNode> read_from)
      : parent(std::move(parent)), read_from(std::move(read_from)),
        seqno(++RFSCUnfoldingTree::unf_ctr) {};
    std::shared_ptr<UnfoldingNode> parent, read_from;
    UnfoldingNodeChildren children;
    unsigned seqno;
  };

private:
  std::map<CPid,UnfoldingNodeChildren> first_events;
  std::mutex unfolding_tree_mutex;

};
#endif
