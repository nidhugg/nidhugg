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

#include "DPORDriver.h"

#include "CheckModule.h"
#include "Debug.h"
#include "Interpreter.h"
#include "POWERInterpreter.h"
#include "POWERARMTraceBuilder.h"
#include "PSOInterpreter.h"
#include "PSOTraceBuilder.h"
#include "StrModule.h"
#include "TSOInterpreter.h"
#include "TSOTraceBuilder.h"
#include "RFSCTraceBuilder.h"
#include "RFSCUnfoldingTree.h"
#include "Cpubind.h"

#include <fstream>
#include <stdexcept>
#include <iomanip>
#include <cfloat>
#include <thread>

#if defined(HAVE_LLVM_IR_LLVMCONTEXT_H)
#include <llvm/IR/LLVMContext.h>
#elif defined(HAVE_LLVM_LLVMCONTEXT_H)
#include <llvm/LLVMContext.h>
#endif
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>

namespace {
  const char ESC_char = 27;

  struct RunResult {
    Trace *trace;
    bool assume_blocked;
    int tasks_created;
  };

  template<typename T = RunResult>
  class BlockingQueue {
    std::mutex mtx;
    std::condition_variable cv;
    std::queue<T> queue;

  public:
    T dequeue() {
      std::unique_lock<std::mutex> lock(mtx);
      while (queue.empty()) {
        cv.wait(lock);
      }
      T res = std::forward<T>(queue.front());
      queue.pop();
      return res;
    }

    void enqueue(T res) {
      std::lock_guard<std::mutex> lock(mtx);
      queue.emplace(std::forward<T>(res));
      cv.notify_one();
    }
  };
}

DPORDriver::DPORDriver(const Configuration &C) :
  conf(C) {
  std::string ErrorMsg;
  llvm::sys::DynamicLibrary::LoadLibraryPermanently(0,&ErrorMsg);
}

DPORDriver *DPORDriver::parseIRFile(const std::string &filename,
                                    const Configuration &C){
  DPORDriver *driver =
    new DPORDriver(C);
  read_file(filename,driver->src);
  return driver;
}

DPORDriver *DPORDriver::parseIR(const std::string &llvm_asm,
                                const Configuration &C){
  DPORDriver *driver =
    new DPORDriver(C);
  driver->src = llvm_asm;
  return driver;
}

void DPORDriver::read_file(const std::string &filename, std::string &tgt){
  std::ifstream is(filename);
  if(!is){
    throw std::logic_error("Failed to read assembly file.");
  }
  is.seekg(0,std::ios::end);
  tgt.resize(is.tellg());
  is.seekg(0,std::ios::beg);
  is.read(&tgt[0],tgt.size());
  is.close();
}

std::unique_ptr<llvm::Module> DPORDriver::
parse(ParseOptions opts, llvm::LLVMContext &context){
  std::unique_ptr<llvm::Module> mod(StrModule::read_module_src(src,context));
  if (opts == PARSE_AND_CHECK) {
    CheckModule::check_functions(mod.get());
  }
  if(mod->LLVM_MODULE_GET_DATA_LAYOUT_STRING().empty()){
    if(llvm::sys::IsLittleEndianHost){
      mod->setDataLayout("e");
    }else{
      mod->setDataLayout("E");
    }
  }
  return mod;
}

std::unique_ptr<DPORInterpreter> DPORDriver::
create_execution_engine(TraceBuilder &TB, llvm::Module *mod,
                        const Configuration &conf) const {
  std::string ErrorMsg;
  std::unique_ptr<DPORInterpreter> EE = 0;
  switch(conf.memory_model){
  case Configuration::SC:
    EE = llvm::Interpreter::create(mod,static_cast<TSOPSOTraceBuilder&>(TB),conf,&ErrorMsg);
    break;
  case Configuration::TSO:
    EE = TSOInterpreter::create(mod,static_cast<TSOTraceBuilder&>(TB),conf,&ErrorMsg);
    break;
  case Configuration::PSO:
    EE = PSOInterpreter::create(mod,static_cast<PSOTraceBuilder&>(TB),conf,&ErrorMsg);
    break;
  case Configuration::ARM: case Configuration::POWER:
    EE = POWERInterpreter::create(mod,static_cast<POWERARMTraceBuilder&>(TB),conf,&ErrorMsg);
    break;
  case Configuration::MM_UNDEF:
    throw std::logic_error("DPORDriver: No memory model is specified.");
    break;
  default:
    throw std::logic_error("DPORDriver: Unsupported memory model.");
  }

  if (!EE) {
    if (!ErrorMsg.empty()){
      throw std::logic_error("Error creating EE: " + ErrorMsg);
    }else{
      throw std::logic_error("Unknown error creating EE!");
    }
  }

  llvm::Function *EntryFn = mod->getFunction("main");
  if(!EntryFn){
    throw std::logic_error("No main function found in module.");
  }

  // Reset errno to zero on entry to main.
  errno = 0;

  // Run static constructors.
  EE->runStaticConstructorsDestructors(false);

  // Trigger compilation separately so code regions that need to be
  // invalidated will be known.
  (void)EE->getPointerToFunction(EntryFn);

  return EE;
}

