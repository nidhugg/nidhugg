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

#ifndef __DPOR_DRIVER_H__
#define __DPOR_DRIVER_H__

#include "Configuration.h"
#include "Trace.h"
#include "TraceBuilder.h"

#if defined(HAVE_LLVM_IR_MODULE_H)
#include <llvm/IR/Module.h>
#elif defined(HAVE_LLVM_MODULE_H)
#include <llvm/Module.h>
#endif

#include <string>

namespace llvm{
  class ExecutionEngine;
}

/* The DPORDriver is the main driver of the trace exploration. It
 * takes an LLVM Module, and repeatedly explores its different traces.
 */
class DPORDriver{
public:
  /* Create and return a new DPORDriver with a module as defined in
   * the LLVM assembly or bitcode file filename.
   */
  static DPORDriver *parseIRFile(const std::string &filename,
                                 const Configuration &conf);
  /* Create and return a new DPORDriver with a module as defined by
   * the LLVM assembly given in llvm_asm.
   */
  static DPORDriver *parseIR(const std::string &llvm_asm,
                             const Configuration &conf);
  virtual ~DPORDriver();
  DPORDriver(const DPORDriver&) = delete;
  DPORDriver &operator=(const DPORDriver&) = delete;

  /* A Result object describes the result of exploring the traces of
   * some module.
   */
  class Result{
  public:
    /* Empty result */
    Result() : trace_count(0), sleepset_blocked_trace_count(0), error_trace(0) {};
    ~Result(){
      if(all_traces.empty()){ // Otherwise error_trace also appears in all_traces.
        delete error_trace;
      }
      for(Trace *t : all_traces){
        delete t;
      }
    };
    /* The number of explored (non-sleepset-blocked) traces */
    int trace_count;
    /* The number of explored sleepset-blocked traces */
    int sleepset_blocked_trace_count;
    /* An empty trace if no error has been encountered. Otherwise some
     * error trace.
     */
    Trace *error_trace;
    bool has_errors() const { return error_trace && error_trace->has_errors(); };
    /* Contains all traces that were explored, sleepset blocked and
     * otherwise.
     *
     * Used only if debug_collect_all_traces is set in the
     * configuration (off by default). Otherwise, all_traces is empty.
     *
     * WARNING: This quickly grows out of hand. Should be used only
     * for modules known to have few traces. Automated testing is the
     * intended usage.
     */
    std::vector<Trace*> all_traces;
  };

  /* Explore the traces of the given module, and return the result.
   */
  Result run();
private:
  /* Configuration */
  const Configuration &conf;
  /* The module to explore */
  llvm::Module *mod;
  /* The source code of the module to explore. Expressed as LLVM
   * assembly or as LLVM bitcode.
   */
  std::string src;

  DPORDriver(const Configuration &conf);
  Trace *run_once(TraceBuilder &TB) const;
  void reparse();
  /* Opens and reads the file filename. Stores the entire content in
   * tgt. Throws an exception on failure.
   */
  static void read_file(const std::string &filename, std::string &tgt);
  /* Creates an execution engine based on the configuration conf and
   * the TraceBuilder TB. The kind of execution engine is determined
   * by conf.memory_model.
   */
  llvm::ExecutionEngine *create_execution_engine(TraceBuilder &TB, const Configuration &conf) const;
};

#endif
