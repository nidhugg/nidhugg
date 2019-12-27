/* Copyright (C) 2014-2017 Alexis Remmers and Nodari Kankava
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

#include "RFSCDriver.h"

#include "CheckModule.h"
#include "Debug.h"
#include "SigSegvHandler.h"

#include "RFSCUnfoldingTree.h"

#include "ctpl.h"


#define N_THREADS 1

RFSCDriver::RFSCDriver(const Configuration &C) : DPORDriver(C) {}


DPORDriver *RFSCDriver::parseIRFile(const std::string &filename,
                                    const Configuration &C){
  RFSCDriver *driver =
    new RFSCDriver(C);
  read_file(filename,driver->src);
  driver->reparse();
  CheckModule::check_functions(driver->mod);
  return driver;
}
typedef Trace * (*driver_runner)(TraceBuilder &TB, bool &assume_blocked);

void RFSCDriver::thread_runner(int id, RFSCDriver *driver, std::vector<RFSCTraceBuilder *> &TBs, moodycamel::BlockingConcurrentQueue<std::tuple<bool, Trace *, bool>> &queue) {
  bool assume_blocked = false;
  // llvm::dbgs() << "DEBUG:  Run once from " << id << "\n";
  if (!TBs[id]->reset()) {
    queue.enqueue(std::make_tuple(false, nullptr, false));
    return;
  };
  Trace *t= driver->run_once(*TBs[id], assume_blocked);

  // TODO:
  // By being able to move both compute unfolding, and compute prefix after returning trace from run_once,
  // we could send the trace to a producer and compute unfolding concurrently,
  // and later run comp_prefixes sequentially with a lock without hindering the trace consumer.
  TBs[id]->compute_unfolding();
  TBs[id]->compute_prefixes();

  queue.enqueue(std::make_tuple(true, std::move(t), assume_blocked));
}


DPORDriver::Result RFSCDriver::run(){

  Result res;

  moodycamel::BlockingConcurrentQueue<std::tuple<bool, Trace *, bool>> queue;
  std::tuple<bool, Trace *, bool> tup;

  RFSCDecisionTree decision_tree;
  RFSCUnfoldingTree unfolding_tree;

  ctpl::thread_pool threadpool(N_THREADS);

  std::vector<RFSCTraceBuilder*> TBs;
  for (int i = 0; i < N_THREADS; i++) {
    TBs.push_back(new RFSCTraceBuilder(decision_tree, unfolding_tree, conf));
  }

  auto fun = [this, &TBs, &queue] (int id) {thread_runner(id, this, TBs, queue);};

  decision_tree.run_func = fun;
  decision_tree.threadpool = &threadpool;
  
  // TraceBuilder *TB = new RFSCTraceBuilder(decision_tree, unfolding_tree, decision_tree.get_root(), conf);
  
  SigSegvHandler::setup_signal_handler();

  uint64_t computation_count = 0;
  long double estimate = 1;
  // threadpool.push(decision_tree.run_func);
  threadpool.push(fun);
  // threadpool.push(fun);
  threadpool.push(fun);
  do{
    if(conf.print_progress){
      print_progress(computation_count, estimate, res);
    }
    if((computation_count+1) % 1000 == 0){
      reparse();
    }

    // auto fun = [this, &TBs, &queue] (int id) {thread_runner(id, this, TBs, queue);};

    

    // threadpool.push([this, &TBs, &queue] (int id) {thread_runner(id, this, TBs, queue);});

    // threadpool.push(fun);

    // threadpool.push([this, &TBs, &queue](int id){
    //   bool assume_blocked = false;
    //   // llvm::dbgs() << "DEBUG:  Run once from " << id << "\n";
    //   TBs[id]->reset();
    //   Trace *t= this->run_once(*TBs[id], assume_blocked);

    //   // TODO:
    //   // By being able to move both compute unfolding, and compute prefix after returning trace from run_once,
    //   // we could send the trace to a producer and compute unfolding concurrently,
    //   // and later run comp_prefixes sequentially with a lock without hindering the trace consumer.
    //   TBs[id]->compute_unfolding();
    //   TBs[id]->compute_prefixes();

    //   queue.enqueue(std::pair<Trace *, bool>(std::move(t), assume_blocked));
    // });


    queue.wait_dequeue(tup);
    bool working;
    Trace *t;
    bool assume_blocked; // = p.second;

    std::tie(working, t, assume_blocked) = tup;
    if (!working) break;

    if (handle_trace(TBs[0], t, &computation_count, res, assume_blocked)) break;
    if(conf.print_progress_estimate && (computation_count+1) % 100 == 0){
      estimate = std::round(TBs[0]->estimate_trace_count());
    }

    // if (decision_tree.work_queue_empty()) break;
    
    // delete TBs[0];
    // TBs[0] = new RFSCTraceBuilder(decision_tree, unfolding_tree, conf);
  } while(true);

  if(conf.print_progress){
    llvm::dbgs() << ESC_char << "[K\n";
  }

  SigSegvHandler::reset_signal_handler();

  delete TBs[0];

  return res;  
}