Trace *DPORDriver::run_once(TraceBuilder &TB, llvm::Module *mod,
                            bool &assume_blocked) const{
  std::unique_ptr<DPORInterpreter> EE(create_execution_engine(TB,mod,conf));

  // Run main.
  EE->runFunctionAsMain(mod->getFunction("main"), conf.argv, 0);

  // Run static destructors.
  EE->runStaticConstructorsDestructors(true);

  if(conf.check_robustness){
    static_cast<llvm::Interpreter*>(EE.get())->checkForCycles();
  }

  assume_blocked = EE->assumeBlocked();

  EE.reset();

  Trace *t = 0;
  if(TB.has_error() || conf.debug_collect_all_traces){
    if(TB.has_error() &&
       (conf.memory_model == Configuration::ARM ||
        conf.memory_model == Configuration::POWER)){
      static_cast<POWERARMTraceBuilder&>(TB).replay();
      Configuration conf2(conf);
      conf2.ee_store_trace = true;
      std::unique_ptr<DPORInterpreter> E = create_execution_engine(TB,mod,conf2);
      E->runFunctionAsMain(mod->getFunction("main"), conf.argv, 0);
      E->runStaticConstructorsDestructors(true);
      if(conf.check_robustness){
        static_cast<llvm::Interpreter*>(E.get())->checkForCycles();
      }
      t = TB.get_trace();
    }else{
      t = TB.get_trace();
    }
  }// else avoid computing trace

  return t;
}

void DPORDriver::print_progress(uint64_t computation_count, long double estimate, Result &res, int tasks_left) {
  if(computation_count % 100 == 0){
    llvm::dbgs() << ESC_char << "[K" // Erase the line
                 << "Traces: " << res.trace_count;
    if(res.sleepset_blocked_trace_count)
      llvm::dbgs() << ", " << res.sleepset_blocked_trace_count << " ssb";
    if(res.assume_blocked_trace_count)
      llvm::dbgs() << ", " << res.assume_blocked_trace_count << " ab";
    if(res.await_blocked_trace_count)
      llvm::dbgs() << ", " << res.await_blocked_trace_count << " awb";
    if(tasks_left != -1)
      llvm::dbgs() << " (" << tasks_left << " jobs)";
    if(conf.print_progress_estimate){
      std::stringstream ss;
      ss << std::setprecision(LDBL_DIG) << estimate;
      llvm::dbgs() << " ("
                   << int(100.0*(long double)(computation_count+1)/estimate)
                   << "% of total estimate: "
                   << ss.str() << ")";
    }
    llvm::dbgs() << "\r"; // Move cursor to start of line
  }
}

bool DPORDriver::handle_trace(TraceBuilder *TB, Trace *t, uint64_t *computation_count, Result &res, bool assume_blocked) {
  bool t_used = false;
  if(t && conf.debug_collect_all_traces){
    res.all_traces.push_back(t);
    t_used = true;
  }
  if(!TB->sleepset_is_empty()) {
    ++res.sleepset_blocked_trace_count;
  }else if(assume_blocked){
    ++res.assume_blocked_trace_count;
  }else if(TB->await_blocked()){
    ++res.await_blocked_trace_count;
  }else{
    ++res.trace_count;
  }
  ++*computation_count;
  if(t && t->has_errors() && !res.has_errors()){
    res.error_trace = t;
    t_used = true;
  }
  bool has_errors = t && t->has_errors();
  if(!t_used){
    delete t;
  }

  return has_errors && !conf.explore_all_traces;
}


namespace{
  std::unique_ptr<RFSCScheduler> make_scheduler(const Configuration &conf) {
    switch (conf.exploration_scheduler) {
    case Configuration::PRIOQUEUE:
      return std::unique_ptr<RFSCScheduler>(new PriorityQueueScheduler());
    case Configuration::WORKSTEALING:
      return std::unique_ptr<RFSCScheduler>(new WorkstealingPQScheduler(conf.n_threads));
    default:
      abort();
    }
  }
}

DPORDriver::Result DPORDriver::run_rfsc_sequential() {
  Result res;
  std::unique_ptr<llvm::Module> mod = parse(PARSE_AND_CHECK);
  RFSCDecisionTree decision_tree(make_scheduler(conf));
  RFSCUnfoldingTree unfolding_tree;
  RFSCTraceBuilder TB(decision_tree, unfolding_tree, conf);

  uint64_t computation_count = 0;
  long double estimate = 1;
  int tasks_left = 1;

  do{
    if(conf.print_progress){
      print_progress(computation_count, estimate, res);
    }

    bool assume_blocked = false;
    TB.reset();
    Trace *t= this->run_once(TB, mod.get(), assume_blocked);
    TB.compute_prefixes();
    tasks_left--;

    int to_create = TB.tasks_created;


    tasks_left += to_create;

    if (handle_trace(&TB, t, &computation_count, res, assume_blocked)) {
      break;
    }
    if(conf.print_progress_estimate && (computation_count+1) % 100 == 0){
      estimate = std::round(TB.estimate_trace_count());
    }
    if((computation_count+1) % 1000 == 0){
      /* llvm::ExecutionEngine leaks global variables until the Module is
       * destructed */
      mod = parse();
    }

  } while(tasks_left);

  if(conf.print_progress){
    llvm::dbgs() << ESC_char << "[K\n";
  }

  return res;
}

