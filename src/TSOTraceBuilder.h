/* Copyright (C) 2014-2017 Carl Leonardsson
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
#ifndef __TSO_TRACE_BUILDER_H__
#define __TSO_TRACE_BUILDER_H__

#include "TSOPSOTraceBuilder.h"
#include "VClock.h"
#include "SymEv.h"
#include "WakeupTrees.h"
#include "Option.h"

#include <boost/container/flat_map.hpp>
#include <unordered_map>

typedef llvm::SmallVector<SymEv,1> sym_ty;

class TSOTraceBuilder : public TSOPSOTraceBuilder{
public:
  TSOTraceBuilder(const Configuration &conf = Configuration::default_conf);
  virtual ~TSOTraceBuilder() override;
  virtual bool schedule(int *proc, int *aux, int *alt, bool *dryrun) override;
  virtual void refuse_schedule() override;
  virtual void mark_available(int proc, int aux = -1) override;
  virtual void mark_unavailable(int proc, int aux = -1) override;
  virtual void cancel_replay() override;
  virtual bool is_replaying() const override;
  virtual void metadata(const llvm::MDNode *md) override;
  virtual bool sleepset_is_empty() const override;
  virtual bool await_blocked() const override { return blocked_awaits.size(); }
  virtual bool check_for_cycles() override;
  virtual Trace *get_trace() const override;
  virtual bool reset() override;
  virtual IID<CPid> get_iid() const override;

  virtual void debug_print() const override;

  virtual NODISCARD bool spawn() override;
  virtual NODISCARD bool store(const SymData &ml) override;
  virtual NODISCARD bool atomic_store(const SymData &ml) override;
  virtual NODISCARD bool atomic_rmw(const SymData &ml, RmwAction action) override;
  virtual NODISCARD bool xchg_await(const SymData &ml, AwaitCond cond) override;
  virtual NODISCARD bool xchg_await_fail(const SymData &ml, AwaitCond cond) override;

  virtual NODISCARD bool compare_exchange
  (const SymData &sd, const SymData::block_type expected, bool success) override;
  virtual NODISCARD bool load(const SymAddrSize &ml) override;
  virtual NODISCARD bool load_await(const SymAddrSize &ml, AwaitCond cond)
    override;
  virtual NODISCARD bool load_await_fail(const SymAddrSize &ml, AwaitCond cond)
    override;
  virtual NODISCARD bool full_memory_conflict() override;
  virtual NODISCARD bool fence() override;
  virtual NODISCARD bool join(int tgt_proc) override;
  virtual NODISCARD bool mutex_lock(const SymAddrSize &ml) override;
  virtual NODISCARD bool mutex_lock_fail(const SymAddrSize &ml) override;
  virtual NODISCARD bool mutex_trylock(const SymAddrSize &ml) override;
  virtual NODISCARD bool mutex_unlock(const SymAddrSize &ml) override;
  virtual NODISCARD bool mutex_init(const SymAddrSize &ml) override;
  virtual NODISCARD bool mutex_destroy(const SymAddrSize &ml) override;
  virtual NODISCARD bool cond_init(const SymAddrSize &ml) override;
  virtual NODISCARD bool cond_signal(const SymAddrSize &ml) override;
  virtual NODISCARD bool cond_broadcast(const SymAddrSize &ml) override;
  virtual NODISCARD bool cond_wait(const SymAddrSize &cond_ml,
                         const SymAddrSize &mutex_ml) override;
  virtual NODISCARD bool cond_awake(const SymAddrSize &cond_ml,
                          const SymAddrSize &mutex_ml) override;
  virtual int cond_destroy(const SymAddrSize &ml) override;
  virtual NODISCARD bool register_alternatives(int alt_count) override;
  virtual long double estimate_trace_count() const override;
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

  /* A store pending in a store buffer. */
  class PendingStore{
  public:
    PendingStore(const SymAddrSize &ml, unsigned store_event,
                 const llvm::MDNode *md)
      : ml(ml), store_event(store_event), last_rowe(-1), md(md) {};
    /* The memory location that is being written to. */
    SymAddrSize ml;
    /* The index into prefix of the store event that produced this store
     * buffer entry.
     */
    unsigned store_event;
    /* An index into prefix to the event of the last load that fetched
     * its value from this store buffer entry by Read-Own-Write-Early.
     *
     * Has the value -1 if there has been no such load.
     */
    int last_rowe;
    /* "dbg" metadata for the store which produced this store buffer
     * entry.
     */
    const llvm::MDNode *md;
  };

  /* Various information about a thread in the current execution.
   */
  class Thread{
  public:
    Thread(const CPid &cpid, int spawn_event)
      : cpid(cpid), available(true), spawn_event(spawn_event), sleeping(false),
        sleep_full_memory_conflict(false), sleep_sym(nullptr) {};
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
    /* The store buffer of this thread. The store buffer is kept in
     * the Thread object for the real thread, not for the auxiliary.
     *
     * Newer entries are further to the back.
     */
    std::vector<PendingStore> store_buffer;
    /* True iff this thread is currently in the sleep set. */
    bool sleeping;
    /* sleep_accesses_r is the set of bytes that will be read by the
     * next event to be executed by this thread (as determined by dry
     * running).
     *
     * Empty if !sleeping.
     */
    VecSet<SymAddr> sleep_accesses_r;
    /* sleep_accesses_w is the set of bytes that will be written by
     * the next event to be executed by this thread (as determined by
     * dry running).
     *
     * Empty if !sleeping.
     */
    VecSet<SymAddr> sleep_accesses_w;
    /* sleep_full_memory_conflict is set when the next event to be
     * executed by this thread will be a full memory conflict (as
     * determined by dry running).
     */
    bool sleep_full_memory_conflict;
    /* sleep_sym is the set of globally visible actions that the next
     * event to be executed by this thread will do (as determined by
     * dry running).
     *
     * NULL if !sleeping.
     */
    sym_ty *sleep_sym;

    /* The iid-index of the last event of this thread, or 0 if it has not
     * executed any events yet.
     */
    int last_event_index() const { return event_indices.size(); }
  };
  /* The threads in the current execution, in the order they were
   * created. Threads on even indexes are real, threads on odd indexes
   * i are the auxiliary threads corresponding to the real threads at
   * index i-1.
   */
  std::vector<Thread> threads;
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
    /* Set of events that updated this byte since it was last read.
     * Represented as a map from pid to index since at most one event
     * per process may be in the set.
     *
     * Either contains last_update or is empty.
     */
    boost::container::flat_map<IPid,unsigned> unordered_updates;
    /* The set of events that is ordered before the current set of
     * unordered updates. */
    VecSet<int> before_unordered;
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

  /* A Branch object is a pair of an IPid p and an alternative index
   * (see Event::alt below) i. It will be tagged on an event in the
   * execution to indicate that if instead of that event, p is allowed
   * to execute (with alternative index i), then a different trace can
   * be produced.
   */
  class Branch{
  public:
    Branch (IPid pid, int alt = 0, sym_ty sym = {})
      : sym(std::move(sym)), pid(pid), alt(alt), size(1) {}
    Branch (const Branch &base, sym_ty sym)
      : sym(std::move(sym)), pid(base.pid), alt(base.alt), size(base.size) {}
    /* Symbolic representation of the globally visible operation of this event.
     */
    sym_ty sym;
    IPid pid;
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
    bool operator<(const Branch &b) const{
      return pid < b.pid || (pid == b.pid && alt < b.alt);
    };
    bool operator==(const Branch &b) const{
      return pid == b.pid && alt == b.alt;
    };
  };

  struct Race {
  public:
    enum Kind : uint8_t {
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
      /* An event that can be blocked by other processes. Includes a set
       * "exclude" to exclude from the wakeup sequence. */
      SEQUENCE,
    };
    SymEv ev;
    Kind kind;
    int first_event;
    int second_event;
    IID<IPid> second_process;
    std::vector<unsigned> exclude; // XXX: Make me fixed_vector
    union{
      int witness_event;
      int alternative;
      int unlock_event;
    };
    static Race Nonblock(int first, int second) {
      return Race(NONBLOCK, first, second, {-1,0}, -1);
    };
    static Race Observed(int first, int second, int witness) {
      return Race(OBSERVED, first, second, {-1,0}, witness);
    };
    static Race LockFail(int first, int second, IID<IPid> process) {
      return Race(LOCK_FAIL, first, second, process, -1);
    };
    static Race LockSuc(int first, int second, int unlock) {
      return Race(LOCK_SUC, first, second, {-1,0}, unlock);
    };
    static Race Nondet(int event, int alt) {
      return Race(NONDET, event, -1, {-1,0}, alt);
    };
    static Race Sequence(int first, int second, IID<IPid> process, SymEv ev,
                         std::vector<unsigned> exclude) {
      return Race(SEQUENCE, first, second, process, std::move(ev),
                  std::move(exclude));
    }
    bool is_fail_kind() const {
      return kind == LOCK_FAIL || kind == SEQUENCE;
    }
  private:
    Race(Kind k, int f, int s, IID<IPid> p, int w) :
      kind(k), first_event(f), second_event(s), second_process(p),
      witness_event(w) {}
    Race(Kind k, int f, int s, IID<IPid> p, SymEv &&ev,
         std::vector<unsigned> &&exclude) :
      ev(std::move(ev)), kind(k), first_event(f), second_event(s),
      second_process(p), exclude(std::move(exclude)) {}
  };

  /* Locations in the trace where a process is blocked waiting for a
   * lock. All are of kind LOCK_FAIL.
   */
  std::vector<Race> lock_fail_races;

  /* All currently blocking Await-events. */
  struct BlockedAwait {
    BlockedAwait(int index, SymEv ev)
      : ev(std::move(ev)), index(index) {}
    SymEv ev;
    int index;
  };
  std::unordered_map<SymAddr, boost::container::flat_map<IPid,BlockedAwait>>
    blocked_awaits;

  /* Information about a (short) sequence of consecutive events by the
   * same thread. At most one event in the sequence may have conflicts
   * with other events, and if the sequence has a conflicting event,
   * it must be the first event in the sequence.
   */
  class Event{
  public:
    Event(const IID<IPid> &iid, sym_ty sym = {})
      : iid(iid), origin_iid(iid), md(0), clock(), may_conflict(false),
        sym(std::move(sym)), sleep_branch_trace_count(0) {};
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
     */
    VClock<IPid> clock;
    /* Indices into prefix of events that happen before this one. */
    std::vector<unsigned> happens_after;
    /* Possibly reversible races found in the current execution
     * involving this event as the main event.
     */
    std::vector<Race> races;
    /* Events that unblock the current event (and thus are not races),
     * but are not inherently needed to enable this event.
     *
     * For a lock-event, the prior unlock event does not need to be in
     * here, as it is handles specially by LockSuc races in races.
     *
     * May not contain -1
     */
    VecSet<int> happens_after_later;

    /* Is it possible for any event in this sequence to have a
     * conflict with another event?
     */
    bool may_conflict;
    /* Symbolic representation of the globally visible operation of this event.
     * Empty iff !may_conflict
     */
    sym_ty sym;
    /* The set of threads that go to sleep immediately before this
     * event sequence.
     */
    VecSet<IPid> sleep;
    /* The events that the threads in sleep will perform as their next step,
     * as determined by dry running.
     * sleep and sleep_evs are of the same size and correspond pairwise.
     */
    std::vector<sym_ty> sleep_evs;
    /* The set of sleeping threads that wake up during or after this
     * event sequence.
     */
    VecSet<IPid> wakeup;
    /* For each previous IID that has been explored at this position
     * with the exact same prefix, some number of traces (both sleep
     * set blocked and otherwise) have been
     * explored. sleep_branch_trace_count is the total number of such
     * explored traces.
     */
    uint64_t sleep_branch_trace_count;
  };

  /* The fixed prefix of events in the current execution. This may be
   * either the complete sequence of events executed thus far in the
   * execution, or the events executed followed by the subsequent
   * events that are determined in advance to be executed.
   */
  WakeupTreeExplorationBuffer<Branch, Event> prefix;

  /* The number of threads that have been dry run since the last
   * non-dry run event was scheduled.
   */
  int dry_sleepers;

  /* The index into prefix corresponding to the last event that was
   * scheduled. Has the value -1 when no events have been scheduled.
   */
  int prefix_idx;

  /* The index of the currently expected symbolic event, as an index into
   * curev().sym. Equal to curev().sym.size() (or 0 when prefix_idx == -1) when
   * not replaying.
   */
  unsigned sym_idx;

  /* Are we currently executing an event in dry run mode? */
  bool dryrun;

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
    assert(-1 <= aux && aux <= 0);
    assert(proc*2+1 < int(threads.size()));
    return aux ? proc*2 : proc*2+1;
  };

  Event &curev() {
    assert(0 <= prefix_idx);
    assert(prefix_idx < int(prefix.len()));
    return prefix[prefix_idx];
  };

  const Event &curev() const {
    assert(0 <= prefix_idx);
    assert(prefix_idx < int(prefix.len()));
    return prefix[prefix_idx];
  };

  const Branch &curbranch() const {
    assert(0 <= prefix_idx);
    assert(prefix_idx < int(prefix.len()));
    return prefix.branch(prefix_idx);
  };

  /* Symbolic events in Branches in the wakeup tree do not record the
   * data of memory accesses as these can change between executions.
   * branch_with_symbolic_data(i) returns a Branch of the event i, but
   * with data of memory accesses in the symbolic events.
   */
  Branch branch_with_symbolic_data(unsigned index) const {
    return Branch(prefix.branch(index), prefix[index].sym);
  };

  IID<CPid> get_iid(unsigned i) const;

  /* Perform the logic of atomic_store(), aside from recording a
   * symbolic event.
   */
  void do_atomic_store(const SymData &ml);
  /* Perform the logic of load(), aside from recording a symbolic
   * event.
   */
  void do_load(const SymAddrSize &ml);
  /* Adds the happens-before edges that result from the current event
   * reading ml. m must be mem[ml].
   * is_update indicates whether the current event is an update.
   */
  void observe_memory(SymAddr ml, ByteInfo &m,
                      VecSet<int> &seen_accesses,
                      VecSet<std::pair<int,int>> &seen_pairs,
                      bool is_update);
  /* Adds the races implied by an await at position pos in prefix to
   * races. pos may be prefix.len() (corresponding to a deadlocked
   * await).
   * Returns false if an error was added */
  bool do_await(unsigned pos, const IID<IPid> &iid, const SymEv &e,
                const VClock<IPid> &above_clock, std::vector<Race> &races);

  /* Finds the index in prefix of the event of process pid that has iid-index
   * index.
   */
  std::pair<bool,unsigned> try_find_process_event(IPid pid, int index) const;
  unsigned find_process_event(IPid pid, int index) const;

  /* Pretty-prints the iid of prefix[pos]. */
  std::string iid_string(std::size_t pos) const;
  /* Pretty-prints the iid <branch.pid, index>. */
  std::string iid_string(const Branch &branch, int index) const;
  /* Pretty prints the wakeup tree subtree rooted at <branch,node>
   * into the buffer lines starting at line line.
   * iid_map needs to be an iid_map at <branch,node>. It is restored to
   * the value it had when calling the function before returning.
   */
  void wut_string_add_node(std::vector<std::string> &lines,
                           std::vector<int> &iid_map,
                           unsigned line, Branch branch,
                           WakeupTreeRef<Branch> node) const;
