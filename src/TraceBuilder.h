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

#ifndef __TRACE_BUILDER_H__
#define __TRACE_BUILDER_H__

#include "Configuration.h"
#include "Trace.h"

#include <string>
#include <vector>

/* A TraceBuilder object is maintained throughout DPOR trace
 * exploration. The TraceBuilder provides scheduling for each trace
 * during exploration, based on previously explored traces. During
 * exploration of an execution, various methods of the TraceBuilder
 * should be called in order to register events that affect the
 * chronological trace, such as memory accesses, fences, thread
 * creation etc. When the explored execution terminates,
 * TraceBuilder::reset should be called to initiate scheduling and
 * building for the next trace.
 */
class TraceBuilder{
public:
  TraceBuilder(const Configuration &conf = Configuration::default_conf);
  virtual ~TraceBuilder();
  /* Returns true iff the sleepset is currently empty. */
  virtual bool sleepset_is_empty() const = 0;
  /* When called at the end of an execution, returns true iff there are
   * any await-statements that are blocking.
   */
  virtual bool await_blocked() const { return false; }
  /* Returns true iff this trace contains any happens-before cycle.
   *
   * If there is a happens-before cycle, and conf.check_robustness is
   * set, then a corresponding error is added to errors.
   *
   * Call this method only at the end of execution, before calling
   * reset.
   */
  virtual bool check_for_cycles() = 0;
  /* Have any errors been discovered in the current trace? */
  virtual bool has_error() const { return errors.size(); };
  /* Compute the Trace of the current execution. */
  virtual Trace *get_trace() const = 0;
  /* Notify the TraceBuilder that the current execution has terminated.
   *
   * The TraceBuilder will reset, and based on observed conflicts and
   * previous traces prepare for the next trace that should be
   * explored.
   *
   * If there are no more traces to explore, returns false. Otherwise
   * returns true.
   */
  virtual bool reset() = 0;
  /* Return the IID of the currently scheduled event. */
  virtual IID<CPid> get_iid() const = 0;

  virtual void debug_print() const {};

  /* Notify the TraceBuilder that an assertion has failed.
   *
   * If loc is given, the error is associated with that location
   * rather than the current.
   */
  virtual void assertion_error(std::string cond, const IID<CPid> &loc = IID<CPid>());
  /* Notify the TraceBuilder that an erroneous usage of the pthreads
   * library was detected.
   *
   * If loc is given, the error is associated with that location
   * rather than the current.
   */
  virtual void pthreads_error(std::string msg, const IID<CPid> &loc = IID<CPid>());
  /* Notify the TraceBuilder that a segmentation fault occurred in the
   * explored execution.
   *
   * If loc is given, the error is associated with that location
   * rather than the current.
   */
  virtual void segmentation_fault_error(const IID<CPid> &loc = IID<CPid>());
  /* Notify the TraceBuilder some memory error, described by msg, has
   * occurred.
   *
   * If loc is given, the error is associated with that location
   * rather than the current.
   */
  virtual void memory_error(std::string msg, const IID<CPid> &loc = IID<CPid>());
  /* Notify the TraceBuilder that nondeterminism has been detected.
   *
   * If loc is given, the error is associated with that location
   * rather than the current.
   */
  virtual void nondeterminism_error(std::string msg, const IID<CPid> &loc = IID<CPid>());
  /* Notify the TraceBuilder that invalid behaviour has been detected.
   *
   * If loc is given, the error is associated with that location
   * rather than the current.
   */
  virtual void invalid_input_error(std::string msg, const IID<CPid> &loc = IID<CPid>());
  /* Estimate the total number of traces for this program based on the
   * traces that have been seen.
   */
  virtual long double estimate_trace_count() const { return 1; };
protected:
  const Configuration &conf;
  std::vector<Error*> errors;
};

#endif
