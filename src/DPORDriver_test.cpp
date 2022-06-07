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
    conf.explore_all_traces = true;
    return conf;
  }

  const Configuration &get_sc_conf(){
    static Configuration conf;
    conf.memory_model = Configuration::SC;
    conf.debug_collect_all_traces = true;
    conf.explore_all_traces = true;
    conf.commute_rmws = true;
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

  trace_set_spec spec_xprod(const std::vector<trace_set_spec> &specs){
    trace_set_spec res{{}}; /* Singleton set of the empty spec */

    for (const trace_set_spec &set_spec : specs) {
      trace_set_spec newres;
      for (const trace_spec speca : res)
        for (const trace_spec specb : set_spec) {
          trace_spec spec = speca;
          spec.insert(spec.end(), specb.begin(), specb.end());
          newres.push_back(std::move(spec));
        }
      res = std::move(newres);
    }
    return res;
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
                        const Configuration &conf,
                        const DPORDriver::Result *optimal){
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
    if((res.trace_count + res.assume_blocked_trace_count) != spec.size()){
      llvm::dbgs() << "DPORDriver_test::check_all_traces: expected number of traces: "
                   << spec.size() << ", actual number: "
                   << res.trace_count << "+" << res.assume_blocked_trace_count
                   << " (" << res.all_traces.size() << ")\n";
      retval = false;
    }
    std::vector<int> t2e(res.all_traces.size(),-1);
    for(unsigned i = 0; i < spec.size(); ++i){
      bool found = false;
      int prev_match = -1;
      for(unsigned j = 0; j < res.all_traces.size(); ++j){
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
    if (optimal) {
      if (!check_optimal_equiv(res, *optimal, conf)) retval = false;
      if (!check_all_traces(*optimal, spec, conf)) retval = false;
    }
    return retval;
  }

  static bool error_equiv(const Error *a, const Error *b){
    return a->get_location() == b->get_location()
      && a->to_string() == b->to_string();
  }

  static bool trace_equiv(const Trace *a, const Trace *b){
    if (a->get_errors().size() != b->get_errors().size()) return false;
    std::list<Error*> bes;
    for (Error *be : b->get_errors()) {
      bes.push_back(be);
    }
    for (Error *ae : a->get_errors()) {
      auto bei = std::find_if(bes.begin(), bes.end(), [ae](const Error *be){
          return error_equiv(ae, be);
        });
      if (bei == bes.end()) return false;
      bei = bes.erase(bei);
    }
    return true;
  }

  bool check_optimal_equiv(const DPORDriver::Result &source,
                           const DPORDriver::Result &optimal,
                           const Configuration &conf){
    bool retval = true;
    if (optimal.trace_count != source.trace_count) {
      llvm::dbgs() << "DPORDriver_test::check_optimal_equiv: "
                   << "Mismatching Source and Optimal trace counts; "
                   << source.trace_count << " and " << optimal.trace_count
                   << ".\n";
      retval = false;
    }
    for (auto oti = optimal.all_traces.cbegin();
         oti != optimal.all_traces.cend(); ++oti) {
      if ((*oti)->is_blocked() && !(*oti)->has_errors()) {
        llvm::dbgs() << "DPORDriver_test::check_optimal_equiv: Optimal trace #"
                     << (oti - optimal.all_traces.cbegin())
                     << " is sleepset blocked.\n";
        retval = false;
      }
    }
    if (optimal.has_errors() != source.has_errors()) {
      llvm::dbgs() << "DPORDriver_test::check_optimal_equiv: "
                   << "Only " << (optimal.has_errors() ? "Optimal" : "Source")
                   << "-DPOR reported an error.\n";
      retval = false;
    }

    if(!conf.explore_all_traces){
      BOOST_WARN_MESSAGE(false,"DPORDriver_test::check_optimal_equiv requires debug_collect_all_traces and explore_all_traces.");
      return retval;
    }

    std::list<const Trace*> sts, ots;
    for (const Trace *t : source.all_traces) if(t->has_errors()) sts.push_back(t);
    for (const Trace *t : optimal.all_traces) if(t->has_errors()) ots.push_back(t);

    for (auto sti = sts.begin(); sti != sts.end();) {
      auto oti = std::find_if(ots.begin(), ots.end(), [&sti](const Trace *ot){
          return trace_equiv(*sti, ot);
        });
      if (oti == ots.end()) {
        ++sti;
        continue;
      }
      sti = sts.erase(sti);
      oti = ots.erase(oti);
    }

    if (sts.size()) {
      llvm::dbgs() << "DPORDriver_test::check_optimal_equiv: "
                   << sts.size() << " traces found by Source-DPOR does not have"
                   << " any error-equivalents found by Optimal-DPOR.\n";
      retval = false;
    }
    if (ots.size()) {
      llvm::dbgs() << "DPORDriver_test::check_optimal_equiv: "
                   << ots.size() << " traces found by Optimal-DPOR does not have"
                   << " any error-equivalents found by Source-DPOR.\n";
      retval = false;
    }

    return retval;
  }
}  // namespace DPORDriver_test

#endif
