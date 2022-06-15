/* Copyright (C) 2017 Carl Leonardsson
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

#ifndef __DET_CHECK_TRACE_BUILDER_H__
#define __DET_CHECK_TRACE_BUILDER_H__

#include "TraceBuilder.h"

#include <vector>

/* A DetCheckTraceBuilder is an abstract TraceBuilder which adds some
 * functionality for detecting and reporting thread-wise
 * nondeterminism in the target program during computation replays.
 */
class DetCheckTraceBuilder : public TraceBuilder {
public:
  DetCheckTraceBuilder(const Configuration &conf = Configuration::default_conf);
  virtual ~DetCheckTraceBuilder() {}
  /* Execute a conditional branch instruction, branching on the value
   * cnd.
   *
   * If thread-wise non-determinism is detected, an error is
   * registered and false is returned. Otherwise true is returned.
   */
  virtual bool cond_branch(bool cnd);
  /* Returns true if the currently scheduled event is a replay of a
   * previous computation.
   */
  virtual bool is_replaying() const = 0;

protected:
  /* Call before starting a new computation to restart the branch
   * logging.
   */
  virtual void reset_cond_branch_log();

  /* cond_branch_log and cond_branch_log_index are used to log the
   * conditional branches of a computation. cond_branch_log contains
   * the branch condition values in order of execution. The first
   * cond_branch_log_index entries in cond_branch_log correspond to
   * branches that have been executed in the current
   * computation. Later entries are carried over from previous
   * (longer) computations. The carried-over entries are used to check
   * that the branching is deterministic during computation replay.
   */
  std::vector<bool> cond_branch_log;
  unsigned cond_branch_log_index; // See above
};

#endif
