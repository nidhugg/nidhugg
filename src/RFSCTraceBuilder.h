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
#ifndef __RFSC_TRACE_BUILDER_H__
#define __RFSC_TRACE_BUILDER_H__

#include "TSOPSOTraceBuilder.h"
#include "VClock.h"
#include "SymEv.h"
#include "WakeupTrees.h"
#include "Option.h"
#include "SaturatedGraph.h"
#include "RFSCUnfoldingTree.h"
#include "RFSCDecisionTree.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/container/flat_map.hpp>

class RFSCTraceBuilder final : public TSOPSOTraceBuilder{
public:
  RFSCTraceBuilder(RFSCDecisionTree &desicion_tree_,
                   RFSCUnfoldingTree &unfolding_tree_,
                   const Configuration &conf = Configuration::default_conf);
  ~RFSCTraceBuilder() override;
  bool schedule(int *proc, int *aux, int *alt, bool *dryrun) override;
  void refuse_schedule() override;
  void mark_available(int proc, int aux = -1) override;
  void mark_unavailable(int proc, int aux = -1) override;
  void cancel_replay() override;
  bool is_replaying() const override;
  void metadata(const llvm::MDNode *md) override;
  bool sleepset_is_empty() const override;
  bool check_for_cycles() override;
  Trace *get_trace() const override;
  bool reset() override;
  IID<CPid> get_iid() const override;

  void debug_print() const override;
  bool cond_branch(bool cnd) override { return true; }

  NODISCARD bool spawn() override;
  NODISCARD bool store(const SymData &ml) override;
  NODISCARD bool atomic_store(const SymData &ml) override;
  NODISCARD bool atomic_rmw(const SymData &ml, RmwAction action) override;
  NODISCARD bool compare_exchange
  (const SymData &sd, const SymData::block_type expected, bool success) override;
  NODISCARD bool load(const SymAddrSize &ml) override;
  NODISCARD bool full_memory_conflict() override;
  NODISCARD bool fence() override;
  NODISCARD bool join(int tgt_proc) override;
  NODISCARD bool mutex_lock(const SymAddrSize &ml) override;
  NODISCARD bool mutex_lock_fail(const SymAddrSize &ml) override;
  NODISCARD bool mutex_trylock(const SymAddrSize &ml) override;
  NODISCARD bool mutex_unlock(const SymAddrSize &ml) override;
  NODISCARD bool mutex_init(const SymAddrSize &ml) override;
  NODISCARD bool mutex_destroy(const SymAddrSize &ml) override;
  NODISCARD bool cond_init(const SymAddrSize &ml) override;
  NODISCARD bool cond_signal(const SymAddrSize &ml) override;
  NODISCARD bool cond_broadcast(const SymAddrSize &ml) override;
  NODISCARD bool cond_wait(const SymAddrSize &cond_ml,
                           const SymAddrSize &mutex_ml) override;
  NODISCARD bool cond_awake(const SymAddrSize &cond_ml,
                            const SymAddrSize &mutex_ml) override;
  NODISCARD int cond_destroy(const SymAddrSize &ml) override;
  NODISCARD bool register_alternatives(int alt_count) override;
  long double estimate_trace_count() const override;

  /* Perform planning of future executions. Requires the trace to be
   * maximal or sleepset blocked.
   */
  void compute_prefixes();

  /* Amount of siblings found during compute_prefixes. */
  int tasks_created;

  /* Active work item, signifies the leaf of an exploration.*/
  std::shared_ptr<DecisionNode> work_item;

protected:
  /* An identifier for a thread. An index into this->threads.
   *
   * Even indexes are for real threads. Odd indexes i are for
   * auxiliary threads corresponding to the real thread at index i-1.
   */
  typedef int IPid;

  /* An Access is a pair (tp,ml) representing an access to
   * memory. Accesses come in two varieties:
   *
   * (W_ALL_MEMORY,0) is considered as a store access to all memory.
   *
   * (tp,ml) with tp in {R,W}, is a Read or Write access to the byte
   * indicated by the pointer ml.
   */
  class Access{
  public:
    /* The type of memory access. */
    enum Type {R, W, W_ALL_MEMORY, NA} type;
    /* The accessed byte. */
    const void *ml;
    bool operator<(const Access &a) const{
      return type < a.type || (type == a.type && ml < a.ml);
    }
    bool operator==(const Access &a) const{
      return type == a.type && (type == NA || ml == a.ml);
    }
    Access() : type(NA), ml(0) {}
    Access(Type t, const void *m) : type(t), ml(m) {}
  };

