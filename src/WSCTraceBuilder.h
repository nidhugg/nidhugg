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
#ifndef __WSC_TRACE_BUILDER_H__
#define __WSC_TRACE_BUILDER_H__

#include "TSOPSOTraceBuilder.h"
#include "VClock.h"
#include "SymEv.h"
#include "WakeupTrees.h"
#include "Option.h"
#include "SaturatedGraph.h"

#include <unordered_map>
#include <unordered_set>

static unsigned unf_ctr = 0;

class WSCTraceBuilder final : public TSOPSOTraceBuilder{
public:
  WSCTraceBuilder(const Configuration &conf = Configuration::default_conf);
  virtual ~WSCTraceBuilder();
  virtual bool schedule(int *proc, int *aux, int *alt, bool *dryrun);
  virtual void refuse_schedule();
  virtual void mark_available(int proc, int aux = -1);
  virtual void mark_unavailable(int proc, int aux = -1);
  virtual void cancel_replay();
  virtual bool is_replaying() const;
  virtual void metadata(const llvm::MDNode *md);
  virtual bool sleepset_is_empty() const;
  virtual bool check_for_cycles();
  virtual Trace *get_trace() const;
  virtual bool reset();
  virtual IID<CPid> get_iid() const;

  virtual void debug_print() const ;
  virtual bool cond_branch(bool cnd) { return true; }

