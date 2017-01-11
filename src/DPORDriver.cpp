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
#include "SigSegvHandler.h"
#include "StrModule.h"
#include "TSOInterpreter.h"
#include "TSOTraceBuilder.h"

#include <fstream>
#include <stdexcept>

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

DPORDriver::DPORDriver(const Configuration &C) :
  conf(C), mod(0) {
  std::string ErrorMsg;
  llvm::sys::DynamicLibrary::LoadLibraryPermanently(0,&ErrorMsg);
}

DPORDriver *DPORDriver::parseIRFile(const std::string &filename,
                                    const Configuration &C){
  DPORDriver *driver =
    new DPORDriver(C);
  read_file(filename,driver->src);
  driver->reparse();
  CheckModule::check_functions(driver->mod);
  return driver;
}

DPORDriver *DPORDriver::parseIR(const std::string &llvm_asm,
                                const Configuration &C){
  DPORDriver *driver =
    new DPORDriver(C);
  driver->src = llvm_asm;
  driver->reparse();
  CheckModule::check_functions(driver->mod);
  return driver;
}

DPORDriver::~DPORDriver(){
  delete mod;
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

void DPORDriver::reparse(){
  delete mod;
  mod = StrModule::read_module_src(src);
  if(mod->LLVM_MODULE_GET_DATA_LAYOUT_STRING().empty()){
    if(llvm::sys::IsLittleEndianHost){
      mod->setDataLayout("e");
    }else{
      mod->setDataLayout("E");
    }
  }
}

llvm::ExecutionEngine *DPORDriver::create_execution_engine(TraceBuilder &TB, const Configuration &conf) const {
  std::string ErrorMsg;
  llvm::ExecutionEngine *EE = 0;
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

Trace *DPORDriver::run_once(TraceBuilder &TB) const{
  std::shared_ptr<llvm::ExecutionEngine> EE(create_execution_engine(TB,conf));

  // Run main.
  EE->runFunctionAsMain(mod->getFunction("main"), {"prog"}, 0);

  // Run static destructors.
  EE->runStaticConstructorsDestructors(true);

  if(conf.check_robustness){
    static_cast<llvm::Interpreter*>(EE.get())->checkForCycles();
  }

  EE.reset();

  Trace *t = 0;
  if(TB.has_error() || conf.debug_collect_all_traces){
    if(TB.has_error() &&
       (conf.memory_model == Configuration::ARM ||
        conf.memory_model == Configuration::POWER)){
      static_cast<POWERARMTraceBuilder&>(TB).replay();
      Configuration conf2(conf);
      conf2.ee_store_trace = true;
      llvm::ExecutionEngine *E = create_execution_engine(TB,conf2);
      E->runFunctionAsMain(mod->getFunction("main"), {"prog"}, 0);
      E->runStaticConstructorsDestructors(true);
      if(conf.check_robustness){
        static_cast<llvm::Interpreter*>(E)->checkForCycles();
      }
      t = TB.get_trace();
      delete E;
    }else{
      t = TB.get_trace();
    }
  }// else avoid computing trace

  return t;
}

DPORDriver::Result DPORDriver::run(){
  Result res;

  TraceBuilder *TB = 0;

  switch(conf.memory_model){
  case Configuration::SC:
    TB = new TSOTraceBuilder(conf);
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

  SigSegvHandler::setup_signal_handler();

  char esc = 27;
  if(conf.print_progress){
    llvm::dbgs() << esc << "[s"; // Save terminal cursor position
  }

  int computation_count = 0;
  int estimate = 1;
  do{
    if(conf.print_progress && (computation_count+1) % 100 == 0){
      llvm::dbgs() << esc << "[u" // Restore cursor position
                   << esc << "[s" // Save cursor position
                   << esc << "[K" // Erase the line
                   << "Computation #" << computation_count+1;
      if(conf.print_progress_estimate){
        llvm::dbgs() << " ("
                     << int(100.0*float(computation_count+1)/float(estimate))
                     << "% of total estimate: "
                     << estimate << ")";
      }
    }
    if((computation_count+1) % 1000 == 0){
      reparse();
    }
    Trace *t = run_once(*TB);
    bool t_used = false;
    if(t && conf.debug_collect_all_traces){
      res.all_traces.push_back(t);
      t_used = true;
    }
    if(TB->sleepset_is_empty()){
      ++res.trace_count;
    }else{
      ++res.sleepset_blocked_trace_count;
    }
    ++computation_count;
    if(t && t->has_errors() && !res.has_errors()){
      res.error_trace = t;
      t_used = true;
    }
    bool has_errors = t && t->has_errors();
    if(!t_used){
      delete t;
    }
    if(has_errors && !conf.explore_all_traces) break;
    if(conf.print_progress_estimate && (computation_count+1) % 100 == 0){
      estimate = TB->estimate_trace_count();
    }
  }while(TB->reset());

  if(conf.print_progress){
    llvm::dbgs() << "\n";
  }

  SigSegvHandler::reset_signal_handler();

  delete TB;

  return res;
}
