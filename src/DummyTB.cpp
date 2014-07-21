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

#include "DummyTB.h"

DummyTB::DummyTB(const Configuration &c) : TraceBuilder(c) {
  cpids.push_back(CPid());
  available.push_back(true);
  pcs.push_back(0);
};

bool DummyTB::schedule(int *proc, int *aux, int *alt, bool *dryrun){
  *aux = -1;
  *alt = 0;
  *dryrun = false;
  for(unsigned p = 0; p < available.size(); ++p){
    if(available[p]){
      /* Hack to enable sleeping for one instruction in P0. */
      if(p == 0 && pcs[p] == 1 && pcs.size() == 2 && pcs[1] == 0){
        last_proc = *proc = p;
        *dryrun = true;
        available[p] = false;
        return true;
      }
      if(p == 1 && pcs[p] == 0){
        available[0] = true;
      }
      last_proc = *proc = p;
      ++pcs[p];
      cmp.push_back({cpids[p],pcs[p]});
      return true;
    }
  }
  return false;
};

void DummyTB::refuse_schedule(){
  cmp.pop_back();
  --pcs[last_proc];
  available[last_proc] = false;
};

void DummyTB::spawn(){
  cpids.push_back(CPS.spawn(cpids[last_proc]));
  available.push_back(true);
  pcs.push_back(0);
};

Trace DummyTB::get_trace() const{
  std::vector<Error*> errs;
  for(unsigned i = 0; i < errors.size(); ++i){
    errs.push_back(errors[i]->clone());
  }
  std::vector<const llvm::MDNode*> mds(cmp.size(),0);
  return Trace(cmp,mds,errs);
};

void DummyTB::assertion_error(std::string cond){
  errors.push_back(new AssertionError(cmp.back(),cond));
};

void DummyTB::pthreads_error(std::string msg){
  errors.push_back(new PthreadsError(cmp.back(),msg));
};

void DummyTB::segmentation_fault_error(){
  errors.push_back(new SegmentationFaultError(cmp.back()));
};