  /* Various information about a thread in the current execution. */
  class Thread{
  public:
    Thread(const CPid &cpid, int spawn_event)
      : cpid(cpid), available(true), spawn_event(spawn_event) {}
    CPid cpid;
    /* Is the thread available for scheduling? */
    bool available;
    /* The index of the spawn event that created this thread, or -1 if
     * this is the main thread or one of its auxiliaries.
     */
    int spawn_event;
    /* Indices in prefix of the events of this process.
     */
    std::vector<unsigned> event_indices;

    /* The iid-index of the last event of this thread, or 0 if it has not
     * executed any events yet.
     */
    int last_event_index() const { return event_indices.size(); }
  };

  RFSCUnfoldingTree &unfolding_tree;

  /* The threads in the current execution, in the order they were
   * created. Threads on even indexes are real, threads on odd indexes
   * i are the auxiliary threads corresponding to the real threads at
   * index i-1.
   */
  std::vector<Thread> threads;

  Option<unsigned> po_predecessor(unsigned i) const{
    if (prefix[i].iid.get_index() == 1) return nullptr;
    return find_process_event(prefix[i].iid.get_pid(), prefix[i].iid.get_index()-1);
  }

  /* The CPids of threads in the current execution. */
  CPidSystem CPS;

  /* A ByteInfo object contains information about one byte in
   * memory. In particular, it recalls which events have recently
   * accessed that byte.
   */
  class ByteInfo{
  public:
    ByteInfo() : last_update(-1) {}
    /* An index into prefix, to the latest update that accessed this
     * byte. last_update == -1 if there has been no update to this
     * byte.
     */
    int last_update;
  };
  std::map<SymAddr,ByteInfo> mem;
  /* Index into prefix pointing to the latest full memory conflict.
   * -1 if there has been no full memory conflict.
   */
  int last_full_memory_conflict;

  /* A Mutex represents a pthread_mutex_t object.
   */
  class Mutex{
  public:
    Mutex() : last_access(-1), last_lock(-1), locked(false) {}
    Mutex(int lacc) : last_access(lacc), last_lock(-1), locked(false) {}
    int last_access;
    int last_lock;
    bool locked;
  };
  /* A map containing all pthread mutex objects in the current
   * execution. The key is the position in memory of the actual
   * pthread_mutex_t object.
   */
  std::map<SymAddr,Mutex> mutexes;

  /* A CondVar represents a pthread_cond_t object. */
  class CondVar{
  public:
    CondVar() : last_signal(-1) {}
    CondVar(int init_idx) : last_signal(init_idx) {}
    /* Index in prefix of the latest call to either of
     * pthread_cond_init, pthread_cond_signal, or
     * pthread_cond_broadcast for this condition variable.
     *
     * -1 if there has been no such call.
     */
    int last_signal;
    /* For each thread which is currently waiting for this condition
     * variable, waiters contains the index into prefix of the event
     * where the thread called pthread_cond_wait and started waiting.
     */
    std::vector<int> waiters;
  };
  /* A map containing all pthread condition variable objects in the
   * current execution. The key is the position in memory of the
   * actual pthread_cond_t object.
   */
  std::map<SymAddr,CondVar> cond_vars;

  /* All currently deadlocked threads */
  boost::container::flat_map<SymAddr, std::vector<IPid>> mutex_deadlocks;

  /* Information about a (short) sequence of consecutive events by the
   * same thread. At most one event in the sequence may have conflicts
   * with other events, and if the sequence has a conflicting event,
   * it must be the first event in the sequence.
   */
  class Event{
  public:
    Event(const IID<IPid> &iid, int alt = 0, SymEv sym = {})
      : alt(0), size(1), pinned(false),
        iid(iid), origin_iid(iid), md(0), clock(), may_conflict(false),
        sym(std::move(sym)), sleep_branch_trace_count(0), decision_depth(-1) {}
    /* Some instructions may execute in several alternative ways
     * nondeterministically. (E.g. malloc may succeed or fail
     * nondeterministically if Configuration::malloy_may_fail is set.)
     * Branch::alt is the index of the alternative for the first event
     * in this event sequence. The default execution alternative has
     * index 0. All events in this sequence, except the first, are
     * assumed to run their default execution alternative.
     */
    int alt;
    /* The number of events in this sequence. */
    int size;
    /* If this event should have its read-from preserved. */
    bool pinned;
    /* The identifier for the first event in this event sequence. */
    IID<IPid> iid;
    /* The IID of the program instruction which is the origin of this
     * event. For updates, this is the IID of the corresponding store
     * instruction. For other instructions origin_iid == iid.
     */
    IID<IPid> origin_iid;
    /* Metadata corresponding to the first event in this sequence. */
    const llvm::MDNode *md;
    /* The clock of the first event in this sequence. Only computed
     * after a full execution sequence has been explored.
     * above_clock excludes reversible incoming edges, and below_clock
     * encodes happens-before rather than happens-after.
     */
    VClock<IPid> clock, above_clock;
    /* Indices into prefix of events that happen before this one. */
    std::vector<unsigned> happens_after;
    /* Is it possible for any event in this sequence to have a
     * conflict with another event?
     */
    bool may_conflict;

