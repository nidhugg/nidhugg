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

class SaturatedGraph final {
public:
  SaturatedGraph();

  typedef IID<int> ExtID;
  typedef unsigned Pid;

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
  void add_event(Pid pid, ExtID id, EventKind kind,
                 SymAddr addr, Option<ExtID> read_from,
                 const std::vector<ExtID> &happens_after);
  /* Returns true if the graph is still acyclic. */
  bool saturate();
  bool is_saturated() const { return wq_empty(); }
  bool has_event(ExtID id) const { return (extid_to_id.count(id) != 0); }

  void print_graph(std::ostream &o, std::function<std::string(ExtID)> event_str
                   = std::mem_fn(&IID<int>::to_string)) const;

  /* Accessors */
  bool event_is_store(ExtID id) const;
  SymAddr event_addr(ExtID id) const;
  typedef VClock<int> VC;
  const VC &event_vc(ExtID id) const;
  std::vector<ExtID> event_ids() const;
  std::vector<ExtID> event_in(ExtID id) const;
  std::size_t size() const { return events.size(); }

  void add_edge(ExtID from, ExtID to);

private:
  typedef unsigned ID;
  void add_edge_internal(ID from, ID to);

  struct Event {
    Event() { abort(); }
    Event(IID<Pid> iid, ExtID ext_id, bool is_load, bool is_store, SymAddr addr,
          Option<ID> read_from, immer::vector<ID> readers,
          Option<ID> po_predecessor)
      : iid(iid), ext_id(ext_id), is_load(is_load), is_store(is_store), addr(addr),
        read_from(read_from), readers(std::move(readers)),
        po_predecessor(po_predecessor) {};
    IID<Pid> iid;
    ExtID ext_id;
    bool is_load;
    bool is_store;
    SymAddr addr;
    /* Either empty, meaning we read from init, or a singleton vector of
     * the event we read from.
     */
    Option<ID> read_from;
    /* The events that read from us. */
    immer::vector<ID> readers;
    Option<ID> po_predecessor;
  };

  immer::map<ExtID,ID> extid_to_id;
  immer::vector<Event> events;
  immer::vector<immer::vector<ID>> ins;
  immer::vector<immer::vector<ID>> outs;
  immer::map<SymAddr,immer::map<Pid,ID>> writes_by_address;
  immer::map<SymAddr,immer::vector<ID>> reads_from_init;
  immer::vector<immer::vector<ID>> events_by_pid;
  immer::vector<immer::box<VClock<int>>> vclocks;
  immer::vector<immer::map<SymAddr,immer::vector<ID>>>
    writes_by_process_and_address;
  unsigned saturated_until;

  void add_edges(const std::vector<std::pair<ID,ID>> &);
  ID get_process_event(Pid pid, unsigned index) const;
  Option<ID> maybe_get_process_event(Pid pid, unsigned index) const;
  VC initial_vc_for_event(IID<Pid> iid) const;
  VC initial_vc_for_event(const Event &e) const;
  VC recompute_vc_for_event(const Event &e, const immer::vector<ID> &in) const;
  void add_successors_to_wq(ID id, const Event &e);
  bool is_in_cycle(const Event &e, const immer::vector<ID> &in, const VC &vc) const;
  Option<ID> po_successor(ID id, const Event &e) const;
  VC top() const;
  struct care {
    care(unsigned e_sz) : set(e_sz) {}
    std::vector<bool> set;
    std::vector<ID> vec;
  };
  struct care reverse_care_set() const;
  void add_to_care(struct care &, unsigned) const;
  void reverse_saturate();

  template <typename Fn> void foreach_succ(ID id, const Event &e, Fn f) const;

#ifndef NDEBUG
  void check_graph_consistency() const;
#else
  void check_graph_consistency() const {};
#endif

  immer::flex_vector<ID> wq_queue;
  immer::set<ID> wq_set;
  void wq_add(ID id);
  void wq_add_first(ID id);
  bool wq_empty() const;
  ID wq_pop();
};

#endif
