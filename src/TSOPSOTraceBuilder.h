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

#ifndef __TSO_PSO_TRACE_BUILDER_H__
#define __TSO_PSO_TRACE_BUILDER_H__

#include "Configuration.h"
#include "MRef.h"
#include "Trace.h"
#include "TraceBuilder.h"

#include <string>
#include <vector>

/*  === Thread Identification ===
 *
 * Througout this class, we will use the following convention for
 * identifying threads:
 *
 * A thread is identified by a pair (proc,aux). For real threads, aux
 * == -1. For auxiliary threads, 0 <= aux. The thread (0,-1) is the
 * single original thread in each execution. The thread (proc,-1) is
 * the proc:th thread that was created during the execution. The
 * thread (proc,aux) for 0 <= aux is an auxiliary thread of
 * (proc,-1). The exact interpretation of an auxiliary thread is
 * memory model specific.
 */

class TSOPSOTraceBuilder : public TraceBuilder{
public:
  TSOPSOTraceBuilder(const Configuration &conf = Configuration::default_conf)
    : TraceBuilder(conf) {};
  virtual ~TSOPSOTraceBuilder() {};
  /* Schedules the next thread.
   *
   * Sets *proc and *aux, such that (*proc,*aux) should be the next
   * thread to run.
   *
   * *alt is set to indicate which alternative execution should be
   * used for the next event, in cases where events have several
   * nondeterministic alternatives for execution. *alt == 0 means the
   * first alternative (the only alternative for deterministic
   * events). Higher values means other alternatives.
   *
   * *dryrun is set to indicate that the next event should only be dry
   * run. Dry running an event means simulating its effects without
   * making any changes to the program state, and without considering
   * the event to have been executed. During dry run, the TraceBuilder
   * will collect information about the event through calls to the
   * methods spawn, store, load, ..., which are used to register
   * aspect that affect the chronological trace. This is used to infer
   * when sleeping threads should be awoken.
   *
   * true is returned on successful scheduling. false is returned if
   * no thread can be scheduled (because all threads are marked
   * unavailable).
   */
  virtual bool schedule(int *proc, int *aux, int *alt, bool *dryrun) = 0;
  /* Reject the last scheduling. Erase the corresponding event and its
   * effects on the chronological trace. Also mark the last scheduled
   * thread as unavailable.
   *
   * Can only be used after a successful call to schedule(_,_,_,d)
   * where *d is false. Multiple refusals without intermediate
   * schedulings are not allowed.
   *
   * Use this for example in cases when the scheduled event turns out
   * to be blocked, waiting for e.g. store buffer flushes or the
   * termination of another thread.
   */
  virtual void refuse_schedule() = 0;
  /* Notify the TraceBuilder that (proc,aux) is available for
   * scheduling.
   */
  virtual void mark_available(int proc, int aux = -1) = 0;
  /* Notify the TraceBuilder that (proc,aux) is unavailable for
   * scheduling. This may be e.g. because it has terminated, or
   * because it is blocked waiting for something.
   */
  virtual void mark_unavailable(int proc, int aux = -1) = 0;
  /* Associate the currently scheduled event with LLVM "dbg" metadata. */
  virtual void metadata(const llvm::MDNode *md) = 0;

  /*******************************************/
  /*           Registration Methods          */
  /*******************************************/
  /* The following methods are used to register information about the
   * currently scheduled event, which may affect the chronological
   * trace.
   */

  /* The current event spawned a new thread. */
  virtual void spawn() = 0;
  /* Perform a store to ml. */
  virtual void store(const ConstMRef &ml) = 0;
  /* Perform an atomic store to ml.
   *
   * The exact interpretation depends on the memory model. But
   * examples would typically include instructions such as "store
   * atomic, ..., seq_cst" or "cmpxchg ... seq_cst ...".
   *
   * atomic_store performed by auxiliary threads can be used to model
   * memory updates under non-SC memory models.
   */
  virtual void atomic_store(const ConstMRef &ml) = 0;
  /* Perform a load to ml. */
  virtual void load(const ConstMRef &ml) = 0;
  /* Perform an action that conflicts with all memory accesses and all
   * other full memory conflicts.
   *
   * This is used for things that execute as blackboxes (e.g. calls to
   * unknown external functions) in order to capture all potential
   * conflicts that such blackboxes may cause.
   */
  virtual void full_memory_conflict() = 0;
  /* Perform a full memory fence. */
  virtual void fence() = 0;
  /* Perform a pthread_join, with the thread (tgt_proc,-1). */
  virtual void join(int tgt_proc) = 0;
  /* Perform a successful pthread_mutex_lock to the pthread mutex at
   * ml.
   */
  virtual void mutex_lock(const ConstMRef &ml) = 0;
  /* Perform a failed attempt at pthread_mutex_lock to the pthread
   * mutex at ml.
   */
  virtual void mutex_lock_fail(const ConstMRef &ml) = 0;
  /* Perform a pthread_mutex_trylock (successful or failing) to the
   * pthread mutex at ml.
   */
  virtual void mutex_trylock(const ConstMRef &ml) = 0;
  /* Perform a pthread_mutex_unlock to the pthread mutex at ml. */
  virtual void mutex_unlock(const ConstMRef &ml) = 0;
  /* Initialize a pthread mutex at ml. */
  virtual void mutex_init(const ConstMRef &ml) = 0;
  /* Destroy a pthread mutex at ml. */
  virtual void mutex_destroy(const ConstMRef &ml) = 0;
  /* Initialize a pthread condition variable at ml.
   *
   * Returns true on success, false if a pthreads_error has been
   * generated.
   */
  virtual bool cond_init(const ConstMRef &ml) = 0;
  /* Signal on the condition variable at ml.
   *
   * Returns true on success, false if a pthreads_error has been
   * generated.
   */
  virtual bool cond_signal(const ConstMRef &ml) = 0;
  /* Broadcast on the condition variable at ml.
   *
   * Returns true on success, false if a pthreads_error has been
   * generated.
   */
  virtual bool cond_broadcast(const ConstMRef &ml) = 0;
  /* Wait for a condition variable at cond_ml.
   *
   * The mutex at mutex_ml is used as the mutex for pthread_cond_wait.
   *
   * Returns true on success, false if a pthreads_error has been
   * generated.
   */
  virtual bool cond_wait(const ConstMRef &cond_ml, const ConstMRef &mutex_ml) = 0;
  /* Destroy a pthread condition variable at ml.
   *
   * Returns 0 on success, EBUSY if some thread was waiting for the
   * condition variable, another value if a pthreads_error has been
   * generated.
   */
  virtual int cond_destroy(const ConstMRef &ml) = 0;
  /* Notify TraceBuilder that the current event may
   * nondeterministically execute in several alternative ways. The
   * number of ways is given in alt_count.
   */
  virtual void register_alternatives(int alt_count) = 0;
};

#endif
