/* Copyright (C) 2015-2017 Carl Leonardsson
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

#include "FBVClock.h"

#include <cassert>

std::vector<FBVClock::ClockSystem> FBVClock::sys;

FBVClock::ClockSystemID FBVClock::new_clock_system(){
  for(int i = 0; i < int(sys.size()); ++i){
    if(!sys[i].allocated){
      sys[i].allocated = true;
      return i;
    }
  }
  sys.emplace_back();
  return int(sys.size())-1;
}

void FBVClock::delete_clock_system(ClockSystemID cid){
  sys[cid] = ClockSystem(); // Clear old data
  sys[cid].allocated = false;
}

FBVClock::FBVClock(ClockSystemID cid, int idx) : cid(cid), idx(idx) {
  assert(0 <= cid && cid < int(sys.size()));
  ClockSystem &CS = sys[cid];
  id = int(CS.clocks.size());
  CS.clocks.emplace_back();
  CS.clocks.back().resize(idx+1,false);
  CS.clocks.back()[idx] = true;
  if(int(CS.idx_to_id.size()) <= idx) CS.idx_to_id.resize(idx+1);
  CS.idx_to_id[idx] = id;
  CS.change_ts.push_back(CS.time);
  CS.update_ts.push_back(CS.time);
}

bool FBVClock::operator[](int i){
  update();
  return i < int(sys[cid].clocks[id].size()) && sys[cid].clocks[id][i];
}

FBVClock &FBVClock::operator+=(FBVClock &c){
  ClockSystem &CS = sys[cid];
  if(c.idx < int(CS.clocks[id].size()) && CS.clocks[id][c.idx]){
    // Already added
    return *this;
  }
  c.update();
  ++CS.time;
  CS.update_ts[c.id] = CS.time;
  std::vector<bool> &clk = CS.clocks[id];
  add_clock(clk,CS.clocks[c.id]);
  CS.change_ts[id] = CS.time;
  return *this;
}

void FBVClock::update(ClockSystemID cid, int id){
  assert(0 <= cid && cid < int(sys.size()));
  ClockSystem &CS = sys[cid];
  assert(0 <= id && id < int(CS.clocks.size()));
  std::vector<bool> &clk = CS.clocks[id];
  assert(clk.size() <= CS.idx_to_id.size());
  const int upd_ts = CS.update_ts[id];
  if(upd_ts == CS.time) return;

  for(unsigned i = 0; i < clk.size(); ++i){
    int i_id = CS.idx_to_id[i];
    if(i_id != id && CS.clocks[id][i] && upd_ts < CS.change_ts[i_id]){
      update(cid,i_id);
      add_clock(clk,CS.clocks[i_id]);
    }
  }

  CS.update_ts[id] = CS.time;
}

void FBVClock::add_clock(std::vector<bool> &dst, std::vector<bool> &src){
  if(dst.size() < src.size()) dst.resize(src.size(),false);
  for(unsigned i = 0; i < src.size(); ++i){
    dst[i] = dst[i] || src[i];
  }
}

std::string FBVClock::to_string() const{
  if(id < 0){
    return "";
  }
  update();
  const std::vector<bool> &clk = sys[cid].clocks[id];
  std::string s(sys[cid].idx_to_id.size(),'0');
  for(unsigned i = 0; i < clk.size(); ++i){
    if(clk[i]) s[i] = '1';
  }
  return s;
}
