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

class TSOTraceBuilder : public TSOPSOTraceBuilder{
  bool schedule_replay(int *proc, int *aux, int *alt, bool *dryrun);
  bool schedule_thread(int *proc, unsigned p);
  // merge last (invisible) event to the previous
  // (also invisible) event
  void squeezeLastEvent();
  void mergeInvisibleEvents();
public:
  TSOTraceBuilder(const Configuration &conf = Configuration::default_conf);
  virtual ~TSOTraceBuilder();
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

  virtual void spawn();
  virtual void store(const ConstMRef &ml);
  virtual void atomic_store(const ConstMRef &ml);
  virtual void load(const ConstMRef &ml);
  virtual void full_memory_conflict();
  virtual void fence();
  virtual void join(int tgt_proc);
  virtual void mutex_lock(const ConstMRef &ml);
  virtual void mutex_lock_fail(const ConstMRef &ml);
  virtual void mutex_trylock(const ConstMRef &ml);
  virtual void mutex_unlock(const ConstMRef &ml);
  virtual void mutex_init(const ConstMRef &ml);
  virtual void mutex_destroy(const ConstMRef &ml);
  virtual bool cond_init(const ConstMRef &ml);
  virtual bool cond_signal(const ConstMRef &ml);
  virtual bool cond_broadcast(const ConstMRef &ml);
  virtual bool cond_wait(const ConstMRef &cond_ml, const ConstMRef &mutex_ml);
  virtual int cond_destroy(const ConstMRef &ml);
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

  /* A store pending in a store buffer. */
  class PendingStore{
  public:
    PendingStore(const ConstMRef &ml, const VClock<IPid> &clk, const llvm::MDNode *md)
      : ml(ml), clock(clk), last_rowe(-1), md(md) {};
    /* The memory location that is being written to. */
    ConstMRef ml;
    /* The clock of the store event which produced this store buffer
     * entry.
     */
    VClock<IPid> clock;
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
    Thread(const CPid &cpid, const VClock<IPid> &clk)
      : cpid(cpid), available(true), clock(clk), sleeping(false), sleep_full_memory_conflict(false) {};
    CPid cpid;
    /* Is the thread available for scheduling? */
    bool available;
    /* The clock containing all events that have been seen by this
     * thread.
     */
    VClock<IPid> clock;
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
    VecSet<void const *> sleep_accesses_r;
    /* sleep_accesses_w is the set of bytes that will be written by
     * the next event to be executed by this thread (as determined by
     * dry running).
     *
     * Empty if !sleeping.
     */
    VecSet<void const *> sleep_accesses_w;
    /* sleep_full_memory_conflict is set when the next event to be
     * executed by this thread will be a full memory conflict (as
     * determined by dry running).
     */
    bool sleep_full_memory_conflict;
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
    ByteInfo() : last_update(-1), last_update_ml(0,1) {};
    /* An index into prefix, to the latest update that accessed this
     * byte. last_update == -1 if there has been no update to this
     * byte.
     */
    int last_update;
    /* The complete memory location (possibly multiple bytes) that was
     * accessed by the last update. Undefined if there has been no
     * update to this byte.
     */
    ConstMRef last_update_ml;
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
  std::map<const void*,ByteInfo> mem;
  /* Index into prefix pointing to the latest full memory conflict.
   * -1 if there has been no full memory conflict.
   */
  int last_full_memory_conflict;

  /* A Mutex represents a pthread_mutex_t object.
   */
  class Mutex{
  public:
    Mutex() : last_access(-1), last_lock(-1) {};
    Mutex(int lacc) : last_access(lacc), last_lock(-1) {};
    int last_access;
    int last_lock;
  };
  /* A map containing all pthread mutex objects in the current
   * execution. The key is the position in memory of the actual
   * pthread_mutex_t object.
   */
  std::map<void const*,Mutex> mutexes;

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
  std::map<void const*,CondVar> cond_vars;

  /* A Branch object is a pair of an IPid p and an alternative index
   * (see Event::alt below) i. It will be tagged on an event in the
   * execution to indicate that if instead of that event, p is allowed
   * to execute (with alternative index i), then a different trace can
   * be produced.
   */
  class Branch{
  public:
    IPid pid;
    int alt;
    bool operator<(const Branch &b) const{
      return pid < b.pid || (pid == b.pid && alt < b.alt);
    };
    bool operator==(const Branch &b) const{
      return pid == b.pid && alt == b.alt;
    };
  };

