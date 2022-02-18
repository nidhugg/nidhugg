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

#include "Configuration.h"

#include <llvm/Support/CommandLine.h>
#include "Debug.h"
#include "SmtlibSatSolver.h"

/* Please keep (non-hidden) option names to 23 characters and
 * descriptions wrapped to 47 characters.
 */

llvm::cl::list<std::string>
cl_program_arguments(llvm::cl::desc("[-- <program arguments>...]"),
                     llvm::cl::Positional,
                     llvm::cl::ZeroOrMore);
llvm::cl::opt<std::string>
cl_transform("transform",llvm::cl::init(""),
             llvm::cl::desc("Transform the input module and store it (as LLVM assembly) to OUTFILE."),
             llvm::cl::NotHidden,llvm::cl::value_desc("OUTFILE"));

static llvm::cl::opt<bool> cl_keep_going("keep-going",llvm::cl::NotHidden,
                                          llvm::cl::desc("Continue exploring all traces,\n"
                                                         "even after the first error (also -k)"));
static llvm::cl::alias cl_k("k",llvm::cl::Hidden,llvm::cl::desc("Alias for --keep-going"),
                            llvm::cl::aliasopt(cl_keep_going));
// Previous name
static llvm::cl::alias cl_explore_all("explore-all",llvm::cl::Hidden,
                                      llvm::cl::desc("Alias for --keep-going"),
                                      llvm::cl::aliasopt(cl_keep_going));

static llvm::cl::opt<bool> cl_malloc_may_fail("malloc-may-fail",llvm::cl::NotHidden,
                                              llvm::cl::desc("Also explore the case of malloc failure."));

static llvm::cl::opt<bool> cl_no_check_mutex_init("no-check-mutex-init",llvm::cl::NotHidden,
                                                  llvm::cl::desc("If set, then allow use of mutexes without a\n"
                                                                 "preceding call to pthread_mutex_init.\n"
                                                                 "This switch is necessary when using static mutex\n"
                                                                 "initialization."));
// Previous name
static llvm::cl::alias cl_disable_mutex_init_requirement
("disable-mutex-init-requirement",llvm::cl::Hidden,
 llvm::cl::desc("Alias for --no-check-mutex-init"),
 llvm::cl::aliasopt(cl_no_check_mutex_init));

static llvm::cl::opt<int>
cl_max_search_depth("max-search-depth",
                    llvm::cl::NotHidden,llvm::cl::init(-1),
                    llvm::cl::desc("Bound the length of the analysed computations\n"
                                   "(# instructions/events per process)"));

static llvm::cl::opt<int>
cl_verifier_nondet_int("verifier-nondet-int",
                       llvm::cl::Hidden,llvm::cl::desc("The number to return from __VERIFIER_nondet_u?int"));

static llvm::cl::opt<Configuration::MemoryModel>
cl_memory_model(llvm::cl::NotHidden, llvm::cl::init(Configuration::MM_UNDEF),
                llvm::cl::desc("Select memory model"),
                llvm::cl::values(clEnumValN(Configuration::SC,"sc","Sequential Consistency"),
                                 clEnumValN(Configuration::ARM,"arm","The ARM model"),
                                 clEnumValN(Configuration::POWER,"power","The POWER model"),
                                 clEnumValN(Configuration::PSO,"pso","Partial Store Order"),
                                 clEnumValN(Configuration::TSO,"tso","Total Store Order")
#ifdef LLVM_CL_VALUES_USES_SENTINEL
                                ,clEnumValEnd
#endif
                                 ));

static llvm::cl::opt<bool> cl_c11("c11",llvm::cl::Hidden,
                                  llvm::cl::desc("Only consider c11 atomic accesses."));

#ifndef NO_SMTLIB_SOLVER
static llvm::cl::opt<Configuration::SatSolverEnum>
cl_sat(llvm::cl::NotHidden, llvm::cl::init(Configuration::SMTLIB),
       llvm::cl::desc("Select SAT solver"),
       llvm::cl::values(clEnumValN(Configuration::SMTLIB,"smtlib","External SMTLib process")
#  ifdef LLVM_CL_VALUES_USES_SENTINEL
                       ,clEnumValEnd
#  endif
                                 ));
#endif


