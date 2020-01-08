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


#define N_THREADS 4

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


DPORDriver::Result RFSCDriver::run(){

  Result res;

  moodycamel::BlockingConcurrentQueue<std::tuple<bool, Trace *, bool, int>> queue;
  std::tuple<bool, Trace *, bool, int> tup;

  RFSCDecisionTree decision_tree;
  RFSCUnfoldingTree unfolding_tree;

  ctpl::thread_pool threadpool(N_THREADS);

  std::vector<RFSCTraceBuilder*> TBs;
  for (int i = 0; i < N_THREADS; i++) {
    TBs.push_back(new RFSCTraceBuilder(decision_tree, unfolding_tree, conf));
  }

  auto thread_runner = [this, &TBs, &queue] (int id) {
    bool assume_blocked = false;
    // llvm::dbgs() << "DEBUG:  Run once from " << id << "\n";
    TBs[id]->reset();
    Trace *t= this->run_once(*TBs[id], assume_blocked);

    queue.enqueue(std::make_tuple(true, std::move(t), assume_blocked, TBs[id]->tasks_created));
    };

  // TODO: Letting the decision tree have the ability to directly push a new thread-task after a work_item have been inserted into wq
  decision_tree.run_func = thread_runner;
  decision_tree.threadpool = &threadpool;
  
  SigSegvHandler::setup_signal_handler();

  uint64_t computation_count = 0;
  long double estimate = 1;

  // Initialize the first run.
  threadpool.push(thread_runner);
  int tasks_left = 1;

  do{
    if(conf.print_progress){
      print_progress(computation_count, estimate, res);
    }
    if((computation_count+1) % 1000 == 0){
      reparse();
    }


    queue.wait_dequeue(tup);
    tasks_left--;

    bool working;
    Trace *t;
    bool assume_blocked;
    int to_create;
    std::tie(working, t, assume_blocked, to_create) = tup;


    if (handle_trace(TBs[0], t, &computation_count, res, assume_blocked)) break;
    if(conf.print_progress_estimate && (computation_count+1) % 100 == 0){
      estimate = std::round(TBs[0]->estimate_trace_count());
    }
    tasks_left += to_create;
    // for(int i = 0; i < to_create; i++) {
    //   threadpool.push(thread_runner);
    //   // tasks_left++;
    // }

  } while(tasks_left);

  if(conf.print_progress){
    llvm::dbgs() << ESC_char << "[K\n";
  }

  SigSegvHandler::reset_signal_handler();

  // delete TBs[0];
  // while(queue.try_dequeue(tup));
  TBs.clear();

  return res;  
}