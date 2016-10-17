/* Copyright (C) 2014-2016 Carl Leonardsson
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
#ifdef HAVE_BOOST_UNIT_TEST_FRAMEWORK

#ifndef __DPOR_DRIVER_TEST_H__
#define __DPOR_DRIVER_TEST_H__

#include "CPid.h"
#include "IID.h"
#include "DPORDriver.h"

#include <vector>

/* This namespace provides methods that facilitate for unit testing to
 * check the set of traces produced by DPORDriver.
 */
namespace DPORDriver_test {

  /* A configuration with memory_model == TSO (resp. SC, PSO), and
   * debug_collect_all_traces == true.
   */
  const Configuration &get_tso_conf();
  const Configuration &get_sc_conf();
  const Configuration &get_pso_conf();
  const Configuration &get_power_conf();
  const Configuration &get_arm_conf();

  /* An IIDOrder object {a,b} should be interpreted as a predicate
   * over traces, with the meaning "a precedes b in time".
   */
  struct IIDOrder{
    IID<CPid> a;
    IID<CPid> b;
  };

  /* A trace_spec {o0,o1,...,on} is a predicate over traces, which is
   * the conjunction of o0, o1, ..., on.
   */
  typedef std::vector<IIDOrder> trace_spec;

  /* A trace_set_spec {s0,s1,...,sn} is a predicate over sets S of
   * traces, which has the meaning "The cardinality of S is n+1 and
   * for each trace t in S there is exactly one trace_spec si such
   * that t satisfies si.", i.e., there is a one-to-one correspondence
   * between traces in S and trace_specs in the trace_set_spec.
   */
  typedef std::vector<trace_spec> trace_set_spec;

  /* Returns true iff t satisfies spec. */
  bool check_trace(const Trace &t, const trace_spec &spec);

  /* Returns true iff the set of non-blocked traces in res.all_traces
   * satisfies spec.
   *
   * If conf.debug_collect_all_traces == false, then this function
   * returns true without performing any tests, and emits a warning
   * using BOOST_WARN_MESSAGE.
   */
  bool check_all_traces(const DPORDriver::Result &res,
                        const trace_set_spec &spec,
                        const Configuration &conf);

}

#endif
#endif