static llvm::cl::opt<Configuration::DPORAlgorithm>
cl_dpor_algorithm(llvm::cl::NotHidden, llvm::cl::init(Configuration::SOURCE),
                  llvm::cl::desc("Select SMC algorithm"),
                  llvm::cl::values(clEnumValN(Configuration::SOURCE,"source","Source-DPOR (default)"),
                                   clEnumValN(Configuration::OPTIMAL,"optimal","Optimal-DPOR"),
                                   clEnumValN(Configuration::OBSERVERS,"observers","Optimal-DPOR with Observers"),
                                   clEnumValN(Configuration::READS_FROM,"rf","Optimal Reads-From-centric SMC")
#ifdef LLVM_CL_VALUES_USES_SENTINEL
                                  ,clEnumValEnd
#endif
                                 ));

static llvm::cl::opt<bool> cl_check_robustness("check-robustness",llvm::cl::NotHidden,
                                               llvm::cl::desc("Check for robustness as a correctness criterion."));
// Previous name
static llvm::cl::alias cl_robustness("robustness",llvm::cl::Hidden,
                                     llvm::cl::desc("Alias for --check-robustness"),
                                     llvm::cl::aliasopt(cl_check_robustness));

static llvm::cl::OptionCategory cl_transformation_cat("Module Transformation Passes");

static llvm::cl::opt<bool> cl_transform_no_spin_assume("no-spin-assume",llvm::cl::NotHidden,llvm::cl::cat(cl_transformation_cat));

static llvm::cl::opt<bool> cl_transform_spin_assume("spin-assume",llvm::cl::NotHidden,llvm::cl::cat(cl_transformation_cat),
                                                    llvm::cl::desc("Enable the spin assume pass in module\n"
                                                                   "transformation."));

enum class Tristate{
  DEFAULT,
  TRUE,
  FALSE,
};

static llvm::cl::opt<Tristate> cl_transform_assume_await
(llvm::cl::NotHidden,llvm::cl::cat(cl_transformation_cat), llvm::cl::init(Tristate::DEFAULT),
 llvm::cl::desc("Disable or enable the assume to await pass in module transformation."),
 llvm::cl::values(clEnumValN(Tristate::TRUE,"assume-await","Force-enable the assume to await pass in module transformation"),
                  clEnumValN(Tristate::FALSE,"no-assume-await","Disable the assume to await pass in module transformation"),
                  clEnumValN(Tristate::DEFAULT,"default-assume-await","Enable the assume to await pass in module"
                             " transformation\nif the current mode supports await-statements")
#ifdef LLVM_CL_VALUES_USES_SENTINEL
                  ,clEnumValEnd
#endif
                  ));

static llvm::cl::opt<bool> cl_transform_no_dead_code_elim
("no-dead-code-elim",llvm::cl::NotHidden,llvm::cl::cat(cl_transformation_cat),
 llvm::cl::desc("Disable the dead code elimination pass in module\n"
                "transformation."));

static llvm::cl::opt<bool> cl_transform_no_cast_elim
("no-cast-elim",llvm::cl::NotHidden,llvm::cl::cat(cl_transformation_cat),
 llvm::cl::desc("Disable the cast elimination pass in module\n"
                "transformation."));

static llvm::cl::opt<bool> cl_transform_no_partial_loop_purity
("no-partial-loop-purity",llvm::cl::NotHidden,llvm::cl::cat(cl_transformation_cat),
 llvm::cl::desc("Disable the partial loop purity bounding pass in module\n"
                "transformation."));

static llvm::cl::opt<int>
cl_transform_loop_unroll("unroll",
                         llvm::cl::NotHidden,llvm::cl::init(-1),llvm::cl::value_desc("N"),
                         llvm::cl::cat(cl_transformation_cat),
                         llvm::cl::desc("Bound executions by allowing loops to iterate at\n"
                                        "most N times."));

static llvm::cl::opt<bool> cl_print_progress("print-progress",llvm::cl::NotHidden,
                                             llvm::cl::desc("Continually print analysis progress to stdout."));

static llvm::cl::opt<bool> cl_print_progress_estimate("print-progress-estimate",llvm::cl::NotHidden,
                                                      llvm::cl::desc("Continually print analysis progress and trace\n"
                                                                     "number estimate to stdout."));

static llvm::cl::list<std::string> cl_extfun_no_race("extfun-no-race",llvm::cl::NotHidden,
                                                         llvm::cl::value_desc("FUN"),
                                                         llvm::cl::desc("Assume that the external function FUN, when called\n"
                                                                        "as blackbox, does not participate in any races.\n"
                                                                        "(See manual) May be given multiple times."));
static llvm::cl::opt<bool> cl_commute_rmws("commute-rmws",llvm::cl::NotHidden,llvm::cl::cat(cl_transformation_cat),
                                           llvm::cl::desc("Allow RMW operations to commute."));
