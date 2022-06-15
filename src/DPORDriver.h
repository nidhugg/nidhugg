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
#include "DPORInterpreter.h"
#include "GlobalContext.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#if defined(HAVE_LLVM_IR_MODULE_H)
#include <llvm/IR/Module.h>
#elif defined(HAVE_LLVM_MODULE_H)
#include <llvm/Module.h>
#endif
#if defined(HAVE_LLVM_IR_LLVMCONTEXT_H)
#include <llvm/IR/LLVMContext.h>
#elif defined(HAVE_LLVM_LLVMCONTEXT_H)
#include <llvm/LLVMContext.h>
#endif

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
  virtual ~DPORDriver() {}
  DPORDriver(const DPORDriver&) = delete;
  DPORDriver &operator=(const DPORDriver&) = delete;

private:
  /* Since we have some manual ownership management in Result, we extract it to
   * a private base class so that we don't have to implement custom move
   * constructors and operators for all fields.
   */
  struct ResultBase {
    ResultBase() : error_trace(nullptr) {}
    ~ResultBase(){
      if(all_traces.empty()){ // Otherwise error_trace also appears in all_traces.
        delete error_trace;
      }
      for(Trace *t : all_traces){
        delete t;
      }
    }
    ResultBase(const ResultBase&) = delete;
    ResultBase(ResultBase &&other)
      : error_trace(std::exchange(other.error_trace, nullptr)),
        all_traces(std::move(other.all_traces)) {
      other.all_traces.clear();
    }
    ResultBase &operator=(ResultBase &&other) {
      std::swap(error_trace, other.error_trace);
      std::swap(all_traces, other.all_traces);
      return *this;
    }

    /* An empty trace if no error has been encountered. Otherwise some
     * error trace.
     */
    Trace *error_trace;
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

public:
  /* A Result object describes the result of exploring the traces of
   * some module.
   */
  class Result : public ResultBase{
  public:
    /* Empty result */
    Result() : trace_count(0), sleepset_blocked_trace_count(0),
               assume_blocked_trace_count(0), await_blocked_trace_count(0) {}
    /* The number of explored (non-sleepset-blocked) traces */
    uint64_t trace_count;
    /* The number of explored sleepset-blocked traces */
    uint64_t sleepset_blocked_trace_count;
    /* The number of explored assume-blocked traces */
    uint64_t assume_blocked_trace_count;
    /* The number of explored assume-blocked traces */
    uint64_t await_blocked_trace_count;

    bool has_errors() const { return error_trace && error_trace->has_errors(); }
  };

  /* Explore the traces of the given module, and return the result.
   */
  Result run();

private:
  /* Configuration */
  const Configuration &conf;
  /* The source code of the module to explore. Expressed as LLVM
   * assembly or as LLVM bitcode.
   */
  std::string src;

  DPORDriver(const Configuration &conf);
  Trace *run_once(TraceBuilder &TB, llvm::Module *mod, bool &assume_blocked) const;
  /* Parse the module in a given LLVMContext,
   * optionally checking validity (should be done the first time) */
  enum ParseOptions { PARSE_ONLY, PARSE_AND_CHECK };
  std::unique_ptr<llvm::Module> parse
  (ParseOptions opts,
   llvm::LLVMContext &context);
  /* Opens and reads the file filename. Stores the entire content in
   * tgt. Throws an exception on failure.
   */
  static void read_file(const std::string &filename, std::string &tgt);
  /* Creates an execution engine based on the configuration conf and
   * the TraceBuilder TB. The kind of execution engine is determined
   * by conf.memory_model.
   */
  std::unique_ptr<DPORInterpreter>
  create_execution_engine(TraceBuilder &TB, llvm::Module *mod,
                          const Configuration &conf) const;

  /* Prints the progress of exploration if argument --print-progress was given. */
  void print_progress(uint64_t computation_count, long double estimate, Result &res, int tasks_left = -1);
  /* Updates the result based on the given tracecount and TraceBuilder. */
  bool handle_trace(TraceBuilder *TB, Trace *t, uint64_t *computation_count, Result &res, bool assume_blocked);

  /* Separate run-function for RFSC since it breaks the default
   * algorithm in DPORDriver::run(). This will spawn a threadpool
   * where eash thread will explore the execution tree concurrently.
   */
  Result run_rfsc_parallel();
  /* As RFSC explores asynchronously with a threadpool,
   * an alternate function is given without overhead
   * if it should be run strictly sequential.
   */
  Result run_rfsc_sequential();
  /* Make sure memory does not accumulate indefinately in the module or
   * context. May destroy and reallocate both (invalidates all llvm
   * pointers derived from them). */
  void clear_memory_use(uint64_t trace_number,
                        std::unique_ptr<llvm::LLVMContext> &context,
                        std::unique_ptr<llvm::Module> &module);
};

#endif
