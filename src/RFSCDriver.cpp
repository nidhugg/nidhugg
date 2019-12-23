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
#include "RFSCTraceBuilder.h"
#include "RFSCUnfoldingTree.h"

#include "ctpl.h"


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

  RFSCDecisionTree decision_tree;
  RFSCUnfoldingTree unfolding_tree;

  ctpl::thread_pool threadpool(1);

  
  TraceBuilder *TB = new RFSCTraceBuilder(decision_tree, unfolding_tree, decision_tree.get_root(), conf);
  
  SigSegvHandler::setup_signal_handler();

  uint64_t computation_count = 0;
  long double estimate = 1;
  do{
    if(conf.print_progress){
      print_progress(computation_count, estimate, res);
    }
    if((computation_count+1) % 1000 == 0){
      reparse();
    }


    auto future_trace = threadpool.push([this, &TB](int id){
      bool assume_blocked = false;
      llvm::dbgs() << "DEBUG:  Run once from " << id << "\n";
      Trace *t= this->run_once(*TB, assume_blocked);
      return std::pair<Trace *, bool>(std::move(t), assume_blocked);
    });
    
    // auto future_trace = std::async(std::launch::async, [this, &TB](){
    //   bool assume_blocked = false;
    //   Trace *t= this->run_once(*TB, assume_blocked);
    //   return std::pair<Trace *, bool>(std::move(t), assume_blocked);
    // });

    auto future_result = future_trace.get();
    Trace *t = future_result.first;
    bool assume_blocked = future_result.second;

    if (handle_trace(TB, t, &computation_count, res, assume_blocked)) break;
    if(conf.print_progress_estimate && (computation_count+1) % 100 == 0){
      estimate = std::round(TB->estimate_trace_count());
    }

    if (decision_tree.work_queue_empty()) break;
    
    delete TB;
    TB = new RFSCTraceBuilder(decision_tree, unfolding_tree, decision_tree.get_next_work_task(), conf);
  } while(true);

  if(conf.print_progress){
    llvm::dbgs() << ESC_char << "[K\n";
  }

  SigSegvHandler::reset_signal_handler();

  delete TB;

  return res;  
}