static llvm::cl::opt<bool> cl_no_commute_rmws("no-commute-rmws",llvm::cl::NotHidden,llvm::cl::cat(cl_transformation_cat),
                                              llvm::cl::desc("Do not allow RMW operations to commute."));


static llvm::cl::opt<bool> cl_debug_print_on_reset
("debug-print-on-reset",llvm::cl::Hidden,
 llvm::cl::desc("Print debug info after exploring each trace."));

static llvm::cl::opt<int> cl_n_threads("n-threads",
                                       llvm::cl::NotHidden,llvm::cl::init(1),
                                       llvm::cl::desc("Number of threads to run")
                                      );

static llvm::cl::opt<Configuration::ExplorationScheduler> cl_exploration_scheduler
("exploration-scheduler",llvm::cl::NotHidden,llvm::cl::init(Configuration::WORKSTEALING),
 llvm::cl::desc("Scheduler to use when exploring concurrently\n"
                "(see --n-threads)"),
 llvm::cl::values(clEnumValN(Configuration::PRIOQUEUE,"prioqueue",
                             "A single priority queue"),
                  clEnumValN(Configuration::WORKSTEALING,"workstealing",
                             "A workstealing scheduler (default)")
#ifdef LLVM_CL_VALUES_USES_SENTINEL
                  ,clEnumValEnd
#endif
                  ));

const std::set<std::string> &Configuration::commandline_opts(){
  static std::set<std::string> opts = {
    "cpubind-pack",
    "keep-going",
    "extfun-no-race",
    "exploration-scheduler",
    "malloc-may-fail",
    "no-check-mutex-init",
    "max-search-depth",
    "n-threads",
    "no-cpubind","no-cpubind-singlify",
    "sc","tso","pso","power","arm",
    "smtlib",
    "source","optimal","observers","rf",
    "check-robustness",
    "spin-assume",
    "no-partial-loop-purity",
    "assume-await","no-assume-await","default-assume-await",
    "unroll",
    "print-progress",
    "print-progress-estimate",
    "no-commute-rmws",
  };
  return opts;
}

const Configuration Configuration::default_conf;

void Configuration::assign_by_commandline(){
  explore_all_traces = cl_keep_going;
  for(std::string f : cl_extfun_no_race){
    extfun_no_full_memory_conflict.insert(f);
  }
  n_threads = cl_n_threads;
  exploration_scheduler = cl_exploration_scheduler;
  malloc_may_fail = cl_malloc_may_fail;
  mutex_require_init = !cl_no_check_mutex_init;
  max_search_depth = cl_max_search_depth;
  memory_model = cl_memory_model;
  c11 = cl_c11;
  dpor_algorithm = cl_dpor_algorithm;
  check_robustness = cl_check_robustness;
  transform_spin_assume = cl_transform_spin_assume;
  transform_dead_code_elim = !cl_transform_no_dead_code_elim;
  transform_cast_elim = !cl_transform_no_cast_elim;
  transform_partial_loop_purity = !cl_transform_no_partial_loop_purity;
  if (cl_transform_assume_await == Tristate::DEFAULT) {
    /* Source-DPOR and TSO probably work too, and maybe also Observers,
     * but Optimal-DPOR is the only one we've formally proven correct,
     * for now */
    transform_assume_await = (memory_model == SC
                              && dpor_algorithm == Configuration::OPTIMAL);
  } else {
    transform_assume_await = cl_transform_assume_await == Tristate::TRUE;
  }
  transform_loop_unroll = cl_transform_loop_unroll;
  if (cl_verifier_nondet_int.getNumOccurrences())
    svcomp_nondet_int = (int)cl_verifier_nondet_int;
  print_progress = cl_print_progress || cl_print_progress_estimate;
  print_progress_estimate = cl_print_progress_estimate;
  debug_print_on_reset = cl_debug_print_on_reset;
  commute_rmws = !cl_no_commute_rmws && memory_model == SC;
#ifndef NO_SMTLIB_SOLVER
  sat_solver = cl_sat;
#endif
  argv.resize(1);
  argv[0] = get_default_program_name();
  for(std::string a : cl_program_arguments){
    argv.push_back(a);
  }
}