  virtual void spawn();
  virtual void store(const SymData &ml);
  virtual void atomic_store(const SymData &ml);
  virtual void atomic_rmw(const SymData &ml);
  virtual void compare_exchange
  (const SymData &sd, const SymData::block_type expected, bool success);
  virtual void load(const SymAddrSize &ml);
  virtual void full_memory_conflict();
  virtual void fence();
  virtual void join(int tgt_proc);
  virtual void mutex_lock(const SymAddrSize &ml);
  virtual void mutex_lock_fail(const SymAddrSize &ml);
  virtual void mutex_trylock(const SymAddrSize &ml);
  virtual void mutex_unlock(const SymAddrSize &ml);
  virtual void mutex_init(const SymAddrSize &ml);
  virtual void mutex_destroy(const SymAddrSize &ml);
  virtual bool cond_init(const SymAddrSize &ml);
  virtual bool cond_signal(const SymAddrSize &ml);
  virtual bool cond_broadcast(const SymAddrSize &ml);
  virtual bool cond_wait(const SymAddrSize &cond_ml, const SymAddrSize &mutex_ml);
  virtual bool cond_awake(const SymAddrSize &cond_ml, const SymAddrSize &mutex_ml);
  virtual int cond_destroy(const SymAddrSize &ml);
  virtual void register_alternatives(int alt_count);
  virtual int estimate_trace_count() const;
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
    };
    bool operator==(const Access &a) const{
      return type == a.type && (type == NA || ml == a.ml);
    };
    Access() : type(NA), ml(0) {};
    Access(Type t, const void *m) : type(t), ml(m) {};
  };

  struct UnfoldingNode;
  typedef llvm::SmallVector<std::weak_ptr<UnfoldingNode>,1> UnfoldingNodeChildren;
  struct UnfoldingNode {
  public:
    UnfoldingNode(std::shared_ptr<UnfoldingNode> parent,
                  std::shared_ptr<UnfoldingNode> read_from)
      : parent(std::move(parent)), read_from(std::move(read_from)),
        seqno(++unf_ctr) {};
    std::shared_ptr<UnfoldingNode> parent, read_from;
    UnfoldingNodeChildren children;
    unsigned seqno;
  };

  struct Branch {
  public:
    Branch(int pid, int size, int decision, bool pinned, SymEv sym)
      : pid(pid), size(size), decision(decision), pinned(pinned),
        sym(std::move(sym)) {}
    Branch() : Branch(-1, 0, -1, false, {}) {}
    int pid, size, decision;
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
    DecisionNode() : siblings() {}
    std::unordered_map<std::shared_ptr<UnfoldingNode>, Leaf> siblings;
    std::unordered_set<std::shared_ptr<UnfoldingNode>> sleep;
    SaturatedGraph graph_cache;
  };

  /* Various information about a thread in the current execution. */
  class Thread{
  public:
    Thread(const CPid &cpid, int spawn_event)
      : cpid(cpid), available(true), spawn_event(spawn_event) {};
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

  std::map<CPid,UnfoldingNodeChildren> first_events;

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
    ByteInfo() : last_update(-1), last_update_ml({SymMBlock::Global(0),0},1) {};
    /* An index into prefix, to the latest update that accessed this
     * byte. last_update == -1 if there has been no update to this
     * byte.
     */
    int last_update;
    /* The complete memory location (possibly multiple bytes) that was
     * accessed by the last update. Undefined if there has been no
     * update to this byte.
     */
    SymAddrSize last_update_ml;
    /* last_read[tid] is the index in prefix of the latest (visible)
     * read of thread tid to this memory location, or -1 if thread tid
     * has not read this memory location.
     *
     * The indexing counts real threads only, so e.g. last_read[1] is
     * the last read of the second real thread.
     *
     * last_read_t is simply a wrapper around a vector, which expands
     * the vector as necessary to accomodate accesses through
     * operator[].
     */
    struct last_read_t {
      std::vector<int> v;
      int operator[](int i) const { return (i < int(v.size()) ? v[i] : -1); };
      int &operator[](int i) {
        if(int(v.size()) <= i){
          v.resize(i+1,-1);
        }
        return v[i];
      };
      std::vector<int>::iterator begin() { return v.begin(); };
      std::vector<int>::const_iterator begin() const { return v.begin(); };
      std::vector<int>::iterator end() { return v.end(); };
      std::vector<int>::const_iterator end() const { return v.end(); };
    } last_read;
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
    Mutex() : last_access(-1), last_lock(-1), locked(false) {};
    Mutex(int lacc) : last_access(lacc), last_lock(-1), locked(false) {};
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
    CondVar() : last_signal(-1) {};
    CondVar(int init_idx) : last_signal(init_idx) {};
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

  struct Race {
  public:
    enum Kind {
      /* Any kind of event that does not block any other process */
      NONBLOCK,
      /* A nonblocking race where additionally a third event is
       * required to observe the race between the first two. */
      OBSERVED,
      /* Attempt to acquire a lock that is already locked */
      LOCK_FAIL,
      /* Race between two successful blocking lock aquisitions (with an
       * unlock event in between) */
      LOCK_SUC,
      /* A nondeterministic event that can be performed differently */
      NONDET,
    };
    Kind kind;
    int first_event;
    int second_event;
    IID<IPid> second_process;
    union{
      const Mutex *mutex;
      int witness_event;
      int alternative;
      int unlock_event;
    };
    static Race Nonblock(int first, int second) {
      return Race(NONBLOCK, first, second, {-1,0}, nullptr);
    };
    static Race Observed(int first, int second, int witness) {
      return Race(OBSERVED, first, second, {-1,0}, witness);
    };
    static Race LockFail(int first, int second, IID<IPid> process,
                         const Mutex *mutex) {
      assert(mutex);
      return Race(LOCK_FAIL, first, second, process, mutex);
    };
    static Race LockSuc(int first, int second, int unlock) {
      return Race(LOCK_SUC, first, second, {-1,0}, unlock);
    };
    static Race Nondet(int event, int alt) {
      return Race(NONDET, event, -1, {-1,0}, alt);
    };
  private:
    Race(Kind k, int f, int s, IID<IPid> p, const Mutex *m) :
      kind(k), first_event(f), second_event(s), second_process(p), mutex(m) {}
    Race(Kind k, int f, int s, IID<IPid> p, int w) :
      kind(k), first_event(f), second_event(s), second_process(p),
      witness_event(w) {}
  };

  /* Locations in the trace where a process is blocked waiting for a
   * lock. All are of kind LOCK_FAIL.
   */
  std::vector<Race> lock_fail_races;

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
        decision(-1), sym(std::move(sym)), sleep_branch_trace_count(0) {};
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

    /* Index into decisions. */
    int decision;
    /* The unfolding event corresponding to this executed event. */
    std::shared_ptr<UnfoldingNode> event;

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
  };

  /* The fixed prefix of events in the current execution. This may be
   * either the complete sequence of events executed thus far in the
   * execution, or the events executed followed by the subsequent
   * events that are determined in advance to be executed.
   */
  std::vector<Event> prefix;
  VClockVec below_clocks;

  std::vector<DecisionNode> decisions;

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
  };

  Event &curev() {
    assert(0 <= prefix_idx);
    assert(prefix_idx < int(prefix.size()));
    return prefix[prefix_idx];
  };

  const Event &curev() const {
    assert(0 <= prefix_idx);
    assert(prefix_idx < int(prefix.size()));
    return prefix[prefix_idx];
  };

  /* Perform the logic of atomic_store(), aside from recording a
   * symbolic event.
   */
  void do_atomic_store(const SymData &ml);
  /* Perform the logic of load(), aside from recording a symbolic
   * event.
   */
  void do_load(const SymAddrSize &ml);
  /* Adds the happens-before edges that result from the current event
   * reading m.
   */
  void do_load(ByteInfo &m);

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
  /* Assigns unfolding events to all executed steps. */
  void compute_unfolding();
  std::shared_ptr<UnfoldingNode> find_unfolding_node
  (UnfoldingNodeChildren &parent_list,
   const std::shared_ptr<UnfoldingNode> &parent,
   const std::shared_ptr<UnfoldingNode> &read_from);
  std::shared_ptr<UnfoldingNode> alternative
  (unsigned i, const std::shared_ptr<UnfoldingNode> &read_from);
  void add_event_to_graph(SaturatedGraph &g, unsigned i) const;
  const SaturatedGraph &get_cached_graph(unsigned i);
  /* Perform planning of future executions. Requires the trace to be
   * maximal or sleepset blocked, and that the vector clocks have been
   * computed.
   */
  void compute_prefixes();
  /* Check whether a read-from might be satisfiable according to the
   * vector clocks.
   */
  bool can_rf_by_vclocks(int r, int ow, int w) const;
  /* Assuming that r and w are RMW, that r immediately preceeds w in
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
  void record_symbolic(SymEv event);
  Leaf try_sat(int, std::map<SymAddr,std::vector<int>> &);
  Leaf order_to_leaf(int decision, const std::vector<unsigned> order, SaturatedGraph g) const;
  void output_formula(SatSolver &sat,
                      std::map<SymAddr,std::vector<int>> &,
                      const std::vector<bool> &);
  std::vector<bool> causal_past(int decision) const;
  void causal_past_1(std::vector<bool> &acc, unsigned i) const;
  /* Estimate the total number of traces that have the same prefix as
   * the current one, up to the first idx events.
   */
  int estimate_trace_count(int idx) const;

  bool is_load(unsigned idx) const;
  bool is_store(unsigned idx) const;
  bool is_store_when_reading_from(unsigned idx, int read_from) const;
  bool is_cmpxhgfail(unsigned idx) const;
  bool is_lock(unsigned idx) const;
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