    /* The unfolding event corresponding to this executed event. */
    RFSCUnfoldingTree::NodePtr event;

    Option<int> read_from;
    /* Symbolic representation of the globally visible operation of this event.
     * Empty iff !may_conflict
     */
    SymEv sym;
    /* For each previous IID that has been explored at this position
     * with the exact same prefix, some number of traces (both sleep
     * set blocked and otherwise) have been
     * explored. sleep_branch_trace_count is the total number of such
     * explored traces.
     */
    int sleep_branch_trace_count;

    /* Pointer to the corresponding DecisionNode. */
    std::shared_ptr<DecisionNode> decision_ptr;

    int get_decision_depth() const {
      return decision_depth;
    }
    void set_decision(std::shared_ptr<DecisionNode> decision) {
      decision_ptr = std::move(decision);
      decision_depth = decision_ptr ? decision_ptr->depth : -1;
    }
    void set_branch_decision(int decision, const std::shared_ptr<DecisionNode> &work_item) {
      decision_depth = decision;
      decision_ptr = decision == -1 ? nullptr : RFSCDecisionTree::find_ancestor(work_item, decision_depth);
    }

    void decision_swap(Event &e) {
      std::swap(decision_ptr, e.decision_ptr);
      std::swap(decision_depth, e.decision_depth);
    }

  private:
    /* The hierarchical order of events. */
    int decision_depth;
  };

  /* The fixed prefix of events in the current execution. This may be
   * either the complete sequence of events executed thus far in the
   * execution, or the events executed followed by the subsequent
   * events that are determined in advance to be executed.
   */
  std::vector<Event> prefix;
  VClockVec below_clocks;

  /* Reference to a global tree of DecisionNode, mainly accessed
   * to construct new witnesses during compute_prefix or
   * retrieving new work items for execution.
   */
  RFSCDecisionTree &decision_tree;

  /* The index into prefix corresponding to the last event that was
   * scheduled. Has the value -1 when no events have been scheduled.
   */
  int prefix_idx;

  /* Whether the currently replaying event has had its sym set. */
  bool seen_effect;

  /* Are we currently replaying the events given in prefix from the
   * previous execution? Or are we executing new events by arbitrary
   * scheduling?
   */
  bool replay;

  /* Is the execution cancelled? If so, we must reset before anything
   * else can be executed.
   */
  bool cancelled;

  /* The number of events that were or are going to be replayed in the
   * current computation.
   */
  int replay_point;

  /* The latest value passed to this->metadata(). */
  const llvm::MDNode *last_md;

  IPid ipid(int proc, int aux) const {
    assert(aux == -1);
    assert(proc < int(threads.size()));
    return proc;
  }

  Event &curev() {
    assert(0 <= prefix_idx);
    assert(prefix_idx < int(prefix.size()));
    return prefix[prefix_idx];
  }

  const Event &curev() const {
    assert(0 <= prefix_idx);
    assert(prefix_idx < int(prefix.size()));
    return prefix[prefix_idx];
  }

  /* Perform the logic of atomic_store(), aside from recording a
   * symbolic event.
   */
  void do_atomic_store(const SymData &ml);
  /* Perform the logic of load(), aside from recording a symbolic
   * event.
   */
  void do_load(const SymAddrSize &ml);

  /* Finds the index in prefix of the event of process pid that has iid-index
   * index.
   */
  Option<unsigned> try_find_process_event(IPid pid, int index) const;
  unsigned find_process_event(IPid pid, int index) const;

