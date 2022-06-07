/* Copyright (C) 2015-2017 Carl Leonardsson
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

#ifndef __POWER_TRACE_BUILDER_H__
#define __POWER_TRACE_BUILDER_H__

#include <stdexcept>
#include <string>
#include <vector>

#include "BVClock.h"
#include "FBVClock.h"
#include "TraceBuilder.h"
#include "MRef.h"

#if defined(HAVE_LLVM_IR_METADATA_H)
#include <llvm/IR/Metadata.h>
#elif defined(HAVE_LLVM_METADATA_H)
#include <llvm/Metadata.h>
#endif

/* A POWERARMTraceBuilder is a TraceBuilder for the POWER or ARM
 * memory models. POWERARMTraceBuilder is an abstract class, inherited
 * by the concrete classes POWERTraceBuilder and ARMTraceBuilder.
 *
 * Thread identification:
 *
 * Threads are identified by natural numbers.  When these appear as
 * parameters for methods, they are usually named "proc". The initial
 * thread is identified by the number 0. New threads spawned by calls
 * to the spawn method will be identified by the smallest natural
 * number hitherto unused for thread identification. The pid part of
 * IID<int> identifies threads by the same scheme.
 */
class POWERARMTraceBuilder : public TraceBuilder {
public:
  POWERARMTraceBuilder(const Configuration &conf = Configuration::default_conf)
    : TraceBuilder(conf) {}
  virtual ~POWERARMTraceBuilder() {}

  /* Access identification:
   *
   * Individual accesses of a single event are identified by natural
   * numbers: The i:th load access of this event (counting from 0) is
   * identified by the access identifier (Accid) i, or the load access
   * identifier (Ldaccid) i. The j:th store access of this event
   * (counting from 0) is identified by the access identifier (Accid)
   * (L+j) or by the store access identifier (Staccid) j, where L is
   * the number of load accesses of this event.
   */
  typedef int Accid;
  typedef int Ldaccid;
  typedef int Staccid;

  /* Fetch a new event for thread proc.
   *
   * The event will perform load_count memory loads and store_count
   * memory stores.
   *
   * addr_deps and data_deps represent the address and data
   * dependencies of this event as follows: An integer i is in
   * addr_deps (resp. data_deps) precisely when the event
   * IID<int>(proc,i) is an address dependency (resp. data dependency)
   * of any one of the accesses performed by this event.
   *
   * For each access, its address must be registered before the event
   * can be committed. For each store access, its data must
   * additionally be registered before the event can be committed. See
   * methods register_addr and register_data below.
   *
   * Returns the IID of the new event.
   */
  virtual IID<int> fetch(int proc,
                         int load_count,
                         int store_count,
                         const VecSet<int> &addr_deps,
                         const VecSet<int> &data_deps) = 0;
  virtual void register_addr(const IID<int> &iid, Accid a, const MRef &addr) = 0;
  virtual void register_data(const IID<int> &iid, Staccid a, const MBlock &data) = 0;
  typedef std::function<bool(const MBlock&)> ldreqfun_t;
  virtual void register_load_requirement(const IID<int> &iid, Ldaccid a, ldreqfun_t *f) = 0;
  virtual void fetch_ctrl(int proc, const VecSet<int> &deps) = 0;
  enum FenceType { ISYNC, EIEIO, LWSYNC, SYNC };
  virtual void fetch_fence(int proc, FenceType type) = 0;
  /********************************
   * Methods for Recording Traces *
   ********************************/
  /* When recording a trace, trace_register_metadata(proc,md) should
   * be called by the ExecutionEngine when an instruction has been
   * committed. proc should be the committing thread and md should be
   * the dbg metadata attached to that instruction.
   */
  virtual void trace_register_metadata(int proc, const llvm::MDNode *md) = 0;
  /* When recording a trace,
   * trace_register_external_function_call(proc,fname,md) should be
   * called by the ExecutionEngine when some function call instruction
   * to an external function has been committed. proc should be the
   * committing thread, fname should be the name of the entered
   * function, md should be the dbg metadata attached to the call
   * instruction.
   */
  virtual void trace_register_external_function_call(int proc, const std::string &fname, const llvm::MDNode *md) = 0;
  /* When recording a trace,
   * trace_register_function_entry(proc,fname,md) should be called by
   * the ExecutionEngine when some function call instruction has been
   * committed. proc should be the committing thread, fname should be
   * the name of the entered function, md should be the dbg metadata
   * attached to the call instruction.
   */
  virtual void trace_register_function_entry(int proc, const std::string &fname, const llvm::MDNode *md) = 0;
  /* When recording a trace, trace_register_function_exit(proc) should
   * be called by the ExecutionEngine when a thread proc returns from
   * a function (commits the return instruction).
   */
  virtual void trace_register_function_exit(int proc) = 0;
  /* When recording a trace, trace_register_error(proc,err_msg) should
   * be called by the ExecutionEngine when an error is detected in the
   * thread proc. The string err_msg will be added to the printed
   * error trace as a description of the error.
   */
  virtual void trace_register_error(int proc, const std::string &err_msg) = 0;

  /* Schedules and commits a pending event evt.
   *
   * evt will be such that there are no pending events preceding evt
   * in the chrono order. This implies that all dependencies of evt
   * are already committed.
   *
   * *iid is assigned the IID of evt.
   *
   * *values is setup such that it has one element per load access of
   * evt. The i:th element of *values will be the value read by the
   * i:th load access of evt.
   *
   * If there are no pending events that can be scheduled (either the
   * computation has terminated or is blocked.), then false is
   * returned and *iid and *values are undefined. Otherwise true is
   * returned.
   */
  virtual bool schedule(IID<int> *iid, std::vector<MBlock> *values) = 0;

  virtual bool check_for_cycles(){
    throw std::logic_error("POWERARMTraceBuilder::check_for_cycles: Not supported.");
  }
  /* Call to spawn a new thread. proc should be the parent thread. The
   * identifier of the new thread is returned.
   */
  virtual int spawn(int proc) = 0;
  virtual void abort() = 0;
  /* The replay method acts much like the reset method. But instead of
   * changing some parameter to generate a different trace, replay
   * keeps the same parameters, so that the next run is an identical
   * repetition of the previous.
   */
  virtual void replay() = 0;
};

/* PATB_impl contains the implementation of the concrete classes
 * POWERTraceBuilder and ARMTraceBuilder. A user of those classes
 * should only need to refer to the abstract super class
 * POWERARMTraceBuilder.
 */
namespace PATB_impl{

  enum MemoryModel {
    ARM,
    POWER
  };

  class ARMEvent;
  class POWEREvent;

  enum CB_T {
    /* CB_0:
     * addr U data U ctrl U rf
     */
    CB_0,
    /* CB_ARM:
     * addr U data U ctrl U rf U addr;po U sync U lwsync
     */
    CB_ARM,
    /* CB_POWER:
     * addr U data U ctrl U rf U addr;po U po-loc U sync U lwsync
     */
    CB_POWER
  };

  template<MemoryModel MemMod, CB_T CB, class Event> class TB;
}  // namespace PATB_impl

typedef PATB_impl::TB<PATB_impl::ARM,PATB_impl::CB_ARM,PATB_impl::ARMEvent> ARMTraceBuilder;
typedef PATB_impl::TB<PATB_impl::POWER,PATB_impl::CB_POWER,PATB_impl::POWEREvent> POWERTraceBuilder;

#include "POWERARMTraceBuilder_decl.h"

#endif