#ifndef NDEBUG
  void check_symev_vclock_equiv() const;
#endif
  /* Adds a reversible co-enabled happens-before edge between the
   * current event and event.
   */
  void add_noblock_race(int event);
  /* Adds an observed race between first and second that is observed by
   * the current event.
   */
  void add_observed_race(int first, int second);
  /* Add a race between two successful mutex aquisitions (lock and the
   * current event). Unlock is the unlock event between them.
   */
  void add_lock_suc_race(int lock, int unlock);
  /* Record that the currently executing event is being blocked trying
   * to aquire a mutex, which is held by the lock event event.
   */
  void add_lock_fail_race(int event);
  /* Check if two events in the current prefix are in conflict. */
  bool do_events_conflict(int i, int j) const;
  bool do_events_conflict(const Event &fst, const Event &snd) const;
  /* Check if two symbolic events conflict. */
  bool do_events_conflict(IPid fst_pid, const sym_ty &fst,
                          IPid snd_pid, const sym_ty &snd) const;
  bool do_symevs_conflict(IPid fst_pid, const SymEv &fst,
                          IPid snd_pid, const SymEv &snd) const;
  /* Check if events fst and snd are in an observed race with thd as an
   * observer.
   */
  bool is_observed_conflict(const Event &fst, const Event &snd,
                            const Event &thd) const;
  /* Check if two symbolic events are in an observed race with a third
   * as an observer.
   */
  bool is_observed_conflict(IPid fst_pid, const sym_ty &fst,
                            IPid snd_pid, const sym_ty &snd,
                            IPid thd_pid, const sym_ty &thd) const;
  bool is_observed_conflict(IPid fst_pid, const SymEv &fst,
                            IPid snd_pid, const SymEv &snd,
                            IPid thd_pid, const SymEv &thd) const;
  /* Reconstruct the above-vector clock of a blocked event with IID
   * iid. */
  VClock<IPid> reconstruct_blocked_clock(IID<IPid> iid) const;
  /* Reconstruct the vector clock and symbolic event of a blocked event
   * recorded in race, for example a blocked attempt at aquiring a
   * mutex.
   */
  Event reconstruct_blocked_event(const Race &race) const;
  /* Computes a mapping between IPid and current local clock value
   * (index) of that process after executing the prefix [0,event).
   */
  std::vector<int> iid_map_at(int event) const;
  /* Plays an iid_map forward by one event. */
  void iid_map_step(std::vector<int> &iid_map, const Branch &event) const;
  /* Reverses an iid_map by one event. */
  void iid_map_step_rev(std::vector<int> &iid_map, const Branch &event) const;
  /* Add clocks and branches.
   *
   * All elements e in seen should either be indices into prefix, or
   * be negative. In the latter case they are ignored.
   */
  void see_events(const VecSet<int> &seen);
  /* Add clocks and branches.
   *
   * All pairs in seen should be increasing indices into prefix.
   */
  void see_event_pairs(const VecSet<std::pair<int,int>> &seen);
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
  /* Keep track of whether compute_vclocks has been called yet. */
  bool has_vclocks = false;
  /* Perform planning of future executions. Requires the trace to be
   * maximal or sleepset blocked, and that the vector clocks have been
   * computed.
   */
  void do_race_detect();
  /* Records a symbolic representation of the current event.
   *
   * During replay, events are checked against the replay log. If a mismatch is
   * detected, a nondeterminism error is reported and the function returns
   * false.
   *
   * Otherwise returns true.
   */
  bool NODISCARD record_symbolic(SymEv event);
  /* When planning a reversal with a compare-exchange, the symbolic event
   * of the hoisted event must correctly indicate whether the
   * compare-exchange will succeed or fail, otherwise sleep and
   * wakeup-tree logic can break. This function updates all
   * compare-exchange operations in one event es when preceeded by the
   * events prefix[0..i] ++ v
   */
  void recompute_cmpxhg_success(sym_ty &es, const std::vector<Branch> &v, int i)
    const;
  /* Recompute the observation states on the symbolic events in v. */
  void recompute_observed(std::vector<Branch> &v) const;
  /* Check whether an await would be satisfied if played at position i
   * in the current prefix.
   */
  bool awaitcond_satisfied_before(unsigned i, const SymAddrSize &ml,
                                  const AwaitCond &cond) const;
  /* The same, but assuming that before playing the await, we play a set
   * of events given as seq (referring to events in prefix)
   */
  bool awaitcond_satisfied_by(unsigned i, const std::vector<unsigned> &seq,
                              const SymAddrSize &ml, const AwaitCond &cond) const;
  struct obs_sleep {
    struct sleeper {
      IPid pid;
      const sym_ty *sym;
      Option<SymAddrSize> not_if_read;
    };
    std::vector<struct sleeper> sleep;
    /* Addresses that must be read */
    std::vector<SymAddrSize> must_read;
    /* Check for pid in sleep */
    bool count(IPid pid) const;
  };
  /* Returns a string representation of a sleep set. */
  std::string oslp_string(const struct obs_sleep &slp) const;
  /* Traverses prefix to compute the set of threads that were sleeping
   * as the first event of prefix[i] started executing. Returns that
   * set.
   */
  struct obs_sleep obs_sleep_at(int i) const;
  /* Performs the first half of a sleep set step, adding new sleepers
   * from e.
   */
  void obs_sleep_add(struct obs_sleep &sleep, const Event &e) const;
  enum class obs_wake_res {
    CLEAR,
    CONTINUE,
    BLOCK,
  };
  /* Performs the second half of a sleep set step, removing sleepers that
   * conflict with (p, sym).
   *
   * If obs_wake_res::BLOCK is returned, then this execution has
   * blocked.
   */
  obs_wake_res obs_sleep_wake(struct obs_sleep &osleep, IPid p,
                              const sym_ty &sym) const;
  /* Performs the second half of a sleep set step, removing sleepers that
   * were identified as waking after event e.
   *
   * This overload is a workaround for having the obs_sleep sets work
   * correctly under TSO without a full symbolic conflict detection
   * implementation (as required for Optimal-DPOR), as obs_sleep now is
   * used even for Source-DPOR.
   *
   * As this overload is only used on events that have already been
   * executed, it will never block, and thus has no return value.
   */
  void obs_sleep_wake(struct obs_sleep &sleep, const Event &e) const;
  void race_detect(const Race&, const struct obs_sleep&);
  void race_detect_optimal(const Race&, const struct obs_sleep&);
  /* Compute the wakeup sequence for reversing a race. */
  std::vector<Branch> wakeup_sequence(const Race&) const;
  /* Checks if a sequence of events will clear a sleep set. */
  bool sequence_clears_sleep(const std::vector<Branch> &seq,
                             const struct obs_sleep &sleep) const;
  /* Wake up all threads which are sleeping, waiting for an access
   * (type,ml). */
  void wakeup(Access::Type type, SymAddr ml);
  /* Returns true iff the thread pid has a pending store to some
   * memory location including the byte ml.
   */
  bool has_pending_store(IPid pid, SymAddr ml) const;
  /* Helper for check_for_cycles. */
  bool has_cycle(IID<IPid> *loc) const;
  /* Estimate the total number of traces that have the same prefix as
   * the current one, up to the first idx events.
   */
  long double estimate_trace_count(int idx) const;
};

#endif

