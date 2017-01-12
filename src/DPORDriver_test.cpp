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
#ifdef HAVE_BOOST_UNIT_TEST_FRAMEWORK

#include "DPORDriver_test.h"
#include "VClock.h"

#include <sstream>

#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>

#include <boost/test/unit_test.hpp>

namespace DPORDriver_test {

  const Configuration &get_tso_conf(){
    static Configuration conf;
    conf.memory_model = Configuration::TSO;
    conf.debug_collect_all_traces = true;
    return conf;
  }

  const Configuration &get_sc_conf(){
    static Configuration conf;
    conf.memory_model = Configuration::SC;
    conf.debug_collect_all_traces = true;
    return conf;
  }

  const Configuration &get_pso_conf(){
    static Configuration conf;
    conf.memory_model = Configuration::PSO;
    conf.debug_collect_all_traces = true;
    return conf;
  }

  const Configuration &get_power_conf(){
    static Configuration conf;
    conf.memory_model = Configuration::POWER;
    conf.debug_collect_all_traces = true;
    return conf;
  }

  const Configuration &get_arm_conf(){
    static Configuration conf;
    conf.memory_model = Configuration::ARM;
    conf.debug_collect_all_traces = true;
    return conf;
  }

  std::string trace_spec_to_string(const trace_spec &v){
    std::stringstream ss;
    ss << "{";
    for(unsigned i = 0; i < v.size(); ++i){
      if(i != 0) ss << ", ";
      ss << v[i].a << " -> " << v[i].b;
    }
    return ss.str() + "}";
  }

  void print_trace(const Trace *_t, llvm::raw_ostream &os, int ind){
    os << _t->to_string(ind);
  }

  int find(const IIDSeqTrace *_t, const IID<CPid> &iid){
    const std::vector<IID<CPid> > &t = _t->get_computation();
    for(int i = 0; i < int(t.size()); ++i){
      if(t[i] == iid){
        return i;
      }
    }
    return -1;
  }

  bool check_trace(const IIDSeqTrace *t, const trace_spec &spec){
    for(unsigned i = 0; i < spec.size(); ++i){
      int a_i = find(t,spec[i].a);
      int b_i = find(t,spec[i].b);
      if(a_i == -1 || b_i == -1 || b_i <= a_i){
        return false;
      }
    }
    return true;
  }

  /* Returns true iff the indices of the IIDs of each process is
   * strictly increasing along the computation of t. */
  bool all_clocks_increasing(const IIDSeqTrace *t){
    VClock<CPid> pcs;
    for(auto it = t->get_computation().begin(); it != t->get_computation().end(); ++it){
      if(it->get_index() <= pcs[it->get_pid()]) return false;
      pcs[it->get_pid()] = it->get_index();
    }
    return true;
  }

  bool check_all_traces(const DPORDriver::Result &res,
                        const trace_set_spec &spec,
                        const Configuration &conf){
    if(!conf.debug_collect_all_traces){
      BOOST_WARN_MESSAGE(false,"DPORDriver_test::check_all_traces requires debug_collect_all_traces.");
      return true;
    }
    bool retval = true;
    for(unsigned i = 0; i < res.all_traces.size(); ++i){
      if(!all_clocks_increasing(static_cast<const IIDSeqTrace*>(res.all_traces[i]))){
        llvm::dbgs() << "DPORDriver_test::check_all_traces: "
                     << "Non increasing instruction index in trace.\n";
        retval = false;
      }
    }
    if(res.trace_count != int(spec.size())){
      llvm::dbgs() << "DPORDriver_test::check_all_traces: expected number of traces: "
                   << spec.size() << ", actual number: "
                   << res.trace_count
                   << " (" << res.all_traces.size() << ")\n";
      retval = false;
    }
    std::vector<int> t2e(res.all_traces.size(),-1);
    for(unsigned i = 0; i < spec.size(); ++i){
      bool found = false;
      int prev_match = -1;
      for(unsigned(j) = 0; j < res.all_traces.size(); ++j){
        if(res.all_traces[j]->is_blocked()) continue;
        if(check_trace(static_cast<const IIDSeqTrace*>(res.all_traces[j]),spec[i])){
          if(found){
            // Multiple traces match the same specification
            llvm::dbgs() << "DPORDriver_test::check_all_traces: Multiple traces (#"
                         << prev_match+1
                         << " and #"
                         << j+1
                         << ") match the same specification:\n";
            llvm::dbgs() << "#" << prev_match+1 << ":\n"
                         << res.all_traces[prev_match]->to_string(2)
                         << "\n#" << j+1 << ":\n"
                         << res.all_traces[j]->to_string(2);
            llvm::dbgs() << "  " << trace_spec_to_string(spec[i]) << "\n";
            t2e[j] = i;
            retval = false;
          }else{
            prev_match = j;
            found = true;
            if(t2e[j] != -1){
              // Multiple specifications match the same trace
              llvm::dbgs() << "DPORDriver_test::check_all_traces: A trace matches multiple specifications:\n";
              print_trace(res.all_traces[j],llvm::dbgs(),2);
              retval = false;
            }
            t2e[j] = i;
          }
        }
      }
      if(!found){
        llvm::dbgs() << "DPORDriver_test::check_all_traces: A specification is not matched by any trace:\n";
        llvm::dbgs() << "  " << trace_spec_to_string(spec[i]) << "\n";
        retval = false;
      }
    }
    for(unsigned i = 0; i < t2e.size(); ++i){
      if(!res.all_traces[i]->is_blocked() && t2e[i] < 0){
        llvm::dbgs() << "DPORDriver_test::check_all_traces: A trace is not matched by any specification: #" << i+1 << "\n";
        retval = false;
      }
    }
    return retval;
  }
}

#endif
