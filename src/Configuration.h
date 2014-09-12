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

#include <config.h>

#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

#include "vecset.h"

#include <set>
#include <string>

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
    PSO,
    TSO
  };
  /* Assign default values to all configuration parameters. */
  Configuration(){
    explore_all_traces = false;
    malloc_may_fail = false;
    max_search_depth = -1;
    memory_model = MM_UNDEF;
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
      "__assert_fail"
    };
    check_robustness = false;
    debug_collect_all_traces = false;
    debug_print_on_reset = false;
    debug_print_on_error = false;
    transform_spin_assume = false;
    transform_loop_unroll = -1;
  };
  /* Read the switches given to the program by the user. Assign
   * configuration options accordingly.
   */
  void assign_by_commandline();

  /* Should analysis continue, even when an error was discovered,
   * until all traces have been explored?
   */
  bool explore_all_traces;
  /* Should the analysis consider that malloc may fail and return
   * null? If set, then the analysis will also consider traces where
   * malloc failure is simulated.
   */
  bool malloc_may_fail;
  /* If non-negative, limit the number of instructions that may be
   * executed by each thread to max_search_depth. If negative, threads
   * may continue to run until they terminate by themselves.
   */
  int max_search_depth;
  /* Which memory model should be assumed? */
  MemoryModel memory_model;
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
  /* If transform_loop_unroll is non-negative, in module
   * transformation, enable loop unrolling with depth
   * transform_loop_unroll.
   */
  int transform_loop_unroll;

  /* The set of all commandline switches that are associated with
   * setting configuration options. This set has nothing to do with
   * which switches were actually given by the user.
   */
  static const std::set<std::string> &commandline_opts();
  /* A configuration where all options have their default values. */
  static const Configuration default_conf;
};

#endif
