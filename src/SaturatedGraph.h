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
    RMW,
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
  bool has_event(unsigned id) const {return (events.count(id)  != 0); }

  void print_graph(std::ostream &o, std::function<std::string(unsigned)> event_str
                   = (std::string(&)(unsigned))std::to_string) const;

  /* Accessors */
  bool event_is_store(unsigned id) const;
  SymAddr event_addr(unsigned id) const;
  typedef VClock<int> VC;
  const VC &event_vc(unsigned id) const;
  std::vector<unsigned> event_ids() const;
  std::vector<unsigned> event_in(unsigned id) const;

  void add_edge(unsigned from, unsigned to);

private:
  struct Event {
    Event() { abort(); }
    Event(IID<unsigned> iid, bool is_load, bool is_store, SymAddr addr,
          Option<unsigned> read_from, immer::vector<unsigned> readers,
          Option<unsigned> po_predecessor, immer::vector<unsigned> in,
          immer::vector<unsigned> out)
      : iid(iid), is_load(is_load), is_store(is_store), addr(addr),
        read_from(read_from), readers(std::move(readers)),
        po_predecessor(po_predecessor), in(std::move(in)),
        out(std::move(out)) {};
    IID<unsigned> iid;
    bool is_load;
    bool is_store;
    SymAddr addr;
    /* Either empty, meaning we read from init, or a singleton vector of
     * the event we read from.
     */
    Option<unsigned> read_from;
    /* The events that read from us. */
    immer::vector<unsigned> readers;
    Option<unsigned> po_predecessor;
    /* All but po and read-from edges, which are in po_predecessor and
     * read_from, respectively.
     */
    immer::vector<unsigned> in;
    /* All but read-from edges, which are in readers */
    immer::vector<unsigned> out;
  };

  immer::map<unsigned,Event> events;
  immer::map<SymAddr,immer::vector<unsigned>> writes_by_address;
  immer::map<SymAddr,immer::vector<unsigned>> reads_from_init;
  immer::map<unsigned,immer::vector<unsigned>> events_by_pid;
  immer::map<unsigned,immer::box<VClock<int>>> vclocks;

  void add_edges(const std::vector<std::pair<unsigned,unsigned>> &);
  unsigned get_process_event(unsigned pid, unsigned index) const;
  VC initial_vc_for_event(IID<unsigned> iid) const;
  VC initial_vc_for_event(const Event &e) const;
  VC recompute_vc_for_event(const Event &e) const;
  void add_successors_to_wq(const Event &e);
  bool is_in_cycle(const Event &e, const VC &vc) const;

#ifndef NDEBUG
  void check_graph_consistency() const;
#else
  void check_graph_consistency() const {};
#endif

  immer::flex_vector<unsigned> wq_queue;
  immer::set<unsigned> wq_set;
  void wq_add(unsigned id);
  void wq_add_first(unsigned id);
  bool wq_empty() const;
  unsigned wq_pop();
};

#endif