  /* Information about a (short) sequence of consecutive events by the
   * same thread. At most one event in the sequence may have conflicts
   * with other events, and if the sequence has a conflicting event,
   * it must be the first event in the sequence.
   */
  class Event{
  public:
    Event(const IID<IPid> &iid,
          const VClock<IPid> &clk)
      : iid(iid), origin_iid(iid), size(1), alt(0), md(0), clock(clk),
        may_conflict(false), sleep_branch_trace_count(0) {};
    /* The identifier for the first event in this event sequence. */
    IID<IPid> iid;
    /* The IID of the program instruction which is the origin of this
     * event. For updates, this is the IID of the corresponding store
     * instruction. For other instructions origin_iid == iid.
     */
    IID<IPid> origin_iid;
    /* The number of events in this sequence. */
    int size;
    /* Some instructions may execute in several alternative ways
     * nondeterministically. (E.g. malloc may succeed or fail
     * nondeterministically if Configuration::malloy_may_fail is set.)
     * Event::alt is the index of the alternative for the first event
     * in this event sequence. The default execution alternative has
     * index 0. All events in this sequence, except the first, are
     * assumed to run their default execution alternative.
     */
    int alt;
    /* Metadata corresponding to the first event in this sequence. */
    const llvm::MDNode *md;
    /* The clock of the first event in this sequence. */
    VClock<IPid> clock;
    /* Is it possible for any event in this sequence to have a
     * conflict with another event?
     */
    bool may_conflict;
    /* Different, yet untried, branches that should be attempted from
     * this position in prefix.
     */
    VecSet<Branch> branch;
    /* The set of threads that go to sleep immediately before this
     * event sequence.
     */
    VecSet<IPid> sleep;
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
    int sleep_branch_trace_count;
  };

  /* The fixed prefix of events in the current execution. This may be
   * either the complete sequence of events executed thus far in the
   * execution, or the events executed followed by the subsequent
   * events that are determined in advance to be executed.
   */
  std::vector<Event> prefix;

  /* The number of threads that have been dry run since the last
   * non-dry run event was scheduled.
   */
  int dry_sleepers;

  /* The index into prefix corresponding to the last event that was
   * scheduled. Has the value -1 when no events have been scheduled.
   */
  int prefix_idx;

  /* Are we currently executing an event in dry run mode? */
  bool dryrun;

  /* Are we currently replaying the events given in prefix from the
   * previous execution? Or are we executing new events by arbitrary
   * scheduling?
   */
  bool replay;

  /* The latest value passed to this->metadata(). */
  const llvm::MDNode *last_md;

  IPid ipid(int proc, int aux) const {
    assert(-1 <= aux && aux <= 0);
    assert(proc*2+1 < int(threads.size()));
    return aux ? proc*2 : proc*2+1;
  };

  Event &curnode() {
    assert(0 <= prefix_idx);
    assert(prefix_idx < int(prefix.size()));
    return prefix[prefix_idx];
  };

  const Event &curnode() const {
    assert(0 <= prefix_idx);
    assert(prefix_idx < int(prefix.size()));
    return prefix[prefix_idx];
  };

  std::string iid_string(const Event &evt) const;
  void add_branch(int i, int j);
  /* Add clocks and branches.
   *
   * All elements e in seen should either be indices into prefix, or
   * be negative. In the latter case they are ignored.
   */
  void see_events(const VecSet<int> &seen);
  /* Traverses prefix to compute the set of threads that were sleeping
   * as the first event of prefix[i] started executing. Returns that
   * set.
   */
  VecSet<IPid> sleep_set_at(int i);
  /* Wake up all threads which are sleeping, waiting for an access
   * (type,ml). */
  void wakeup(Access::Type type, void const *ml);
  /* Returns true iff the thread pid has a pending store to some
   * memory location including the byte ml.
   */
  bool has_pending_store(IPid pid, void const *ml) const;
  /* Helper for check_for_cycles. */
  bool has_cycle(IID<IPid> *loc) const;
  /* Estimate the total number of traces that have the same prefix as
   * the current one, up to the first idx events.
   */
  int estimate_trace_count(int idx) const;
};

#endif