DPORDriver::Result DPORDriver::run_rfsc_parallel() {
  Result res;
  unsigned n_threads = conf.n_threads-1;
  std::vector<std::thread> threads;
  threads.reserve(n_threads);
  Cpubind cpubind(conf.n_threads);

  struct state {
    state(const Configuration &conf) : decision_tree(make_scheduler(conf)) {}
    RFSCDecisionTree decision_tree;
    RFSCUnfoldingTree unfolding_tree;
    uint64_t computation_count = 0;
    std::mutex mutex;
  } state(conf);

  auto thread = [this, &res, &state] (unsigned id) {
    auto context = std::make_unique<llvm::LLVMContext>();
    std::unique_ptr<llvm::Module> mod = parse(id ? PARSE_ONLY : PARSE_AND_CHECK,
                                              *context);
    RFSCScheduler &sched = state.decision_tree.get_scheduler();
    sched.register_thread(id);
    RFSCTraceBuilder TB(state.decision_tree, state.unfolding_tree, conf);
    unsigned int my_computation_count = 0;
    while (TB.reset()) {
      bool assume_blocked = false;
      Trace *t = this->run_once(TB, mod.get(), assume_blocked);
      TB.compute_prefixes();
      TB.work_item.reset();

      std::lock_guard<std::mutex> lock(state.mutex);
      uint64_t remain =
        sched.outstanding_jobs.fetch_sub(1, std::memory_order_relaxed)
        - 1;
      if (handle_trace(&TB, t, &state.computation_count, res, assume_blocked)
          || remain == 0) {
        sched.halt();
      }
      if(conf.print_progress){
        const long double estimate = 1;
        print_progress(state.computation_count, estimate, res, remain);
      }
      if (++my_computation_count % 1024 == 0) {
        /* llvm::ExecutionEngine leaks global variables until the Module is
         * destructed */
        mod = parse(PARSE_ONLY, *context);
      }
    }
  };

  for (unsigned i = 0; i < n_threads; ++i) {
    threads.emplace_back(thread, i+1);
    cpubind.bind(threads.back(), i+1);
  }
  cpubind.bind(0);
  thread(0);
  for (unsigned i = 0; i < n_threads; ++i) {
    threads[i].join();
  }

  if(conf.print_progress){
    llvm::dbgs() << ESC_char << "[K\n";
  }

  return res;
}

DPORDriver::Result DPORDriver::run(){
  Result res;
  std::unique_ptr<llvm::Module> mod = parse(PARSE_AND_CHECK);

  TraceBuilder *TB = nullptr;

  switch(conf.memory_model){
  case Configuration::SC:
    if(conf.dpor_algorithm != Configuration::READS_FROM){
      TB = new TSOTraceBuilder(conf);
    }else{
      if (conf.n_threads == 1){
        res = run_rfsc_sequential();
      } else {
        res = run_rfsc_parallel();
      }
      return res;
    }
    break;
  case Configuration::TSO:
    TB = new TSOTraceBuilder(conf);
    break;
  case Configuration::PSO:
    TB = new PSOTraceBuilder(conf);
    break;
  case Configuration::ARM:
    TB = new ARMTraceBuilder(conf);
    break;
  case Configuration::POWER:
    TB = new POWERTraceBuilder(conf);
    break;
  case Configuration::MM_UNDEF:
    throw std::logic_error("DPORDriver: No memory model is specified.");
    break;
  default:
    throw std::logic_error("DPORDriver: Unsupported memory model.");
  }

  uint64_t computation_count = 0;
  long double estimate = 1;
  do{
    if(conf.print_progress){
      print_progress(computation_count, estimate, res);
    }
    if((computation_count+1) % 1000 == 0){
      /* llvm::ExecutionEngine leaks global variables until the Module is
       * destructed */
      mod = parse();
    }

    bool assume_blocked = false;
    Trace *t = run_once(*TB, mod.get(), assume_blocked);

    if (handle_trace(TB, t, &computation_count, res, assume_blocked)) break;
    if(conf.print_progress_estimate && (computation_count+1) % 100 == 0){
      estimate = std::round(TB->estimate_trace_count());
    }
  }while(TB->reset());

  if(conf.print_progress){
    llvm::dbgs() << ESC_char << "[K\n";
  }

  delete TB;

  return res;
}