  /* Pretty-prints the iid of prefix[pos]. */
  std::string iid_string(std::size_t pos) const;
  /* Pretty-prints the iid of event. */
  std::string iid_string(const Event &event) const;
  /* Pretty-prints an iid. */
  std::string iid_string(IID<IPid> iid) const;
  /* Adds a reversible co-enabled happens-before edge between the
   * current event and event.
   */
  void add_noblock_race(int event);
  /* Adds an observed race between first and second that is observed by
   * the current event.
   */
  void add_observed_race(int first, int second);
  /* Computes a mapping between IPid and current local clock value
   * (index) of that process after executing the prefix [0,event).
   */
  std::vector<int> iid_map_at(int event) const;
  /* Plays an iid_map forward by one event. */
  void iid_map_step(std::vector<int> &iid_map, const Event &event) const;
  /* Reverses an iid_map by one event. */
  void iid_map_step_rev(std::vector<int> &iid_map, const Event &event) const;
  /* Adds a non-reversible happens-before edge between first and
   * second.
   */
  void add_happens_after(unsigned second, unsigned first);
  /* Adds a non-reversible happens-before edge between the last event
   * executed by thread (if there is such an event), and second.
   */
  void add_happens_after_thread(unsigned second, IPid thread);
  /* Computes the vector clocks of all events in a complete execution
   * sequence from happens_after and race edges.
   */
  void compute_vclocks();
  int compute_above_clock(unsigned event);
  /* Assigns unfolding events to all executed steps. */
  void compute_unfolding();

  // TODO: Refactor RFSCUnfoldingTree and and deprecate these methods.
  // Workaround due to require access to parent while not having a root-node
  RFSCUnfoldingTree::NodePtr unfold_find_unfolding_node
  (IPid pid, int index, Option<int> read_from);
  RFSCUnfoldingTree::NodePtr unfold_alternative
  (unsigned i, const RFSCUnfoldingTree::NodePtr &read_from);
  // END TODO

  void add_event_to_graph(SaturatedGraph &g, unsigned i) const;
  /* Access a SaturatedGraph from a DecisionNode.
   * This has the risk of mutating a graph which is accessed by
   * multiple threads concurrently, therefore needs to be under
   * exclusive operation.
   */
  const SaturatedGraph &get_cached_graph(DecisionNode &decision);
  /* Checks whether an event is included in a vector clock. */
  bool happens_before(const Event &e, const VClock<IPid> &c) const;
  /* Check whether a read-from might be satisfiable according to the
   * vector clocks.
   */
  bool can_rf_by_vclocks(int r, int ow, int w) const;
  /* Assuming that r and w are RMW, that r immediately precedes w in
   * coherence order, checks whether swapping r and w is possible
   * according to the vector clocks.
   */
  bool can_swap_by_vclocks(int r, int w) const;
  /* Assuming that f and s are MLock, that u is MUnlock, s rf u rf f,
   * checks whether swapping f and s is possible according to the vector
   * clocks.
   */
  bool can_swap_lock_by_vclocks(int f, int u, int s) const;
  /* Records a symbolic representation of the current event.
   */
  bool NODISCARD record_symbolic(SymEv event);
  Leaf try_sat(std::initializer_list<unsigned>, std::map<SymAddr,std::vector<int>> &);
  Leaf order_to_leaf(int decision, std::initializer_list<unsigned> changed,
                     const std::vector<unsigned> order) const;
  void output_formula(SatSolver &sat,
                      std::map<SymAddr,std::vector<int>> &,
                      const std::vector<bool> &);
  std::vector<bool> causal_past(int decision) const;
  void causal_past_1(std::vector<bool> &acc, unsigned i) const;
  /* Estimate the total number of traces that have the same prefix as
   * the current one, up to the first idx events.
   */
  long double estimate_trace_count(int idx) const;

  bool is_load(unsigned idx) const;
  bool is_store(unsigned idx) const;
  bool is_store_when_reading_from(unsigned idx, int read_from) const;
  bool is_cmpxhgfail(unsigned idx) const;
  bool is_lock(unsigned idx) const;
  bool is_trylock_fail(unsigned idx) const;
  bool is_lock_type(unsigned idx) const;
  bool does_lock(unsigned idx) const;
  bool is_unlock(unsigned idx) const;
  bool is_minit(unsigned idx) const;
  bool is_mdelete(unsigned idx) const;
  SymAddrSize get_addr(unsigned idx) const;
  SymData get_data(int idx, const SymAddrSize &addr) const;
  struct CmpXhgUndoLog {
    enum {
          NONE,
          SUCCEED,
          FAIL,
          M_SUCCEED,
          M_FAIL,
    } kind;
    unsigned idx, pos;
    SymEv *e;
  };
  CmpXhgUndoLog recompute_cmpxhg_success(unsigned idx, std::vector<int> &writes);
  void undo_cmpxhg_recomputation(CmpXhgUndoLog log, std::vector<int> &writes);
};

#endif
