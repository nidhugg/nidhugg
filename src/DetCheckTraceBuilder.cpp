/* Copyright (C) 2017 Carl Leonardsson
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

#include "DetCheckTraceBuilder.h"

DetCheckTraceBuilder::DetCheckTraceBuilder(const Configuration &conf) : TraceBuilder(conf){
  reset_cond_branch_log();
}

void DetCheckTraceBuilder::reset_cond_branch_log(){
  cond_branch_log_index = 0;
}

bool DetCheckTraceBuilder::cond_branch(bool cnd){
  if(is_replaying()){
    assert(cond_branch_log_index < cond_branch_log.size());
    bool logged = cond_branch_log[cond_branch_log_index];
    ++cond_branch_log_index;
    if(logged != cnd){
      nondeterminism_error("Branch condition value changed in replay.");
      return false;
    }
  }else{
    if(cond_branch_log_index < cond_branch_log.size()){
      cond_branch_log[cond_branch_log_index] = cnd;
    }else{
      assert(cond_branch_log_index == cond_branch_log.size());
      cond_branch_log.push_back(cnd);
    }
    ++cond_branch_log_index;
  }
  return true;
}