void Configuration::check_commandline(){
  /* Check commandline switch compatibility with --transform. */
  if(!cl_transform.getNumOccurrences()){
    if(cl_transform_spin_assume.getNumOccurrences()){
      Debug::warn("Configuration::check_commandline:no:transform:transform-no-spin-assume")
        << "WARNING: --spin-assume ignored in absence of --transform.\n";
    }
    if(cl_transform_assume_await.getNumOccurrences()){
      Debug::warn("Configuration::check_commandline:no:transform:transform-assume-await")
        << "WARNING: --no-assume-await ignored in absence of --transform.\n";
    }
    if(cl_transform_loop_unroll.getNumOccurrences()){
      Debug::warn("Configuration::check_commandline:no:transform:transform_loop_unroll")
        << "WARNING: --unroll ignored in absence of --transform.\n";
    }
    if(cl_transform_no_cast_elim.getNumOccurrences()){
      Debug::warn("Configuration::check_commandline:no:transform:transform-no-cast-elim")
        << "WARNING: --no-cast-elim ignored in absence of --transform.\n";
    }
    if(cl_transform_no_partial_loop_purity.getNumOccurrences()){
      Debug::warn("Configuration::check_commandline:no:transform:transform-no-partial-loop-purity")
        << "WARNING: --no-partial-loop-purity ignored in absence of --transform.\n";
    }
  }
  /* Check commandline switch compatibility with memory model. */
  {
    std::string mm;
    if(cl_memory_model == Configuration::SC) mm = "SC";
    if(cl_memory_model == Configuration::TSO) mm = "TSO";
    if(cl_memory_model == Configuration::PSO) mm = "PSO";
    if(cl_memory_model == Configuration::POWER) mm = "POWER";
    if(cl_memory_model == Configuration::ARM) mm = "ARM";
    if(cl_memory_model == Configuration::ARM || cl_memory_model == Configuration::POWER){
      if(cl_extfun_no_race.getNumOccurrences()){
        Debug::warn("Configuration::check_commandline:mm:extfun-no-race")
          << "WARNING: --extfun-no-race ignored under memory model " << mm << ".\n";
      }
      if(cl_malloc_may_fail.getNumOccurrences()){
        Debug::warn("Configuration::check_commandline:mm:malloc-may-fail")
          << "WARNING: --malloc-may-fail ignored under memory model " << mm << ".\n";
      }
      if(cl_max_search_depth.getNumOccurrences()){
        Debug::warn("Configuration::check_commandline:mm:max-search-depth")
          << "WARNING: --max-search-depth ignored under memory model " << mm << ".\n";
      }
      if(cl_check_robustness.getNumOccurrences()){
        Debug::warn("Configuration::check_commandline:mm:robustness")
          << "WARNING: --robustness ignored under memory model " << mm << ".\n";
      }
    }
    if ((cl_dpor_algorithm == Configuration::OPTIMAL
         || cl_dpor_algorithm == Configuration::OBSERVERS)
        && cl_memory_model != Configuration::SC
        && cl_memory_model != Configuration::TSO) {
      Debug::warn("Configuration::check_commandline:dpor:mm")
        << "WARNING: Optimal-DPOR not implemented for memory model " << mm << ".\n";
    }
    if (cl_dpor_algorithm == Configuration::READS_FROM
        && cl_memory_model != Configuration::SC) {
      Debug::warn("Configuration::check_commandline:dpor:mm")
        << "WARNING: Optimal Reads-From-SMC not implemented for memory model " << mm << ".\n";
    }

    if (cl_c11 && cl_memory_model != Configuration::SC) {
      Debug::warn("Configuration::check_commandline:c11:mm")
        << "WARNING: --c11 is not yet implemented for memory model " << mm << ".\n";
    }
  }

  if (cl_commute_rmws) {
    Debug::warn("Configuration::check_commandline:commute-rmws:implicit")
      << "WARNING: --commute-rmws is now default, ignoring.\n";
    if (cl_no_commute_rmws) {
      Debug::warn("Configuration::check_commandline:commute-rmws:contradiction")
        << "WARNING: Both --commute-rmws and --no-commute-rmws given, "
        << "taking --no-commute-rmws.\n";
    }
  }

  /* Warn about the --c11 switch */
  if (cl_c11) {
    Debug::warn("Configuration::check_commandline:c11:no-race-detect")
      << "WARNING: The race detector for --c11 is not yet implemented."
      << " Bugs might be missed or cause nondeterminism.\n";
  }
}

std::unique_ptr<SatSolver> Configuration::get_sat_solver() const {
  switch (sat_solver) {
#ifndef NO_SMTLIB_SOLVER
  case SMTLIB:
    return std::make_unique<SmtlibSatSolver>();
#endif
  }
  llvm::errs() << "Error: SC Consistency Decision Procedure required but not"
    " implemented\n";
  abort();
}
