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

#include <config.h>
#ifndef __SATURATED_GRAPH_H__
#define __SATURATED_GRAPH_H__

#include "SymAddr.h"
#include "VClock.h"
#include "Option.h"

#include <vector>
#include <immer/vector.hpp>
#include <immer/flex_vector.hpp>
#include <immer/map.hpp>
#include <immer/box.hpp>
#include <immer/set.hpp>

class SaturatedGraph {
public:
  SaturatedGraph();

  /* write == -1 means init */
  enum EventKind {
    NONE,
    STORE,
    LOAD,
  };
  /* read_from is an event id.
   * Events must be added in program order
   */
  void add_event(unsigned pid, unsigned id, EventKind kind,
                 SymAddr addr, Option<unsigned> read_from,
                 const std::vector<unsigned> &happens_after);
  /* Returns true if the graph is still acyclic. */
  bool saturate();
  bool is_saturated() const { return wq_empty(); }
private:
  struct Event {
    Event() { abort(); }
    Event(EventKind kind, SymAddr addr, immer::vector<unsigned> read_froms,
          Option<unsigned> po_predecessor, immer::vector<unsigned> happens_after)
      : kind(kind), addr(addr), read_froms(std::move(read_froms)),
        po_predecessor(po_predecessor), happens_after(std::move(happens_after)) {};
    EventKind kind;
    SymAddr addr;
    /* If this is a write event, the events that read from us. If this
     * is a read event, either empty, meaning we read from init, or a
     * singleton vector of the event we read from.
     */
    immer::vector<unsigned> read_froms;
    Option<unsigned> po_predecessor;
    /* All but po and read-from edges, which are in po_predecessor and
     * read_froms, respectively.
     */
    immer::vector<unsigned> happens_after;
    /* All but read-from edges, which are in read_froms */
    immer::vector<unsigned> happens_before;
  };

  immer::map<unsigned,Event> events;
  immer::map<SymAddr,immer::vector<unsigned>> writes_by_address;
  immer::map<unsigned,unsigned> last_event_by_pid;
  typedef VClock<int> VC;
  immer::map<unsigned,immer::box<VClock<int>>> vclocks;

  VC recompute_vc_for_event(const Event &e) const;
  void add_successors_to_wq(const Event &e);

  immer::flex_vector<unsigned> wq_queue;
  immer::set<unsigned> wq_set;
  void wq_add(unsigned id);
  bool wq_empty() const;
  unsigned wq_pop();
};

#endif
