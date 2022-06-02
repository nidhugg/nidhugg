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

#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

#include "vecset.h"
#include "SatSolver.h"
#include "Option.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

/* A Configuration object keeps track of all configuration options
 * that should be used during a program analysis. The configuration
 * options can be manually set or decided by the program switches
 * given by the user.
 */
class Configuration{
public:
  enum MemoryModel{
    MM_UNDEF, // No memory model was specified
    SC,
    ARM,
    POWER,
    PSO,
    TSO
  };
  enum DPORAlgorithm{
    SOURCE,
    OPTIMAL,
    OBSERVERS,
    READS_FROM,
  };
  /* Assign default values to all configuration parameters. */
  Configuration(){
    n_threads = 1;
    explore_all_traces = false;
    malloc_may_fail = false;
    mutex_require_init = true;
    max_search_depth = -1;
    memory_model = MM_UNDEF;
    c11 = false;
    dpor_algorithm = SOURCE;
    extfun_no_fence = {
      "pthread_self",
      "malloc",
      "__VERIFIER_nondet_int",
      "__VERIFIER_nondet_uint",
      "__VERIFIER_assume",
      "printf",
      "puts"
    };
    extfun_no_full_memory_conflict = {
      "pthread_create",
      "pthread_join",
      "pthread_self",
      "pthread_exit",
      "pthread_mutex_init",
      "pthread_mutex_lock",
      "pthread_mutex_trylock",
      "pthread_mutex_unlock",
      "pthread_mutex_destroy",
      "malloc",
      "__VERIFIER_nondet_int",
      "__VERIFIER_nondet_uint",
      "__VERIFIER_assume",
      "__assert_fail",
      "atexit"
    };
    check_robustness = false;
    ee_store_trace = false;
    debug_collect_all_traces = false;
    debug_print_on_reset = false;
    debug_print_on_error = false;
    transform_spin_assume = false;
    transform_assume_await = false;
    transform_loop_unroll = -1;
    svcomp_nondet_int = nullptr;
    print_progress = false;
    print_progress_estimate = false;
    exploration_scheduler = WORKSTEALING;
#ifdef NO_SMTLIB_SOLVER
    sat_solver = NONE;
#else
    sat_solver = SMTLIB;
#endif
    argv.push_back(get_default_program_name());
  }
  /* Read the switches given to the program by the user. Assign
   * configuration options accordingly.
   */
  void assign_by_commandline();
  /* Read the switches given to the program by the user. Print
   * warnings if incompatible switches are supplied.
   */
  void check_commandline();

  /* Should analysis continue, even when an error was discovered,
   * until all traces have been explored?
   */
  bool explore_all_traces;
  /* Should the analysis consider that malloc may fail and return
   * null? If set, then the analysis will also consider traces where
   * malloc failure is simulated.
   */
  bool malloc_may_fail;
  /* Should it be required that each pthread mutex is initialized with
   * a call to pthread_mutex_init before use? Disabling
   * mutex_require_init will cause programs using static mutex
   * initialization to be accepted (as well as programs which
   * erroneously use mutexes without any initialization).
   */
  bool mutex_require_init;
  /* If non-negative, limit the number of instructions that may be
   * executed by each thread to max_search_depth. If negative, threads
   * may continue to run until they terminate by themselves.
   */
  int max_search_depth;
  /* Which memory model should be assumed? */
  MemoryModel memory_model;
  /* Should non-atomic accesses be ignored? */
  bool c11;
  /* Which DPOR algorithm should be used? */
  DPORAlgorithm dpor_algorithm;
  /* A set of names of external functions that should be assumed to
   * not have fencing behavior. Notice however that the function
   * itself will still execute atomically, which may cause behaviors
   * that violate the memory model in case the function makes visible
   * stores to memory.
   */
  VecSet<std::string> extfun_no_fence;
  /* A set of names of external functions that should not be assumed
   * to have a conflict with all memory accesses.
   *
   * For these functions, the implementation needs to explicitly state
   * which memory conflicts exist during execution (using
   * TraceBuilder::store, TraceBuilder::load, etc).
   */
  VecSet<std::string> extfun_no_full_memory_conflict;
  /* Should we check for robustness as a correctness criterion? */
  bool check_robustness;
  /* If this flag is set, the ExecutionEngine will log what happens
   * during the run to the TraceBuilder, so that the TraceBuilder can
   * produce a more detailed (human-readable) error trace.
   *
   * This flag is not intended to be set during normal operation. When
   * an error has been found, the erroneous run can be repeated, with
   * ee_store_trace set, in order to produce a readable error trace.
   */
  bool ee_store_trace;
  /* If set, all explored traces will be stored by DPORDriver until
   * the end of the analysis.
   *
   * WARNING: This quickly grows out of hand. Should be used only
   * for modules known to have few traces. Automated testing is the
   * intended usage.
   */
  bool debug_collect_all_traces;
  /* If set, the trace builder is instructed to debug print its state
   * at each reset.
   */
  bool debug_print_on_reset;
  /* If set, the trace builder is instructed to debug print its state
   * when an error is reported.
   */
  bool debug_print_on_error;
  /* In module transformation, enable the SpinAssume pass. */
  bool transform_spin_assume;
  /* In module transformation, enable the DeadCodeElim pass. */
  bool transform_dead_code_elim = true;
  /* In module transformation, enable the CastElim pass. */
  bool transform_cast_elim = true;
  /* In module transformation, enable the PartialLoopPurity pass. */
  bool transform_partial_loop_purity = true;
  /* In module transformation, enable the AssumeAwait pass. */
  bool transform_assume_await;
  /* If transform_loop_unroll is non-negative, in module
   * transformation, enable loop unrolling with depth
   * transform_loop_unroll.
   */
  int transform_loop_unroll;
  /* Number to return from __VERIFIER_nondet_u?int() */
  Option<int> svcomp_nondet_int;
  /* If set, rmws are allowed to commute. */
  bool commute_rmws = false;
  /* If set, DPORDriver will continually print its progress to stdout. */
  bool print_progress;
  /* If set and print_progress is set, DPORDriver will together with
   * the progress, also print its estimate of the total number of
   * traces.
   */
  bool print_progress_estimate;

  /* When running RFSC, Set the amount of threads that does the exploration.
   * The main thread will only consume results from the n-1 worker threads.
   * If n=1 the algorithm operates purely sequential.
   */
  int n_threads;

  /* Scheduler to use when exploring in parallel with --n-threads */
  enum ExplorationScheduler {
    PRIOQUEUE,
    WORKSTEALING,
  } exploration_scheduler;

  /* Sat solver to use. */
  enum SatSolverEnum {
#ifdef NO_SMTLIB_SOLVER
        NONE,
#else
        SMTLIB,
#endif
  } sat_solver;
  std::unique_ptr<SatSolver> get_sat_solver() const;
  /* The arguments that will be passed to the program under test */
  std::vector<std::string> argv;
  /* The default program name to send to the program under test as
   * argv[0].
   */
  static const std::string &get_default_program_name(){
    static const std::string pname = "a.out";
    return pname;
  }
  /* The set of all commandline switches that are associated with
   * setting configuration options. This set has nothing to do with
   * which switches were actually given by the user.
   */
  static const std::set<std::string> &commandline_opts();
  /* A configuration where all options have their default values. */
  static const Configuration default_conf;
};

#endif
