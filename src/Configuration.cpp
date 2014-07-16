/* Copyright (C) 2014 Carl Leonardsson
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

#include "Configuration.h"

#include <llvm/Support/CommandLine.h>

static llvm::cl::opt<bool> cl_explore_all("explore-all",llvm::cl::NotHidden,
                                          llvm::cl::desc("Continue exploring all traces, "
                                                         "even after the first error"));

static llvm::cl::opt<bool> cl_malloc_may_fail("malloc-may-fail",llvm::cl::NotHidden,
                                              llvm::cl::desc("If set then the case of malloc failure is also explored."));

static llvm::cl::opt<int>
cl_max_search_depth("max-search-depth",
                 llvm::cl::NotHidden,llvm::cl::init(-1),
                 llvm::cl::desc("Bound the length of the analysed computations (# instructions/events per process)"));

static llvm::cl::opt<Configuration::MemoryModel>
cl_memory_model(llvm::cl::NotHidden, llvm::cl::Required,
             llvm::cl::desc("Select memory model"),
             llvm::cl::values(clEnumValN(Configuration::SC,"sc","Sequential Consistency"),
                              clEnumValN(Configuration::PSO,"pso","Partial Store Order"),
                              clEnumValN(Configuration::TSO,"tso","Total Store Order"),
                              clEnumValEnd));

static llvm::cl::opt<bool> cl_check_robustness("robustness",llvm::cl::NotHidden,
                                              llvm::cl::desc("Check for robustness as a correctness criterion."));

const std::set<std::string> &Configuration::commandline_opts(){
  static std::set<std::string> opts = {
    "dpor-explore-all",
    "malloc-may-fail",
    "max-search-depth",
    "sc","tso","pso",
    "robustness"
  };
  return opts;
};

const Configuration Configuration::default_conf;

void Configuration::assign_by_commandline(){
  explore_all_traces = cl_explore_all;
  malloc_may_fail = cl_malloc_may_fail;
  max_search_depth = cl_max_search_depth;
  memory_model = cl_memory_model;
  check_robustness = cl_check_robustness;
};
