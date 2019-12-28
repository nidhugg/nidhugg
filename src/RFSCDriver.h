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

#ifndef __RFSC_DRIVER_H__
#define __RFSC_DRIVER_H__

#include "DPORDriver.h"
#include "RFSCTraceBuilder.h"
#include "blockingconcurrentqueue.h"
// #include "Configuration.h"
// #include "Trace.h"
// #include "TraceBuilder.h"
// #include "DPORInterpreter.h"

// #if defined(HAVE_LLVM_IR_MODULE_H)
// #include <llvm/IR/Module.h>
// #elif defined(HAVE_LLVM_MODULE_H)
// #include <llvm/Module.h>
// #endif

// #include <string>


/* The DPORDriver is the main driver of the trace exploration. It
 * takes an LLVM Module, and repeatedly explores its different traces.
 */
class RFSCDriver : public DPORDriver{
  public:

  static DPORDriver *parseIRFile(const std::string &filename,
                                 const Configuration &conf);
  
  // RFSCDriver(const RFSCDriver&) = delete;
  // RFSCDriver &operator=(const RFSCDriver&) = delete;

  RFSCDriver(const Configuration &conf);

  virtual Result run();

  // void thread_runner(int id, RFSCDriver *driver, std::vector<RFSCTraceBuilder *> &TBs, moodycamel::BlockingConcurrentQueue<std::tuple<bool, Trace *, bool>> &queue);
};

#endif