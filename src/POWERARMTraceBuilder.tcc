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

#include "Debug.h"
#include "POWERARMTraceBuilder.h"

#include <cassert>

namespace PATB_impl{

  template<MemoryModel MemMod,CB_T CB,class Event>
  TB<MemMod,CB,Event>::TB(const Configuration &conf)
    : POWERARMTraceBuilder(conf),
      sched_count(0), fetch_count(0),
      uncommitted_nonblocking_count(0),
      is_aborted(false),
      next_clock_index(0),
      TRec(&cpids)
  {
    fch.emplace_back();
    threads.emplace_back();
    cpids.emplace_back();
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  TB<MemMod,CB,Event>::~TB(){
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  Trace *TB<MemMod,CB,Event>::get_trace() const{
    std::vector<Error*> errs;
    for(unsigned i = 0; i < errors.size(); ++i){
      if(error_is_real(*errors[i])){
        errs.push_back(errors[i]->clone());
      }
    }
    std::vector<PATrace::Evt> evts(prefix.size());
    for(unsigned i = 0; i < prefix.size(); ++i){
      evts[i].iid = prefix[i];
      evts[i].param.choices = get_evt(prefix[i]).cur_param.choices;
      // Ignore the rest of the parameter (relations etc.)
    }
    return new PATrace(evts, cpids, conf, errs, TRec.to_string(2),
                       !sleepset_is_empty());
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  int TB<MemMod,CB,Event>::spawn(int proc){
    assert(0 <= proc && proc < int(cpids.size()));
    fch.emplace_back();
    threads.emplace_back();
    cpids.push_back(CPS.spawn(cpids[proc]));
    return int(fch.size())-1;
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::abort(){
    is_aborted = true;
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  IID<int> TB<MemMod,CB,Event>::fetch(int proc,
                                      int load_count,
                                      int store_count,
                                      const VecSet<int> &addr_deps,
                                      const VecSet<int> &data_deps){
    assert(0 <= proc && proc < int(threads.size()));
    assert(threads.size() <= fch.size());
    Thread &T = threads[proc];
    Event *evt;
    if(T.fch_count < int(fch[proc].size())){
      evt = &fch[proc][T.fch_count];
      switch(evt->filled_status){
      case Event::STATUS_EMPTY:
        *evt = Event(load_count,store_count,addr_deps,data_deps,
                     threads[proc].last_fetched_eieio_idx,
                     threads[proc].last_fetched_lwsync_idx,
                     threads[proc].last_fetched_sync_idx,
                     next_clock_index++);
        break;
      case Event::STATUS_PARAMETER:
        {
          Param P = std::move(evt->cur_param);
          Branch B = std::move(evt->cur_branch);
          VecSet<Branch> new_branches(std::move(evt->new_branches));
          assert(evt->new_params.empty());
          int branch_start = evt->branch_start;
          int branch_end = evt->branch_end;
          PAEvent::RecalcParams recalc_params = evt->recalc_params;
          IID<int> recalc_params_load_src = evt->recalc_params_load_src;
          *evt = Event(load_count,store_count,addr_deps,data_deps,
                       threads[proc].last_fetched_eieio_idx,
                       threads[proc].last_fetched_lwsync_idx,
                       threads[proc].last_fetched_sync_idx,
                       next_clock_index++);
          evt->cur_param = std::move(P);
          evt->cur_branch = std::move(B);
          evt->new_branches = std::move(new_branches);
          evt->branch_start = branch_start;
          evt->branch_end = branch_end;
          evt->recalc_params = recalc_params;
          evt->recalc_params_load_src = recalc_params_load_src;
          break;
        }
      case Event::STATUS_COMPLETE:
        assert(evt->load_count == load_count);
        assert(evt->has_load == (0 < load_count));
        assert(evt->has_store == (0 < store_count));
        evt->unknown_addr_count = load_count + store_count;
        evt->unknown_data_count = store_count;
        assert(evt->addr_deps == addr_deps);
        assert(evt->data_deps == data_deps);
        assert(evt->eieio_idx_before == threads[proc].last_fetched_eieio_idx);
        assert(evt->lwsync_idx_before == threads[proc].last_fetched_lwsync_idx);
        assert(evt->sync_idx_before == threads[proc].last_fetched_sync_idx);
        evt->eieio_idx_after = -1;
        evt->lwsync_idx_after = -1;
        evt->sync_idx_after = -1;
        evt->accesses.clear();
        evt->accesses.reserve(load_count+store_count);
        evt->accesses.resize(load_count,Access(Access::LOAD));
        evt->accesses.resize(load_count+store_count,Access(Access::STORE));
        evt->baccesses.clear();
        evt->access_tgts.clear();
        evt->clock_index = next_clock_index++;
        evt->cb_bwd.clear();
        evt->cb_bwd.set(evt->clock_index);
        evt->poloc_bwd_computed = false;
        assert(evt->recalc_params == PAEvent::RECALC_NO);
        break;
      }
    }else{
      assert(T.fch_count == int(fch[proc].size()));
      fch[proc].emplace_back(load_count,store_count,addr_deps,data_deps,
                             threads[proc].last_fetched_eieio_idx,
                             threads[proc].last_fetched_lwsync_idx,
                             threads[proc].last_fetched_sync_idx,
                             next_clock_index++);
      evt = &fch[proc].back();
    }
    ++fetch_count;
    ++uncommitted_nonblocking_count;
    ++T.fch_count;
    evt->iid = IID<int>(proc,T.fch_count);
    evt->ctrl_deps = T.ctrl_deps;
    evt->ctrl_isync_deps = T.ctrl_isync_deps;
    update_addr_known_prefix(proc);
    return evt->iid;
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::register_addr(const IID<int> &iid, Accid a, const MRef &addr){
    Event &evt = get_evt(iid);
    assert(0 <= a && a < Accid(evt.accesses.size()));
    assert(!evt.accesses[a].addr_known);
    evt.accesses[a].addr = addr;
    evt.accesses[a].addr_known = true;
    --evt.unknown_addr_count;
    /* Add the corresponding byte accesses */
    for(int i = 0; i < addr.size; ++i){
      void *p = (void*)&((char*)addr.ref)[i];
      evt.access_tgts.insert(p);
      if(evt.accesses[a].type == Access::LOAD){
        /* Check if there already is a load to that byte. */
        bool do_insert = true;
        for(int i = 0; do_insert && i < evt.load_count; ++i){
          if(i == a) continue;
          if(evt.accesses[i].addr_known &&
             evt.accesses[i].addr.includes(p)){
            do_insert = false;
          }
        }
        if(do_insert){
          evt.baccesses.emplace_back(ByteAccess::LOAD,p);
        }
      }else{
        assert(evt.accesses[a].type == Access::STORE);
        char d;
        if(evt.accesses[a].data_known){
          d = ((char*)evt.accesses[a].data.get_block())[i];
        }
        int i = int(evt.accesses.size()-1);
        while(evt.load_count <= i &&
              !(i != a &&
                evt.accesses[i].addr_known &&
                evt.accesses[i].addr.includes(p))) --i;
        if(i < evt.load_count){ // No other store to this byte (yet)
          /* Insert the store */
          if(evt.accesses[a].data_known){
            evt.baccesses.emplace_back(p,d);
          }else{
            evt.baccesses.emplace_back(ByteAccess::STORE,p);
          }
        }else if(i < a){ // This store overwrites the previous
          if(evt.accesses[a].data_known){
            /* Find the byte access, and update the data */
            int j = 0;
            while(!(evt.baccesses[j].type == ByteAccess::STORE &&
                    evt.baccesses[j].addr == p)) ++j;
            evt.baccesses[j].data = d;
            evt.baccesses[j].data_known = true;
          }// else, data will be stored when it is registered
        } // else, this store is overwritten by the later store (do nothing)
      }
    }
    update_addr_known_prefix(iid.get_pid());
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::register_data(const IID<int> &iid, Staccid a, const MBlock &data){
    Event &evt = get_evt(iid);
    assert(evt.load_count <= a && a < int(evt.accesses.size()));
    assert(evt.accesses[a].type == Access::STORE);
    assert(!evt.accesses[a].data_known);
    evt.accesses[a].data = data;
    evt.accesses[a].data_known = true;
    --evt.unknown_data_count;

    /* Possibly update data in byte accesses */
    if(!evt.accesses[a].addr_known) return; // Byte accesses do not yet exist.
    for(int i = 0; i < evt.accesses[a].addr.size; ++i){
      void *p = (void*)&((char*)evt.accesses[a].addr.ref)[i];
      bool data_written = false;
      /* Search for other stores to the same byte */
      for(int j = int(evt.accesses.size())-1; a < j; --j){
        if(evt.accesses[j].addr_known &&
           evt.accesses[j].addr.includes(p)){
          data_written = true; // Access j overwrites this store (Do nothing)
          break;
        }
      }
      /* Find and update the byte access */
      for(unsigned j = 0; !data_written && j < evt.baccesses.size(); ++j){
        if(evt.baccesses[j].type == ByteAccess::STORE &&
           evt.baccesses[j].addr == p){
          evt.baccesses[j].data_known = true;
          evt.baccesses[j].data = ((char*)data.get_block())[i];
          data_written = true;
        }
      }
      assert(data_written);
    }
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::register_load_requirement(const IID<int> &iid, Ldaccid a, ldreqfun_t *f){
    Event &evt = get_evt(iid);
    assert(0 <= a && a < evt.load_count);
    assert(!evt.accesses[a].load_req);
    if(!evt.has_load_req()){
      /* The event is blocking, but was previously counted as nonblocking. */
      --uncommitted_nonblocking_count;
    }
    evt.accesses[a].load_req = f;
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::fetch_ctrl(int proc, const VecSet<int> &deps){
    assert(0 <= proc && proc < int(threads.size()));
    threads[proc].ctrl_deps.insert(deps);
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::fetch_fence(int proc, FenceType type){
    const int idx = threads[proc].fch_count;
    switch(type){
    case ISYNC:
      threads[proc].ctrl_isync_deps.insert(threads[proc].ctrl_deps);
      threads[proc].ctrl_deps.clear();
      break;
    case EIEIO:
      for(int i = std::max(0,threads[proc].last_fetched_eieio_idx); i < idx; ++i){
        fch[proc][i].eieio_idx_after = idx;
      }
      threads[proc].last_fetched_eieio_idx = idx;
      break;
    case LWSYNC:
      for(int i = std::max(0,threads[proc].last_fetched_lwsync_idx); i < idx; ++i){
        fch[proc][i].lwsync_idx_after = idx;
      }
      threads[proc].last_fetched_lwsync_idx = idx;
      break;
    case SYNC:
      for(int i = std::max(0,threads[proc].last_fetched_sync_idx); i < idx; ++i){
        fch[proc][i].sync_idx_after = idx;
      }
      threads[proc].last_fetched_sync_idx = idx;
      break;
    default:
      throw std::logic_error("POWERARMTraceBuilder::fetch_fence: Unknown type of fence.");
    }
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  bool TB<MemMod,CB,Event>::committable(Event &evt){
    const int proc = evt.iid.get_pid();
    if(evt.committed) return false;
    /* Check dependencies */
    if(!evt.all_addr_and_data_known()) return false;
#ifndef NDEBUG
    for(int i : evt.addr_deps){
      assert(fch[proc][i-1].committed);
    }
    for(int i : evt.data_deps){
      assert(fch[proc][i-1].committed);
    }
    for(const Access &A : evt.accesses){
      assert(A.satisfied());
    }
    for(const ByteAccess &A : evt.baccesses){
      assert(A.satisfied());
    }
#endif
    if(CB == CB_ARM || CB == CB_POWER){
      /* Check addr;po */
      if(threads[proc].addr_known_prefix < evt.iid.get_index()) return false;
    }
    if(CB == CB_ARM || CB == CB_POWER){
      /* Check sync and lwsync */
      if(threads[proc].committed_prefix < evt.sync_idx_before) return false;
      if(threads[proc].committed_prefix < evt.lwsync_idx_before) return false;
    }
    if(CB == CB_POWER){
      /* Check poloc */
      compute_poloc_bwd(evt);
      for(const IID<int> &iid : evt.rel.poloc.bwd){
        if(!get_evt(iid).committed) return false;
      }
    }
    return true;
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::update_committed_prefix(int proc){
    while(threads[proc].committed_prefix < threads[proc].fch_count &&
          fch[proc][threads[proc].committed_prefix].committed){
      ++threads[proc].committed_prefix;
    }
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::update_addr_known_prefix(int proc){
    while(threads[proc].addr_known_prefix < threads[proc].fch_count &&
          fch[proc][threads[proc].addr_known_prefix].unknown_addr_count == 0){
      ++threads[proc].addr_known_prefix;
    }
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  bool TB<MemMod,CB,Event>::schedule(IID<int> *iid, std::vector<MBlock> *values){
    if(is_aborted) return false;
    /* Schedule */
    if(sched_count < int(prefix.size())){
      /* The schedule is decided already by prefix */
      const int proc = prefix[sched_count].get_pid();
      Event &evt = get_evt(prefix[sched_count]);
      assert(committable(evt));
      if(!commit(evt,values,true)){
        /* The branch failed: The fixed load source is not allowed. */
        assert((prefix[evt.branch_end] == evt.iid) ||
               (prefix[evt.branch_end-1] == evt.iid));
        return false;
      }
      *iid = prefix[sched_count];
      update_committed_prefix(proc);
      ++sched_count;
      return true;
    }else{
      /* Find something new to schedule */
      for(unsigned proc = 0; proc < threads.size(); ++proc){
        const int fch_count = threads[proc].fch_count;
        for(int idx = threads[proc].committed_prefix; idx < fch_count; ++idx){
          if(committable(fch[proc][idx])){
            if(commit(fch[proc][idx],values,false)){
              *iid = IID<int>(proc,idx+1);
              while(threads[proc].committed_prefix < threads[proc].fch_count &&
                    fch[proc][threads[proc].committed_prefix].committed){
                ++threads[proc].committed_prefix;
              }
              prefix.push_back(*iid);
              ++sched_count;
              return true;
            }else{
              // The event is blocked. Do nothing, instead execute the
              // next committable event.
            }
          }
        }
      }
    }
    return false;
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  bool TB<MemMod,CB,Event>::commit(Event &evt, std::vector<MBlock> *values, bool replay){
    if(!replay || evt.recalc_params != PAEvent::RECALC_NO){
      populate_parameters(evt);
      if(evt.recalc_params == PAEvent::RECALC_LOAD){
        /* Filter parameters to remove all that do not read from the
         * store event indicated by evt.recalc_params_load_src.
         */
        std::vector<Param> ps = evt.new_params;
        evt.new_params.clear();
        for(const Param &P : ps){
          if(P.rel.rf.bwd.count(evt.recalc_params_load_src)){
            evt.new_params.push_back(P);
          }
        }
      }else if(evt.recalc_params == PAEvent::RECALC_PARAM){
        /* Find the parameter corresponding to cur_param */
        bool found = false;
        for(unsigned i = 0; i < evt.new_params.size(); ++i){
          if(evt.new_params[i] == evt.cur_param){
            evt.new_params[0] = evt.new_params[i];
            evt.new_params.resize(1); // Remove all other parameters
            found = true;
            break;
          }
        }
        if(!found){
          evt.new_params.clear(); // No available parameter
        }
      }
      /* Is there any allowed parameter? */
      if(evt.new_params.empty()){
        detect_rw_conflicts(evt);
        return false;
      }
      /* Choose a parameter */
      evt.cur_param = evt.new_params.back();
      evt.new_params.pop_back();
      evt.recalc_params = PAEvent::RECALC_NO;
    }

    /* Setup relations */
    setup_relations_cur_param(evt);

    /* Assign read values */
    get_cur_load_values(evt,values);
    /* Update mem */
    assert(evt.baccesses.size() == evt.cur_param.choices.size());
    for(unsigned i = 0; i < evt.baccesses.size(); ++i){
      const ByteAccess &A = evt.baccesses[i];
      mem[A.addr].commit(evt,A,evt.cur_param.choices[i]);
    }

    /* Identify conflicts with earlier loads */
    detect_rw_conflicts(evt);

    evt.committed = true;
    if(TRec.is_active()){
      TRec.trace_commit(evt.iid,evt.cur_param,evt.accesses,evt.baccesses,*values);
    }
    if(!evt.has_load_req()){ // evt is nonblocking...
      --uncommitted_nonblocking_count; // ... but no longer uncommitted
    }
    return true;
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::setup_relations_cur_param(Event &evt){
    evt.rel = evt.cur_param.rel;
    return_relations(evt);
    /* hb */
    for(const IID<int> &b_iid : evt.cur_param.ER.EHB.after_I){
      for(const IID<int> &a_iid : evt.cur_param.ER.EHB.I){
        Event &a = get_evt(a_iid);
        Event &b = get_evt(b_iid);
        assert(a.committed && b.committed);
        a.rel.hb.fwd.insert(b_iid);
        b.rel.hb.bwd.insert(a_iid);
      }
    }
    for(const IID<int> &b_iid : evt.cur_param.ER.EHB.after_C){
      for(const IID<int> &a_iid : evt.cur_param.ER.EHB.C){
        Event &a = get_evt(a_iid);
        Event &b = get_evt(b_iid);
        assert(a.committed && b.committed);
        a.rel.hb.fwd.insert(b_iid);
        b.rel.hb.bwd.insert(a_iid);
      }
    }
    /* prop */
    for(auto pr : evt.cur_param.ER.prop){
      const IID<int> &a_iid = pr.first;
      for(const IID<int> &b_iid : pr.second){
        Event &a = get_evt(a_iid);
        Event &b = get_evt(b_iid);
        assert(a.committed && b.committed);
        a.rel.prop.fwd.insert(b_iid);
        b.rel.prop.bwd.insert(a_iid);
      }
    }
    /* cb */
    evt.cb_bwd = compute_cb(evt);
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  BVClock TB<MemMod,CB,Event>::compute_cb(const Event &evt, bool include_rf){
    BVClock cb;
    cb.set(evt.clock_index);
    const int proc = evt.iid.get_pid();
    for(int i : evt.addr_deps) cb += fch[proc][i-1].cb_bwd;
    for(int i : evt.data_deps) cb += fch[proc][i-1].cb_bwd;
    for(int i : evt.ctrl_deps) cb += fch[proc][i-1].cb_bwd;
    for(int i : evt.ctrl_isync_deps) cb += fch[proc][i-1].cb_bwd; // Capture the ctrl deps which are also ctrl+isync deps
    if(include_rf){
      for(const IID<int> &iid : evt.rel.rf.bwd){
        cb += get_evt(iid).cb_bwd;
      }
    }
    if(CB == CB_ARM || CB == CB_POWER){
      /* addr;po U lwsync U sync */
      const int snc = std::max(evt.lwsync_idx_before, evt.sync_idx_before);
      for(int i = evt.iid.get_index()-2; 0 <= i; --i){
        if(i < snc){ // before a lwsync or sync
          cb += fch[proc][i].cb_bwd;
          if(fch[proc][i].lwsync_idx_before == i ||
             fch[proc][i].sync_idx_before == i){
            /* All earlier events are already covered by fch[proc][i].cb_bwd */
            break;
          }
        }else{ // addr;po
          for(int j : fch[proc][i].addr_deps){
            cb += fch[proc][j-1].cb_bwd;
          }
        }
      }
    }
    if(CB == CB_POWER){
      /* po-loc */
      for(const IID<int> &iid : evt.rel.poloc.bwd){
        cb += get_evt(iid).cb_bwd;
      }
    }
    return cb;
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::return_relations(Event &evt){
    for(const IID<int> &iid : evt.rel.rf.fwd){
      get_evt(iid).rel.rf.bwd.insert(evt.iid);
    }
    for(const IID<int> &iid : evt.rel.rf.bwd){
      get_evt(iid).rel.rf.fwd.insert(evt.iid);
    }
    for(const IID<int> &iid : evt.rel.co.fwd){
      get_evt(iid).rel.co.bwd.insert(evt.iid);
    }
    for(const IID<int> &iid : evt.rel.co.bwd){
      get_evt(iid).rel.co.fwd.insert(evt.iid);
    }
    for(const IID<int> &iid : evt.rel.com.fwd){
      get_evt(iid).rel.com.bwd.insert(evt.iid);
    }
    for(const IID<int> &iid : evt.rel.com.bwd){
      get_evt(iid).rel.com.fwd.insert(evt.iid);
    }
    for(const IID<int> &iid : evt.rel.poloc.fwd){
      get_evt(iid).rel.poloc.bwd.insert(evt.iid);
    }
    for(const IID<int> &iid : evt.rel.poloc.bwd){
      get_evt(iid).rel.poloc.fwd.insert(evt.iid);
    }
    for(const IID<int> &iid : evt.rel.ii0.fwd){
      get_evt(iid).rel.ii0.bwd.insert(evt.iid);
    }
    for(const IID<int> &iid : evt.rel.ii0.bwd){
      get_evt(iid).rel.ii0.fwd.insert(evt.iid);
    }
    for(const IID<int> &iid : evt.rel.ci0.fwd){
      get_evt(iid).rel.ci0.bwd.insert(evt.iid);
    }
    for(const IID<int> &iid : evt.rel.ci0.bwd){
      get_evt(iid).rel.ci0.fwd.insert(evt.iid);
    }
    for(const IID<int> &iid : evt.rel.cc0.fwd){
      get_evt(iid).rel.cc0.bwd.insert(evt.iid);
    }
    for(const IID<int> &iid : evt.rel.cc0.bwd){
      get_evt(iid).rel.cc0.fwd.insert(evt.iid);
    }
    for(const IID<int> &iid : evt.rel.hb.fwd){
      get_evt(iid).rel.hb.bwd.insert(evt.iid);
    }
    for(const IID<int> &iid : evt.rel.hb.bwd){
      get_evt(iid).rel.hb.fwd.insert(evt.iid);
    }
    for(const IID<int> &iid : evt.rel.prop.fwd){
      get_evt(iid).rel.prop.bwd.insert(evt.iid);
    }
    for(const IID<int> &iid : evt.rel.prop.bwd){
      get_evt(iid).rel.prop.fwd.insert(evt.iid);
    }
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void Mem<MemMod,CB,Event>::commit(Event &evt, const ByteAccess &A, const Param::Choice &C){
    if(A.type == ByteAccess::STORE){
      coherence.push_back(evt.iid);
      loads.emplace_back();
      for(int i = int(coherence.size())-1; C.co_pos < i; --i){
        coherence[i] = coherence[i-1];
        loads[i+1] = loads[i];
      }
      coherence[C.co_pos] = evt.iid;
      loads[C.co_pos+1].clear();
    }else{ // A.type == ByteAccess::LOAD
      assert(loads.size() == coherence.size() + 1);
      if(C.src.is_null()){
        loads[0].insert(evt.iid);
      }else{
        int l = 1;
        while(coherence[l-1] != C.src) ++l; // Find the source store's position
        loads[l].insert(evt.iid);
      }
    }
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::get_cur_load_values(Event &evt, std::vector<MBlock> *values){
    get_load_values(evt,values,evt.cur_param);
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::get_load_values(Event &evt, std::vector<MBlock> *values, const Param &B){
    values->clear();
    for(int i = 0; i < evt.load_count; ++i){
      values->push_back(MBlock(evt.accesses[i].addr,evt.accesses[i].addr.size));
      char *block = (char*)values->back().get_block();
      for(int j = 0; j < evt.accesses[i].addr.size; ++j){
        void *p = (void*)&((char*)evt.accesses[i].addr.ref)[j];
        int k;
        /* Find the byte load access of p */
        for(k = 0; !(evt.baccesses[k].addr == p && evt.baccesses[k].type == ByteAccess::LOAD); ++k) ;
        if(B.choices[k].src.is_null()){ // Initial value
          block[j] = *(char*)p; // Get the value directly from memory
        }else{ // Get value from source store event
          Event &wevt = get_evt(B.choices[k].src);
          for(const ByteAccess &A : wevt.baccesses){
            if(A.addr == p &&
               A.type == ByteAccess::STORE){
              block[j] = A.data;
              break;
            }
          }
        }
      }
    }
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::detect_rw_conflicts(Event &w_evt){
    const int proc = w_evt.iid.get_pid();
    const int idx = w_evt.iid.get_index();
    std::function<bool(Event&)> is_mutex_lock =
      [](Event &e){
      return e.baccesses.size() == 2 &&
      e.accesses.size() == 2 &&
      e.load_count == 1 &&
      e.baccesses[0].addr == e.baccesses[1].addr &&
      e.accesses[0].load_req;
    };
    VecSet<IID<int> > conflicts;
    bool w_is_mutex_lock = is_mutex_lock(w_evt);
    BVClock cb_wo_rf; // cb relation excluding incoming rf-edges to w_evt
    if(w_is_mutex_lock){
      cb_wo_rf = compute_cb(w_evt,false);
    }
    for(const ByteAccess &A : w_evt.baccesses){
      if(A.type == ByteAccess::LOAD) continue;
      Mem<MemMod,CB,Event> &m = mem[A.addr];
      for(unsigned i = 0; i < m.loads.size(); ++i){
        for(const IID<int> &riid : m.loads[i]){
          Event &r_evt = get_evt(riid);
          if(riid.get_pid() == proc && riid.get_index() <= idx) continue; // w_evt cannot be the source
          if(w_is_mutex_lock && is_mutex_lock(r_evt)){
            /* Special treatment for lock -> lock conflicts. */
            if(0 <= r_evt.branch_end && prefix[r_evt.branch_end] != r_evt.iid) continue;
            if(0 <= r_evt.branch_end) continue; // NOTICE: Is this check correct?
            if(cb_wo_rf[r_evt.clock_index]) continue;
            conflicts.insert(riid);
          }else{
            if(w_evt.cb_bwd[r_evt.clock_index]) continue;
            if(0 <= r_evt.branch_end && prefix[r_evt.branch_end] != r_evt.iid) continue;
            conflicts.insert(riid);
          }
        }
      }
    }
    VecSet<void*> w_stbytes;
    for(const ByteAccess &A : w_evt.baccesses){
      w_stbytes.insert(A.addr);
    }
    for(const IID<int> &riid : conflicts){
      Branch bnc;

      Event &r_evt = get_evt(riid);
      const bool lock_lock_race = w_is_mutex_lock && is_mutex_lock(r_evt);
      int bnc_start;
      if(0 <= r_evt.branch_start){
        bnc_start = r_evt.branch_start;
      }else{
        bnc_start = 0;
        while(prefix[bnc_start] != riid) ++bnc_start;
      }
      VecSet<IID<int> > removed;
      std::function<void(const Event&,Param&)> decrease_co_pos =
        [&removed,this](const Event &evt, Param &P){
        /* Decrease coherence positions corresponding to the number
         * of removed co-earlier stores. */
        for(unsigned b = 0; b < evt.baccesses.size(); ++b){
          const ByteAccess &B = evt.baccesses[b];
          if(B.type == ByteAccess::STORE){
            Mem<MemMod,CB,Event> &m = this->mem[B.addr];
            for(unsigned j = 0; m.coherence[j] != evt.iid; ++j){
              if(removed.count(m.coherence[j])){
                assert(P.choices[b].is_coherence_choice());
                --P.choices[b].co_pos;
              }
            }
          }
        }
      };
      for(int i = bnc_start; i < sched_count; ++i){
        Event &ievt = get_evt(prefix[i]);
        if((!lock_lock_race && w_evt.cb_bwd[ievt.clock_index]) ||
           (lock_lock_race && cb_wo_rf[ievt.clock_index])){
          Param P;
          P.choices = ievt.cur_param.choices;
          decrease_co_pos(ievt,P);
          bnc.branch.push_back({prefix[i],P});
        }else if(ievt.has_store){
          removed.insert(prefix[i]);
        }
      }
      if(lock_lock_race){ // Special treatment for lock->lock races
        Param P;
        bnc.branch.push_back({w_evt.iid,P});
        bnc.param_type = Branch::PARAM_STAR;
      }else{
        /* Add the store event w_evt.iid and the load event riid to the
         * end of the branch. Give them dummy parameters. They are going
         * to have their parameters recalculated anyway according to
         * PAEvent::RECALC_STAR and PAEvent::RECALC_LOAD respectively.
         */
        Param P;
        bnc.branch.push_back({w_evt.iid,P});
        bnc.branch.push_back({riid,P});
        bnc.param_type = Branch::PARAM_WR;
      }
      normalise_branch(bnc);
      assert(get_evt(prefix[bnc_start]).branch_start < 0 || get_evt(prefix[bnc_start]).cur_branch.branch.size());
      if(get_evt(prefix[bnc_start]).branch_start < 0 || get_evt(prefix[bnc_start]).cur_branch < bnc){
        assert(get_evt(prefix[bnc_start]).branch_start < 0 || get_evt(prefix[bnc_start]).branch_start == bnc_start);
        get_evt(prefix[bnc_start]).new_branches.insert(bnc);
      }
    }
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::normalise_branch(Branch &B){
    /* Use insertion sort to order all events in B, except for the
     * suffix of B which is fixed (the store and the load in case of
     * R->W, or the lock in case of lock->lock).
     *
     * The order according to which the events are sorted is cb, where
     * ties are broken by comparing IID.
     */
    std::function<bool(const Branch::PEvent&,const Branch::PEvent&)> lt = // Comparison function
      [this](const Branch::PEvent &A, const Branch::PEvent &B){
      const Event &e0 = this->get_evt(A.iid);
      const Event &e1 = this->get_evt(B.iid);
      if(e1.cb_bwd[e0.clock_index]) return true;
      assert(!e0.cb_bwd[e1.clock_index]); // In normalise_branch A and B should never be compared when B-cb->A.
      return A.iid < B.iid;
    };
    int last_event = -1;
    switch(B.param_type){
    case Branch::PARAM_WR:
      last_event = int(B.branch.size())-2;
      break;
    case Branch::PARAM_STAR:
      last_event = int(B.branch.size())-1;
      break;
    default:
      throw std::logic_error("POWERARMTraceBuilder::normalise_branch: Unknown type of branch.");
    }
    for(int i = 1; i < last_event; ++i){
      int j = i;
      while(0<j && !lt(B.branch[j-1],B.branch[j])){
        /* Switch B.branch[j-1] and B.branch[j]
         *
         * If B.branch[j-1] and B.branch[j] store to the same byte,
         * also nudge their coherence positions.
         */
        Event &e0 = get_evt(B.branch[j-1].iid);
        Event &e1 = get_evt(B.branch[j].iid);
        for(unsigned a = 0; a < e0.baccesses.size(); ++a){
          if(e0.baccesses[a].type != ByteAccess::STORE) continue;
          for(unsigned b = 0; b < e1.baccesses.size(); ++b){
            if(e1.baccesses[b].type != ByteAccess::STORE) continue;
            if(e0.baccesses[a].addr == e1.baccesses[b].addr){
              if(B.branch[j-1].param.choices[a].co_pos < B.branch[j].param.choices[b].co_pos){
                --B.branch[j].param.choices[b].co_pos;
              }else{
                ++B.branch[j-1].param.choices[a].co_pos;
              }
            }
          }
        }
        Branch::PEvent tmp = std::move(B.branch[j]);
        B.branch[j] = std::move(B.branch[j-1]);
        B.branch[j-1] = std::move(tmp);
        --j;
      }
    }
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::populate_parameters(Event &evt){
    const int proc = evt.iid.get_pid();
    const int idx = evt.iid.get_index();
    assert(evt.new_params.empty());
    std::vector<Param> prm, next_prm;
    prm.emplace_back(); // Empty parameter to start with
    Param &B0 = prm.back();
    fences_to_hb(evt,B0.rel.hb);
    for(int i : evt.addr_deps){
      B0.rel.ii0.bwd.insert(IID<int>(proc,i));
      B0.rel.cc0.bwd.insert(IID<int>(proc,i));
    }
    for(int i : evt.data_deps){
      B0.rel.ii0.bwd.insert(IID<int>(proc,i));
      B0.rel.cc0.bwd.insert(IID<int>(proc,i));
    }
    for(int i : evt.ctrl_deps){
      B0.rel.cc0.bwd.insert(IID<int>(proc,i));
    }
    for(int i : evt.ctrl_isync_deps){
      B0.rel.ci0.bwd.insert(IID<int>(proc,i));
    }
    for(int i = 0; i < idx-1; ++i){
      for(int j : fch[proc][i].addr_deps){
        if(fch[proc][j-1].committed){
          B0.rel.cc0.bwd.insert(IID<int>(proc,i)); // addr;po
        }
      }
    }
    for(int i = idx; i < threads[proc].fch_count; ++i){
      if(fch[proc][i].addr_deps.count(idx)){
        /* All later events are addr;po-after this event. */
        for(int j = i+1; j < threads[proc].fch_count; ++j){
          if(fch[proc][j].committed){
            B0.rel.cc0.fwd.insert(IID<int>(proc,j+1)); // addr;po
          }
        }
        break;
      }
    }
    for(unsigned bi = 0; bi < evt.baccesses.size(); ++bi){
      const ByteAccess &BA = evt.baccesses[bi];
      for(const Param &B : prm){
        if(BA.type == ByteAccess::STORE){
          populate_parameters_store(evt,bi,BA,B,next_prm);
        }else{ // BA.type == ByteAccess::LOAD
          populate_parameters_load(evt,bi,BA,B,next_prm);
        }
      }
      /* Let prm and next_prm switch roles */
      prm.swap(next_prm);
      next_prm.clear();
    }

    bool has_load_req = false;
    for(const Access &A : evt.accesses){
      if(A.load_req){
        has_load_req = true;
        break;
      }
    }

    /* Check the parameters against the axioms NO THIN AIR,
     * OBSERVATION and PROPAGATION. Inserts the allowed parameters
     * into new_params. */
    evt.new_params.reserve(prm.size());
    for(unsigned i = 0; i < prm.size(); ++i){
      Param B = prm[i];
      ppo_to_hb(evt,B.rel,B.ER.EHB);
      if(CB == CB_POWER){
        /* hb subseteq cb_POWER, so there can be no cycles. */
        assert(no_thin_air(evt,B.rel,B.ER.EHB));
      }else{
        if(!no_thin_air(evt,B.rel,B.ER.EHB)) continue;
      }
      calc_prop(evt,B.rel,B.ER);
      if(!propagation(evt,B.rel,B.ER)) continue;
      if(!observation(evt,B.rel,B.ER.EHB)) continue;
      bool ok_load_reqs = true;
      if(has_load_req){
        std::vector<MBlock> values;
        get_load_values(evt,&values,B);
        for(int i = 0; i < evt.load_count; ++i){
          if(evt.accesses[i].load_req && !(*evt.accesses[i].load_req)(values[i])){
            ok_load_reqs = false;
            break;
          }
        }
      }
      if(!ok_load_reqs) continue;
      evt.new_params.push_back(B);
    }
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::populate_parameters_store(Event &evt,int stidx,const ByteAccess &S,
                                                      const Param &B,std::vector<Param> &parameters){
    const int proc = evt.iid.get_pid();
    Mem<MemMod,CB,Event> &m = mem[S.addr];
    /* Compute the allowed coherence positions for the store access.
     *
     * This will be done by gradually narrowing a range of possible
     * positions between (and including) first and last.
     */
    int first = 0, last = int(m.coherence.size());
    Rel poloc = B.rel.poloc;
    Rel cc0 = B.rel.cc0;

    /**************************
     * AXIOM: SC PER LOCATION *
     **************************/
    {
      IID<int> poloc_before_iid, poloc_after_iid;
      int poloc_before_idx, poloc_after_idx;
      ByteAccess::Type poloc_before_tp, poloc_after_tp;
      find_poloc(evt,m,poloc_before_iid,poloc_before_idx,poloc_before_tp,
                 poloc_after_iid,poloc_after_idx,poloc_after_tp);
      if(!poloc_before_iid.is_null()){
        if(MemMod == POWER) cc0.bwd.insert(poloc_before_iid);
        poloc.bwd.insert(poloc_before_iid);
        if(poloc_before_tp == ByteAccess::STORE){
          first = poloc_before_idx+1;
        }else{
          first = poloc_before_idx;
        }
      }
      if(!poloc_after_iid.is_null()){
        if(MemMod == POWER) cc0.fwd.insert(poloc_after_iid);
        poloc.fwd.insert(poloc_after_iid);
        if(poloc_after_tp == ByteAccess::STORE){
          last = poloc_after_idx;
        }else{
          last = poloc_after_idx-1;
        }
      }

      clear_event_colour();
      com_poloc_bwd_search(B.rel.com.bwd,{});
      com_poloc_bwd_search(B.rel.poloc.bwd,{});
      /* Check which events we are already com-poloc-after */
      for(int i = m.coherence.size()-1; first <= i; --i){ // Check stores
        if(get_evt(m.coherence[i]).colour){
          first = i+1;
          break;
        }
      }
      for(int i = m.loads.size()-1; first < i; --i){ // Check loads
        for(const IID<int> &liid : m.loads[i]){
          if(get_evt(liid).colour){
            first = i;
          }
        }
      }
      /* Check which events we cannot be after */
      VecSet<IID<int> > compoloc_after(B.rel.com.fwd);
      compoloc_after.insert(B.rel.poloc.fwd);
      for(int i = first; i <= last; ++i){
        if((0 < i && // Check store
            com_poloc_bwd_search({m.coherence[i-1]},compoloc_after))
           ||
           (com_poloc_bwd_search(m.loads[i],compoloc_after))){
          last = i-1;
        }
      }
    }

    /* Add brances, update relations */
    parameters.reserve(parameters.size() + last - first + 1);
    for(int i = first; i <= last; ++i){
      parameters.push_back(B);
      Param &B2 = parameters.back();
      B2.choices.emplace_back(i);
      B2.rel.poloc = poloc;
      if(0 < i && m.coherence.size()){
        B2.rel.co.bwd.insert(m.coherence[i-1]);
        B2.rel.com.bwd.insert(m.coherence[i-1]);
      }
      if(i < int(m.coherence.size())){
        B2.rel.co.fwd.insert(m.coherence[i]);
        B2.rel.com.fwd.insert(m.coherence[i]);
      }
      B2.rel.com.bwd.insert(m.loads[i]); // fr
      /* Add detour (fwd) */
      for(int j = i; j < int(m.coherence.size()); ++j){
        if(m.coherence[j].get_pid() != proc){
          for(const IID<int> &liid : m.loads[j+1]){
            if(liid.get_pid() == proc){
              B2.rel.ci0.fwd.insert(liid); // detour
            }
          }
        }
      }
      B2.rel.cc0 = cc0;
    }
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::populate_parameters_load(Event &evt,int ldidx,const ByteAccess &L,
                                                     const Param &B,std::vector<Param> &parameters){
    const int proc = evt.iid.get_pid();
    Mem<MemMod,CB,Event> &m = mem[L.addr];

    /* Compute the allowed sources for the load access.
     *
     * This will be done by gradually narrowing a range of possible
     * sources between (and including) first and last. The indexes
     * first and last should be interpreted as indexes into
     * m.coherence when they are non-negative, and as indicating the
     * initial store when they take the value -1.
     */
    int first = -1, last = int(m.coherence.size())-1;
    Rel poloc = B.rel.poloc;
    Rel cc0 = B.rel.cc0;

    /**************************
     * AXIOM: SC PER LOCATION *
     **************************/
    {
      IID<int> poloc_before_iid, poloc_after_iid;
      int poloc_before_idx, poloc_after_idx;
      ByteAccess::Type poloc_before_tp, poloc_after_tp;
      find_poloc(evt,m,poloc_before_iid,poloc_before_idx,poloc_before_tp,
                 poloc_after_iid,poloc_after_idx,poloc_after_tp);
      if(!poloc_before_iid.is_null()){
        if(MemMod == POWER) cc0.bwd.insert(poloc_before_iid);
        poloc.bwd.insert(poloc_before_iid);
        if(poloc_before_tp == ByteAccess::STORE){
          first = poloc_before_idx;
        }else{
          first = poloc_before_idx-1;
        }
      }
      if(!poloc_after_iid.is_null()){
        if(MemMod == POWER) cc0.fwd.insert(poloc_after_iid);
        poloc.fwd.insert(poloc_after_iid);
        last = poloc_after_idx-1;
      }

      clear_event_colour();
      com_poloc_bwd_search(B.rel.com.bwd,{});
      com_poloc_bwd_search(B.rel.poloc.bwd,{});
      /* Check which events we are already com-poloc-after */
      for(int i = m.coherence.size()-1; first < i; --i){
        if(get_evt(m.coherence[i]).colour){
          first = i;
          break;
        }
      }
      /* Check which events we cannot be after */
      VecSet<IID<int> > compoloc_after(B.rel.com.fwd);
      compoloc_after.insert(B.rel.poloc.fwd);
      for(int i = std::max(0,first); i < int(m.coherence.size()); ++i){
        if(com_poloc_bwd_search({m.coherence[i]},compoloc_after)){
          /* Being com-poloc-after m.coherence[i] causes a cycle */
          last = i-1;
          break;
        }
      }
    }

    /* Add brances, update relations */
    parameters.reserve(parameters.size() + last - first + 1);
    for(int i = last; first <= i; --i){
      parameters.push_back(B);
      Param &B2 = parameters.back();
      if(i == -1){
        B2.choices.emplace_back(IID<int>()); // Initial store
      }else{
        B2.choices.emplace_back(m.coherence[i]);
        B2.rel.rf.bwd.insert(m.coherence[i]);
        B2.rel.com.bwd.insert(m.coherence[i]);
        if(m.coherence[i].get_pid() == proc){ // rfi
          B2.rel.ii0.bwd.insert(m.coherence[i]); // rfi in ii
        }else{ // rfe
          B2.rel.hb.bwd.insert(m.coherence[i]); // rfe in hb
          /* Add detour */
          for(int j = 0; j < i; ++j){
            if(m.coherence[j].get_pid() == proc){
              B2.rel.ci0.bwd.insert(m.coherence[j]); // detour
            }
          }
          /* Add rdw (bwd) */
          for(int j = 0; j <= i; ++j){
            for(const IID<int> &liid : m.loads[j]){
              if(liid.get_pid() == proc){
                B2.rel.ii0.bwd.insert(liid); // rdw
              }
            }
          }
          /* Add rdw (fwd) */
          for(int j = i+1; j < int(m.coherence.size()); ++j){
            if(m.coherence[j].get_pid() != proc){
              for(const IID<int> &liid : m.loads[j+1]){
                if(liid.get_pid() == proc){
                  B2.rel.ii0.fwd.insert(liid); // rdw
                }
              }
            }
          }
        }
      }
      if(i < int(m.coherence.size())-1){
        B2.rel.com.fwd.insert(m.coherence[i+1]);
      }
      B2.rel.poloc = poloc;
      B2.rel.cc0 = cc0;
    }
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::fences_to_hb(Event &evt, Rel &hb){
    const int proc = evt.iid.get_pid();
    const int fch_count = threads[proc].fch_count;
    int a = std::max(evt.lwsync_idx_before,evt.sync_idx_before);
    if(evt.has_store){
      a = std::max(a,evt.eieio_idx_before);
    }
    int b = evt.lwsync_idx_after;
    if(b == -1 || (evt.sync_idx_after != -1 && evt.sync_idx_after < b)){
      b = evt.sync_idx_after;
    }
    if(evt.has_store &&
       (b == -1 || (evt.eieio_idx_after != -1 && evt.eieio_idx_after < b))){
      b = evt.eieio_idx_after;
    }
    for(int i = 0; i < a; ++i){
      Event &evt2 = fch[proc][i];
      if(!evt2.committed) continue;
      if((i < evt.sync_idx_before) ||
         (i < evt.lwsync_idx_before && (evt.has_store || evt2.has_load)) ||
         (i < evt.eieio_idx_before && evt.has_store && evt2.has_store)){
        hb.bwd.insert(IID<int>(proc,i+1));
      }
    }
    if(b != -1){
      for(int i = b; i < fch_count; ++i){
        Event &evt2 = fch[proc][i];
        if(!evt2.committed) continue;
        if((evt.sync_idx_after <= i) ||
           (evt.lwsync_idx_after <= i && (evt2.has_store || evt.has_load)) ||
           (evt.eieio_idx_after <= i && evt.has_store && evt2.has_store)){
          hb.fwd.insert(IID<int>(proc,i+1));
        }
      }
    }
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::compute_poloc_bwd(Event &evt){
    if(evt.poloc_bwd_computed) return;
    const int proc = evt.iid.get_pid();
    assert(evt.iid.get_index() <= threads[proc].addr_known_prefix);
    VecSet<void*> A = evt.access_tgts;
    for(int i = evt.iid.get_index()-2; A.size() && 0 <= i; --i){
      int rem_count = A.erase(fch[proc][i].access_tgts);
      if(rem_count){
        evt.rel.poloc.bwd.insert(fch[proc][i].iid);
      }
    }
    evt.poloc_bwd_computed = true;
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::find_poloc(Event &evt, Mem<MemMod,CB,Event> &m,
                                       IID<int> &before_iid, int &before_pos,
                                       ByteAccess::Type &before_type,
                                       IID<int> &after_iid, int &after_pos,
                                       ByteAccess::Type &after_type){
    const int proc = evt.iid.get_pid();
    const int idx = evt.iid.get_index();
    int poloc_before = 0, poloc_after = 0;
    for(int i = 0; i < int(m.coherence.size()); ++i){
      if(m.coherence[i].get_pid() == proc){
        if(m.coherence[i].get_index() < idx){
          poloc_before = std::max(poloc_before,m.coherence[i].get_index());
          before_pos = i;
          before_type = ByteAccess::STORE;
        }else{
          if(poloc_after){
            poloc_after = std::min(poloc_after,m.coherence[i].get_index());
          }else{
            poloc_after = m.coherence[i].get_index();
          }
          after_pos = i;
          after_type = ByteAccess::STORE;
          break;
        }
      }
    }
    const int a = poloc_before ? before_pos+1 : 0;
    const int b = poloc_after ? after_pos+1 : int(m.loads.size());
    for(int i = a; i < b; ++i){
      for(const IID<int> &liid : m.loads[i]){
        if(liid.get_pid() == proc){
          if(liid.get_index() < idx){
            if(poloc_before < liid.get_index()){
              poloc_before = liid.get_index();
              before_pos = i;
              before_type = ByteAccess::LOAD;
            }
          }else{
            if(poloc_after == 0 || liid.get_index() < poloc_after){
              poloc_after = liid.get_index();
              after_pos = i;
              after_type = ByteAccess::LOAD;
            }
            break;
          }
        }
      }
    }
    if(poloc_before){
      before_iid = IID<int>(proc,poloc_before);
    }else{
      before_iid = IID<int>();
    }
    if(poloc_after){
      after_iid = IID<int>(proc,poloc_after);
    }else{
      after_iid = IID<int>();
    }
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  IID<CPid> TB<MemMod,CB,Event>::get_iid() const{
    if(sched_count == 0){
      return IID<CPid>(); // Null
    }
    return IID<CPid>(cpids[prefix[sched_count-1].get_pid()],
                     prefix[sched_count-1].get_index());
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  bool TB<MemMod,CB,Event>::sleepset_is_empty() const{
    return uncommitted_nonblocking_count == 0;
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::clear_event_colour(){
    for(unsigned p = 0; p < threads.size(); ++p){
      for(int i = 0; i < threads[p].fch_count; ++i){
        fch[p][i].colour = 0;
      }
    }
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  bool TB<MemMod,CB,Event>::com_poloc_bwd_search(const VecSet<IID<int> > &inits,
                                                 const VecSet<IID<int> > &tgt){
    std::vector<IID<int> > stack(inits.begin(),inits.end());

    while(stack.size()){
      IID<int> iid = stack.back();
      stack.pop_back();
      Event &evt = get_evt(iid);
      if(evt.colour) continue;

      evt.colour = 1;
      stack.insert(stack.end(),
                   evt.rel.com.bwd.begin(),
                   evt.rel.com.bwd.end());
      stack.insert(stack.end(),
                   evt.rel.poloc.bwd.begin(),
                   evt.rel.poloc.bwd.end());
    }

    for(const IID<int> &iid : tgt){
      if(get_evt(iid).colour){
        return true;
      }
    }
    return false;
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::ppo_to_hb(Event &evt, Relations &rel, ExtraHB &EHB){
    EHB.I.clear(); EHB.C.clear();
    EHB.after_I.clear(); EHB.after_C.clear();
    const int VISITED_I_FROM_I = 1; // Mask: The search starting from I has visited I
    const int VISITED_I_FROM_C = 2; // Mask: The search starting from C has visited I
    const int VISITED_C_FROM_I = 4; // Mask: The search starting from I has visited C
    const int VISITED_C_FROM_C = 8; // Mask: The search starting from C has visited C
    const int VISITED_I_FROM_ALL = VISITED_I_FROM_I | VISITED_I_FROM_C;
    const int VISITED_C_FROM_ALL = VISITED_C_FROM_I | VISITED_C_FROM_C;
    clear_event_colour();
    std::vector<Event*> stack;
    stack.push_back(&evt);
    evt.colour = VISITED_I_FROM_ALL | VISITED_C_FROM_C;
    /* Search backwards */
    while(stack.size()){
      Event *e = stack.back();
      stack.pop_back();
      if(e != &evt && e->has_load){
        if(e->colour & VISITED_I_FROM_I){
          EHB.I.insert(e->iid);
          EHB.C.insert(e->iid);
        }else{
          EHB.C.insert(e->iid);
        }
      }
      VecSet<IID<int> > &ii0 = (e == &evt) ? rel.ii0.bwd : e->rel.ii0.bwd;
      VecSet<IID<int> > &ci0 = (e == &evt) ? rel.ci0.bwd : e->rel.ci0.bwd;
      VecSet<IID<int> > &cc0 = (e == &evt) ? rel.cc0.bwd : e->rel.cc0.bwd;
      assert(e->colour & VISITED_I_FROM_ALL);
      for(const IID<int> &iid : ii0){
        Event &e2 = get_evt(iid);
        const int IVISIT = e->colour & VISITED_I_FROM_ALL;
        const int new_e2_colour = e2.colour | IVISIT;
        if(e2.colour != new_e2_colour){
          e2.colour = new_e2_colour;
          stack.push_back(&e2);
        }
      }
      for(const IID<int> &iid : ci0){
        Event &e2 = get_evt(iid);
        const int IVISIT = e->colour & VISITED_I_FROM_ALL;
        const int CVISIT = IVISIT << 2;
        const int new_e2_colour = e2.colour | IVISIT | CVISIT;
        if(e2.colour != new_e2_colour){
          e2.colour = new_e2_colour;
          stack.push_back(&e2);
        }
      }
      if(e->colour & (VISITED_C_FROM_ALL)){
        for(const IID<int> &iid : cc0){
          Event &e2 = get_evt(iid);
          const int CVISIT = e->colour & VISITED_C_FROM_ALL;
          const int IVISIT = CVISIT >> 2;
          const int new_e2_colour = e2.colour | IVISIT | CVISIT;
          if(e2.colour != new_e2_colour){
            e2.colour = new_e2_colour;
            stack.push_back(&e2);
          }
        }
      }
    }
    /* Update rel.hb.bwd */
    if(evt.has_store){
      rel.hb.bwd.insert(EHB.C);
    }else{
      rel.hb.bwd.insert(EHB.I);
    }
    /* Search forwards */
    clear_event_colour();
    evt.colour = VISITED_I_FROM_I | VISITED_C_FROM_ALL;
    stack.push_back(&evt);
    while(stack.size()){
      Event *e = stack.back();
      assert(e->colour & VISITED_C_FROM_ALL);
      stack.pop_back();
      if(e != &evt){ // Update hb
        if(e->has_store){
          if(e->colour & VISITED_C_FROM_C){
            EHB.after_I.erase(e->iid); // To avoid intersecting
            EHB.after_C.insert(e->iid);
          }else{ // Only visited from I
            EHB.after_I.insert(e->iid);
          }
          if(evt.has_load){
            rel.hb.fwd.insert(e->iid);
          }
        }else{
          if(e->colour & VISITED_I_FROM_C){
            EHB.after_I.erase(e->iid); // To avoid intersecting
            EHB.after_C.insert(e->iid);
          }else if(e->colour & VISITED_I_FROM_I){
            EHB.after_I.insert(e->iid);
          }
          if(evt.has_load && (e->colour & VISITED_I_FROM_ALL)){
            rel.hb.fwd.insert(e->iid);
          }
        }
      }
      const VecSet<IID<int> > &ii0 = (e == &evt) ? rel.ii0.fwd : e->rel.ii0.fwd;
      const VecSet<IID<int> > &ci0 = (e == &evt) ? rel.ci0.fwd : e->rel.ci0.fwd;
      const VecSet<IID<int> > &cc0 = (e == &evt) ? rel.cc0.fwd : e->rel.cc0.fwd;
      if(e->colour & VISITED_I_FROM_ALL){
        for(const IID<int> &iid : ii0){
          Event &e2 = get_evt(iid);
          const int IVISIT = e->colour & VISITED_I_FROM_ALL;
          const int CVISIT = IVISIT << 2;
          const int new_e2_colour = e2.colour | IVISIT | CVISIT;
          if(e2.colour != new_e2_colour){
            e2.colour = new_e2_colour;
            stack.push_back(&e2);
          }
        }
      }
      for(const IID<int> &iid : ci0){
        Event &e2 = get_evt(iid);
        const int CVISIT = e->colour & VISITED_C_FROM_ALL;
        const int IVISIT = CVISIT >> 2;
        const int new_e2_colour = e2.colour | IVISIT | CVISIT;
        if(e2.colour != new_e2_colour){
          e2.colour = new_e2_colour;
          stack.push_back(&e2);
        }
      }
      for(const IID<int> &iid : cc0){
        Event &e2 = get_evt(iid);
        const int CVISIT = e->colour & VISITED_C_FROM_ALL;
        const int new_e2_colour = e2.colour | CVISIT;
        if(e2.colour != new_e2_colour){
          e2.colour = new_e2_colour;
          stack.push_back(&e2);
        }
      }
    }
  }

  /* Add to new_prop a set of events which includes all events which
   * may have gained a new forward prop edge by the addition of the
   * edges in rel and EHB.
   */
  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::calc_prop_inits(Event &new_evt, Relations &rel, ExtraHB &EHB,
                                            VecSet<Event*> &new_prop){
    /* Masks for colour */
    const int AFTER_I = 1;  // In EHB.after_I
    const int AFTER_C = 2;  // In EHB.after_C
    const int HB = 4;       // Reached by hb*
    clear_event_colour();
    const std::function<void(Event*)> follow_com_star =
      [this,&new_prop,&new_evt,&rel](Event *e){
      std::vector<Event*> com_stack;
      com_stack.push_back(e);
      while(com_stack.size()){
        Event *ce = com_stack.back();
        com_stack.pop_back();
        auto pr = new_prop.insert(ce);
        if(pr.second){ // New entry into new_prop
          const VecSet<IID<int> > &com = (ce == &new_evt) ? rel.com.bwd : ce->rel.com.bwd;
          for(const IID<int> &iid : com){
            com_stack.push_back(&this->get_evt(iid));
          }
        }
      }
    };
    follow_com_star(&new_evt);
    std::vector<Event*> stack;
    const std::function<void(const VecSet<IID<int> >&)> follow_hb =
      [this,&stack,HB](const VecSet<IID<int> > &S){
      for(const IID<int> &iid : S){
        Event &e = this->get_evt(iid);
        if(e.colour & HB) continue;
        e.colour |= HB;
        stack.push_back(&e);
      }
    };
    stack.push_back(&new_evt);
    new_evt.colour |= HB;
    for(const IID<int> &iid : EHB.after_I){
      Event &e = get_evt(iid);
      e.colour |= (AFTER_I | HB);
      stack.push_back(&e);
    }
    for(const IID<int> &iid : EHB.after_C){
      Event &e = get_evt(iid);
      e.colour |= (AFTER_I | HB);
      stack.push_back(&e);
    }
    while(stack.size()){
      Event *e = stack.back();
      stack.pop_back();
      /* Follow hb */
      if(e == &new_evt){
        follow_hb(rel.hb.bwd);
      }else{
        follow_hb(e->rel.hb.bwd);
      }
      if(e->colour & AFTER_C){
        follow_hb(EHB.C);
      }else if(e->colour & AFTER_I){
        follow_hb(EHB.I);
      }
      /* Follow com*;fences */
      const int proc = e->iid.get_pid();
      int fnc_load = std::max(e->lwsync_idx_before,e->sync_idx_before);
      int fnc_store = e->sync_idx_before;
      if(e->has_store){
        fnc_store = std::max(fnc_store,std::max(e->lwsync_idx_before,e->eieio_idx_before));
      }
      int fnc = std::max(fnc_load,fnc_store);
      for(int i = 0; i < fnc; ++i){
        Event &fe = fch[proc][i];
        if(!fe.committed && &fe != &new_evt) continue; // Not committed
        if((fe.has_load && i < fnc_load) || (fe.has_store && i < fnc_store)){
          /* fe reached by fences;hb* */
          /* Follow com* */
          follow_com_star(&fe);
        }
      }
    }
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::calc_prop(Event &new_evt, Relations &rel, ExtraRel &ER){
    /* Masks for colour */
    const int RFE_NE = 1;     // rfe-before new_evt
    const int COM_NE = 2;     // com-before new_evt
    const int HB_NE = 4;      // hb-before new_evt
    const int IN_I = 8;       // In ER.EHB.I
    const int IN_C = 16;      // In ER.EHB.C
    const int RFE = 32;       // Reached by rfe?
    const int COM = 64;       // Reached by com*
    const int L_HB = 128;     // Reached by rfe?;fences;hb*
    const int R_HB = 256;     // Reached by com*;fences;hb*
    const int R_HB_SNC = 512; // Reached by com*;prop-base*;ffence;hb*
    ExtraHB &_EHB = ER.EHB;
    VecSet<Event*> inits;
    calc_prop_inits(new_evt,rel,_EHB,inits);
    for(Event *init_e : inits){
      clear_event_colour();
      for(const IID<int> &iid : rel.rf.bwd){
        if(iid.get_pid() == new_evt.iid.get_pid()) continue; // Not rfe
        get_evt(iid).colour |= RFE_NE;
      }
      for(const IID<int> &iid : rel.com.bwd) get_evt(iid).colour |= COM_NE;
      for(const IID<int> &iid : rel.hb.bwd) get_evt(iid).colour |= HB_NE;
      for(const IID<int> &iid : _EHB.I) get_evt(iid).colour |= IN_I;
      for(const IID<int> &iid : _EHB.C) get_evt(iid).colour |= IN_C;
      init_e->colour |= (RFE | COM);
      std::vector<Event*> stack;
      /* Mark all in rfe */
      const VecSet<IID<int> > &rf = (init_e == &new_evt) ? rel.rf.fwd : init_e->rel.rf.fwd;
      for(const IID<int> &iid : rf){
        if(iid.get_pid() == init_e->iid.get_pid()) continue; // Not rfe
        Event &rfe_e = get_evt(iid);
        rfe_e.colour |= RFE;
      }
      if(init_e->colour & RFE_NE) new_evt.colour |= RFE;
      /* Insert all in com* */
      {
        std::vector<Event*> com_stack;
        com_stack.push_back(init_e);
        while(com_stack.size()){
          Event *e = com_stack.back();
          com_stack.pop_back();
          stack.push_back(e);
          const VecSet<IID<int> > &com = (e == &new_evt) ? rel.com.fwd : e->rel.com.fwd;
          for(const IID<int> &iid : com){
            Event &ce = get_evt(iid);
            if(ce.colour & COM) continue; // Already added
            ce.colour |= COM;
            stack.push_back(&ce);
            com_stack.push_back(&ce);
          }
          if(e->colour & COM_NE){
            if(!(new_evt.colour & COM)){
              new_evt.colour |= COM;
              stack.push_back(&new_evt);
              com_stack.push_back(&new_evt);
            }
          }
        }
      }
      const std::function<void(Event*,const VecSet<IID<int> >&)> follow_hb =
        [this,&stack,&new_evt,L_HB,R_HB,R_HB_SNC,init_e,&rel,&ER]
        (Event *e, const VecSet<IID<int> > &hb_fwd){
        for(const IID<int> &iid : hb_fwd){
          Event &e2 = this->get_evt(iid);
          if(!e2.committed && &e2 != &new_evt) continue; // Not committed
          int new_colour = e2.colour | (e->colour & (L_HB | R_HB | R_HB_SNC));
          if(new_colour != e2.colour){
            e2.colour = new_colour;
            stack.push_back(&e2);
          }
          if((e2.has_store && init_e->has_store && (e2.colour & L_HB)) ||
             (e2.colour & R_HB_SNC)){
            /* init_e -prop-> e2 */
            VecSet<IID<int> > &pfwd = (init_e == &new_evt) ?
            rel.prop.fwd : init_e->rel.prop.fwd;
            if(!pfwd.count(e2.iid)){
              /* The prop edge is new */
              if(&e2 == &new_evt){
                rel.prop.bwd.insert(init_e->iid);
              }
              if(init_e == &new_evt){
                rel.prop.fwd.insert(e2.iid);
              }else if(&e2 != &new_evt){
                ER.prop[init_e->iid].insert(e2.iid);
              }
            }
          }
        }
      };
      /* Follow fences and hb */
      while(stack.size()){
        Event *e = stack.back();
        stack.pop_back();
        /* Follow fences */
        const int proc = e->iid.get_pid();
        const int fch_count = threads[proc].fch_count;
        const int eieio = (e->eieio_idx_after == -1) ? fch_count : e->eieio_idx_after;
        const int lwsync = (e->lwsync_idx_after == -1) ? fch_count : e->lwsync_idx_after;
        const int sync = (e->sync_idx_after == -1) ? fch_count : e->sync_idx_after;
        const int fnc_load = e->has_load ? std::min(sync,lwsync) : sync;
        const int fnc_store = e->has_store ?
          std::min(std::min(sync,lwsync),eieio) :
          std::min(sync,lwsync);
        int fnc = std::min(fnc_load,fnc_store);
        for(int i = fnc; i < fch_count; ++i){
          Event &fe = fch[proc][i];
          if(!fe.committed && &fe != &new_evt) continue; // Not committed
          if((fe.has_load && fnc_load <= i) ||
             (fe.has_store && fnc_store <= i)){
            /* (e,fe) in fences */
            int new_colour = fe.colour;
            if(e->colour & RFE) new_colour |= L_HB;
            if(e->colour & COM) new_colour |= R_HB;
            new_colour |= (e->colour & (L_HB | R_HB | R_HB_SNC));
            if(sync <= i) new_colour |= R_HB_SNC;
            if(new_colour != fe.colour){
              fe.colour = new_colour;
              stack.push_back(&fe);
            }
          }
        }
        /* Follow hb */
        if(e == &new_evt){
          follow_hb(e,rel.hb.fwd);
        }else{
          follow_hb(e,e->rel.hb.fwd);
        }
        if(e->colour & IN_I){
          follow_hb(e,_EHB.after_I);
          follow_hb(e,_EHB.after_C);
        }else if(e->colour & IN_C){
          follow_hb(e,_EHB.after_C);
        }
        if(e->colour & HB_NE){
          follow_hb(e,{new_evt.iid});
        }
      }
    }
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  bool TB<MemMod,CB,Event>::no_thin_air(Event &evt, Relations &rel, ExtraHB &EHB){
    const int VISITED = 1;   // Mask for colour: event has been visited
    const int DONE = 2;      // Mask for colour: event has been completed
    const int EVTFWD = 4;    // Mask for colour: event is in rel.hb.fwd
    const int AFTER_I = 8;   // Mask for colour: event is in EHB.after_I
    const int AFTER_C = 16;  // Mask for colour: event is in EHB.after_C
    clear_event_colour();
    for(const IID<int> &iid : rel.hb.fwd){
      get_evt(iid).colour = EVTFWD;
    }
    for(const IID<int> &iid : EHB.after_I){
      get_evt(iid).colour |= AFTER_I;
    }
    for(const IID<int> &iid : EHB.after_C){
      get_evt(iid).colour |= AFTER_C;
    }
#ifndef NDEBUG
    int done_count = 0;
#endif
    std::vector<Event*> stack;
    const std::function<bool(const IID<int>&)> enter =
      [this,DONE,VISITED,&stack,&evt](const IID<int> &iid){
      Event &e2 = this->get_evt(iid);
      if(!e2.committed && &e2 != &evt) return true; // Don't insert
      if(!(e2.colour & DONE) && (e2.colour & VISITED)) return false; // Cycle
      stack.push_back(&e2);
      return true;
    };
    for(unsigned proc = 0; proc < threads.size(); ++proc){
      for(int idx = 0; idx < threads[proc].fch_count; ++idx){
        Event &e = fch[proc][idx];
        if(!e.committed && &e != &evt) continue;
        if(!(e.colour & DONE)){
          assert(!(e.colour & VISITED));
          stack.push_back(&e);
          while(stack.size()){
            Event *e = stack.back();
            if(e->colour & VISITED){
              if(!(e->colour & DONE)){
#ifndef NDEBUG
                ++done_count;
#endif
                e->colour |= DONE;
              }
              stack.pop_back();
            }else{
              VecSet<IID<int> > *bwd;
              if(e == &evt){
                bwd = &rel.hb.bwd;
              }else{
                bwd = &e->rel.hb.bwd;
              }
              for(const IID<int> &iid : *bwd){
                if(!enter(iid)) return false;
              }
              if(e->colour & AFTER_I){
                for(const IID<int> &iid : EHB.I){
                  if(!enter(iid)) return false;
                }
              }
              if(e->colour & AFTER_C){
                for(const IID<int> &iid : EHB.C){
                  if(!enter(iid)) return false;
                }
              }
              if(e->colour & EVTFWD){ // Can also reach evt backwards
                if(!enter(evt.iid)) return false;
              }
              e->colour |= VISITED;
            }
          }
        }else{
          assert(e.colour & VISITED);
        }
      }
    }
    assert(done_count == sched_count+1);
    return true;
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  bool TB<MemMod,CB,Event>::propagation(Event &new_evt, Relations &rel, ExtraRel &ER){
    const int CO_NE = 1;       // co-before new_evt
    const int PROP_NE = 2;     // prop-before new_evt
    const int IN_ER_PROP = 4;  // ER.prop contains a forward prop edge from this event
    const int CP_VISITED = 8;  // Has been visited
    const int CP_DONE = 16;    // Visited, and all successors are done
    clear_event_colour();
    std::vector<Event*> stack;
    const std::function<bool(Event*)> enter =
      [&stack](Event *e){
      if((e->colour & CP_VISITED) && !(e->colour & CP_DONE)){
        return false;
      }
      stack.push_back(e);
      return true;
    };
    enter(&new_evt);
    for(auto pr : ER.prop){
      Event &e = get_evt(pr.first);
      e.colour |= IN_ER_PROP;
      enter(&e);
    }
    for(const IID<int> &iid : rel.co.bwd) get_evt(iid).colour |= CO_NE;
    for(const IID<int> &iid : rel.prop.bwd) get_evt(iid).colour |= PROP_NE;
    while(stack.size()){
      Event *e = stack.back();
      if(e->colour & CP_VISITED){
        e->colour |= CP_DONE;
        stack.pop_back();
      }else{
        assert(!(e->colour & CP_DONE));
        e->colour |= CP_VISITED;
        /* Follow co */
        const VecSet<IID<int> > &co = (e == &new_evt) ? rel.co.fwd : e->rel.co.fwd;
        for(const IID<int> &iid : co){
          if(!enter(&get_evt(iid))) return false;
        }
        if(e->colour & CO_NE){
          if(!enter(&new_evt)) return false;
        }
        /* Follow prop */
        const VecSet<IID<int> > &prop = (e == &new_evt) ? rel.prop.fwd : e->rel.prop.fwd;
        for(const IID<int> &iid : prop){
          if(!enter(&get_evt(iid))) return false;
        }
        auto it = ER.prop.find(e->iid);
        if(it != ER.prop.end()){
          for(const IID<int> &iid : it->second){
            if(!enter(&get_evt(iid))) return false;
          }
        }
        if(e->colour & PROP_NE){
          if(!enter(&new_evt)) return false;
        }
      }
    }
    return true;
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  bool TB<MemMod,CB,Event>::observation(Event &new_evt, Relations &rel, ExtraHB &EHB){
    // TODO: Fix observation to make use of ExtraRel.

    /* Masks for colour */
    const int IN_I = 1;      // In EHB.I
    const int IN_C = 2;      // In EHB.C
    const int RFE_NE = 4;    // rfe-before new_evt
    const int COM_NE = 8;    // com-before new_evt
    const int HB_NE = 16;    // hb-before new_evt
    const int FRE = 32;      // Reached from fre
    const int RFE = 64;      // Reached from fre;rfe
    const int COM = 128;     // Reached from fre;com*
    const int HB = 256;      // Reached from fre;prop;hb*
    const int HB_SYNC = 512; // Reached from fre;com*;prop-base* but has not passed a sync
    VecSet<Event*> inits;
    observation_find_inits(new_evt,rel,EHB,inits);

    for(Event *R : inits){
      VecSet<Event*> fre;
      observation_get_fre(*R, new_evt, rel, fre);
      if(fre.empty()) continue;
      clear_event_colour();
      for(const IID<int> &iid : EHB.I) get_evt(iid).colour |= IN_I;
      for(const IID<int> &iid : EHB.C) get_evt(iid).colour |= IN_C;
      for(const IID<int> &iid : rel.rf.bwd){
        if(iid.get_pid() != new_evt.iid.get_pid()){
          get_evt(iid).colour |= RFE_NE;
        }
      }
      for(const IID<int> &iid : rel.com.bwd) get_evt(iid).colour |= COM_NE;
      for(const IID<int> &iid : rel.hb.bwd) get_evt(iid).colour |= HB_NE;
      for(Event *e : fre) e->colour |= (FRE | COM);
      std::vector<Event*> stack(fre.begin(),fre.end());
      const std::function<void(Event&,int)> stack_insert =
        [&stack](Event &evt, int mask){
        if((evt.colour | mask) != evt.colour){
          evt.colour |= mask;
          stack.push_back(&evt);
        }
      };
      while(stack.size()){
        Event *e = stack.back();
        stack.pop_back();
        if(e == R) return false;
        if(e->colour & FRE){
          /* Continue via rfe */
          const VecSet<IID<int> > &rf = (e == &new_evt) ? rel.rf.fwd : e->rel.rf.fwd;
          for(const IID<int> &iid : rf){
            if(iid.get_pid() == e->iid.get_pid()) continue; // Not rfe
            Event &rfe_e = get_evt(iid);
            stack_insert(rfe_e,RFE);
          }
          if(e->colour & RFE_NE){ // (e,new_evt) in rfe
            stack_insert(new_evt,RFE);
          }
        }
        if(e->colour & COM){
          /* Continue via com* */
          const VecSet<IID<int> > &com = (e == &new_evt) ? rel.com.fwd : e->rel.com.fwd;
          for(const IID<int> &iid : com){
            Event &com_e = get_evt(iid);
            if(!(com_e.colour & (COM | RFE))){
              com_e.colour |= COM;
              stack.push_back(&com_e);
            }
          }
          if(e->colour & COM_NE){ // (e,new_evt) in com
            if(!(new_evt.colour & (COM | RFE))){
              new_evt.colour |= COM;
              stack.push_back(&new_evt);
            }
          }
        }
        if(e->colour & (FRE | RFE | COM)){
          /* Continue via fences */
          const int proc = e->iid.get_pid();
          const int fch_count = threads[proc].fch_count;
          const int eieio_idx = (e->eieio_idx_after == -1) ? fch_count : e->eieio_idx_after;
          const int lwsync_idx = (e->lwsync_idx_after == -1) ? fch_count : e->lwsync_idx_after;
          const int sync_idx = (e->sync_idx_after == -1) ? fch_count : e->sync_idx_after;
          const int fnc_store =
            e->has_store ?
            std::min(eieio_idx,std::min(lwsync_idx,sync_idx)) :
            std::min(lwsync_idx,sync_idx);
          const int fnc_load =
            e->has_load ?
            std::min(lwsync_idx,sync_idx) :
            sync_idx;
          const int fnc = std::min(fnc_store,fnc_load);
          for(int i = fnc; i < fch_count; ++i){
            Event &fnc_e = fch[proc][i];
            if(!fnc_e.committed && &fnc_e != &new_evt) continue; // Not committed
            if(fnc_e.colour & HB) continue; // Already inserted
            if((fnc_e.has_load && fnc_load <= i) ||
               (fnc_e.has_store && fnc_store <= i)){
              /* (e,fnc_e) in fences */
              int new_colour = fnc_e.colour;
              if(sync_idx <= i || (e->colour & (FRE | RFE))){
                new_colour |= (HB | HB_SYNC);
              }else{
                new_colour |= HB_SYNC;
              }
              if(new_colour != fnc_e.colour){
                fnc_e.colour = new_colour;
                stack.push_back(&fnc_e);
              }
            }
          }
        }
        if(e->colour & (HB | HB_SYNC)){
          /* Continue via hb */
          const std::function<void(const IID<int>&)> add_hb =
            [this,&stack,e,HB,HB_SYNC](const IID<int> &iid){
            Event &hb_e = get_evt(iid);
            int new_colour = hb_e.colour | (e->colour & (HB | HB_SYNC));
            if(!(new_colour & HB) &&
               (e->iid.get_pid() == hb_e.iid.get_pid()) &&
               (e->sync_idx_after < hb_e.iid.get_index())){
              /* (e,hb_e) in ffence */
              new_colour |= HB;
            }
            if(new_colour != hb_e.colour){
              hb_e.colour = new_colour;
              stack.push_back(&hb_e);
            }
          };
          const VecSet<IID<int> > &hb = (e == &new_evt) ? rel.hb.fwd : e->rel.hb.fwd;
          for(const IID<int> &iid : hb){
            add_hb(iid);
          }
          if(e->colour & IN_I){
            for(const IID<int> &iid : EHB.after_I) add_hb(iid);
            for(const IID<int> &iid : EHB.after_C) add_hb(iid);
          }else if(e->colour & IN_C){
            for(const IID<int> &iid : EHB.after_C) add_hb(iid);
          }
          if(e->colour & HB_NE){
            add_hb(new_evt.iid);
          }
        }
      }
    }

    return true;
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::observation_get_fre(Event &r_evt, Event &new_evt, Relations &rel,
                                                VecSet<Event*> &fre){
    const int proc = r_evt.iid.get_pid();
    const VecSet<IID<int> > rf = (&r_evt == &new_evt) ? rel.rf.bwd : r_evt.rel.rf.bwd;
    fre.clear();
    for(unsigned i = 0; i < r_evt.baccesses.size(); ++i){
      if(r_evt.baccesses[i].type == ByteAccess::STORE) continue;
      void * const addr = r_evt.baccesses[i].addr;
      const Mem<MemMod,CB,Event> &m = mem[addr];
      for(int j = int(m.coherence.size())-1; 0 <= j; --j){
        if(rf.count(m.coherence[j])) break;
        if(m.coherence[j].get_pid() != proc){
          fre.insert(&get_evt(m.coherence[j]));
        }
      }
    }
  }

  /* Search forward from new_evt and EHB.after_I and EHB.after_C for
   * load events.
   */
  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::observation_find_inits(Event &new_evt, Relations &rel, ExtraHB &EHB,
                                                   VecSet<Event*> &inits){
    inits.clear();
    /* Masks for colour */
    const int COM = 1;
    const int HB = 2;
    const int IN_I = 4;
    const int IN_C = 8;
    /* First find all events from which we should search via hb:
     * - EHB.after_I U EHB.after_C
     * - new_evt
     * - Every event e such that (new_evt,e) in com*;fences
     */
    VecSet<Event*> hb_inits;
    for(const IID<int> &iid : EHB.after_I){
      hb_inits.insert(&get_evt(iid));
    }
    for(const IID<int> &iid : EHB.after_C){
      hb_inits.insert(&get_evt(iid));
    }
    hb_inits.insert(&new_evt);
    clear_event_colour();
    for(const IID<int> &iid : EHB.I){ get_evt(iid).colour |= IN_I; }
    for(const IID<int> &iid : EHB.C){ get_evt(iid).colour |= IN_C; }
    {
      std::vector<Event*> stack;
      stack.push_back(&new_evt);
      std::vector<int> fnc_store(threads.size(),-1);
      std::vector<int> fnc_load(threads.size(),-1);
      const std::function<int(int,int)> fncmin =
        [](int a, int b){
        if(a == -1) return b;
        if(b == -1) return a;
        return std::min(a,b);
      };
      while(stack.size()){
        Event *e = stack.back();
        stack.pop_back();
        /* Events reachable from e by fences */
        const int proc = e->iid.get_pid();
        fnc_store[proc] =
          fncmin(fnc_store[proc],fncmin(e->lwsync_idx_after,e->sync_idx_after));
        if(e->has_store) fnc_store[proc] = fncmin(fnc_store[proc],e->eieio_idx_after);
        fnc_load[proc] = fncmin(fnc_load[proc],e->sync_idx_after);
        if(e->has_load) fnc_load[proc] = fncmin(fnc_load[proc],e->lwsync_idx_after);
        /* Events reachable from e by com */
        const VecSet<IID<int> > &com =
          (e == &new_evt) ? rel.com.fwd : e->rel.com.fwd;
        for(const IID<int> &iid : com){
          Event &e2 = get_evt(iid);
          if(e2.colour & COM) continue;
          e2.colour |= COM;
          stack.push_back(&e2);
        }
      }
      for(unsigned p = 0; p < threads.size(); ++p){
        if(fnc_store[p] == -1 && fnc_load[p] == -1) continue;
        const int fch_count = threads[p].fch_count;
        const int fnc = fncmin(fnc_store[p],fnc_load[p]);
        for(int i = fnc; i < fch_count; ++i){
          if((fch[p][i].has_load && fnc_load[p] != -1 && fnc_load[p] <= i) ||
             (fch[p][i].has_store && fnc_store[p] != -1 && fnc_store[p] <= i)){
            hb_inits.insert(&fch[p][i]);
          }
        }
      }
    }
    /* Next search for loads via hb from hb_inits */
    std::function<std::string(Event * const &)> f =
      [](Event * const &p){
      return p->iid.to_string();
    };
    {
      for(Event *p : hb_inits){ p->colour |= HB; }
      std::vector<Event*> stack(hb_inits.begin(),hb_inits.end());
      const std::function<void(const VecSet<IID<int> >&)> follow_hb =
        [this,&stack,HB](const VecSet<IID<int> > &fwd){
        for(const IID<int> &iid : fwd){
          Event &e2 = this->get_evt(iid);
          if(!(e2.colour & HB)){
            e2.colour |= HB;
            stack.push_back(&e2);
          }
        }
      };
      while(stack.size()){
        Event *e = stack.back();
        stack.pop_back();
        if(e->has_load) inits.insert(e);
        const VecSet<IID<int> > &hb =
          (e == &new_evt) ? rel.hb.fwd : e->rel.hb.fwd;
        follow_hb(hb);
        if(e->colour & IN_I){
          follow_hb(EHB.after_I);
          follow_hb(EHB.after_C);
        }else if(e->colour & IN_C){
          follow_hb(EHB.after_C);
        }
      }
    }
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::debug_print() const{
    for(const IID<int> &iid : prefix){
      const Event &evt = fch[iid.get_pid()][iid.get_index()-1];
      llvm::dbgs() << iid << " "
                   << evt.to_string(evt.cur_param);
      if(evt.new_params.size()){
        llvm::dbgs() << " #prm:" << evt.new_params.size();
      }
      if(evt.new_branches.size()){
        llvm::dbgs() << " #bnc:" << evt.new_branches.size();
      }
      llvm::dbgs() << "\n";
    }
    if(!sleepset_is_empty()){
      llvm::dbgs() << "BRANCH BLOCKED\n";
    }
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::union_except_one(VecSet<Branch> &tgt, const VecSet<Branch> &B){
    VecSet<Branch> A(std::move(tgt));
    tgt.clear();
    std::vector<Branch> AB;
    AB.reserve(A.size()+B.size()-1);
    int a = 0, b = 1;
    while(a < A.size() && b < B.size()){
      if(A[a] < B[b]){
        AB.push_back(std::move(A[a]));
        ++a;
      }else if(A[a] == B[b]){
        AB.push_back(std::move(A[a]));
        ++a;
        ++b;
      }else{
        AB.push_back(B[b]);
        ++b;
      }
    }
    while(a < A.size()){
      AB.push_back(std::move(A[a]));
      ++a;
    }
    while(b < B.size()){
      AB.push_back(B[b]);
      ++b;
    }
    tgt = VecSet<Branch>(std::move(AB));
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::trace_register_metadata(int proc, const llvm::MDNode *md){
    if(TRec.is_active()){
      TRec.trace_register_metadata(proc,md);
    }
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::trace_register_external_function_call(int proc, const std::string &fname, const llvm::MDNode *md){
    if(TRec.is_active()){
      TRec.trace_register_external_function_call(proc,fname,md);
    }
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::trace_register_function_entry(int proc, const std::string &fname, const llvm::MDNode *md){
    if(TRec.is_active()){
      TRec.trace_register_function_entry(proc,fname,md);
    }
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::trace_register_function_exit(int proc){
    if(TRec.is_active()){
      TRec.trace_register_function_exit(proc);
    }
  }

  template<MemoryModel MemMod,CB_T CB, class Event>
  void TB<MemMod,CB,Event>::trace_register_error(int proc, const std::string &err_msg){
    if(TRec.is_active()){
      TRec.trace_register_error(proc,err_msg);
    }
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  void TB<MemMod,CB,Event>::replay(){    /* Clear events from this computation */
    {
      for(unsigned p = 0; p < threads.size(); ++p){
        const int fch_count = threads[p].fch_count;
        for(int j = 0; j < fch_count; ++j){
          Event &evt = fch[p][j];
          if(evt.committed){
            /* Remove selectively */
            evt.committed = false;
            evt.accesses.clear();
            evt.baccesses.clear();
          }else{
            /* Remove the event entirely */
            evt.filled_status = Event::STATUS_EMPTY;
          }
        }
      }
    }

    /* Clear various other data */
    sched_count = 0;
    fetch_count = 0;
    threads.clear();
    threads.emplace_back();
    is_aborted = false;
    mem.clear();
    next_clock_index = 0;
    cpids.clear();
    cpids.emplace_back();
    CPS = CPidSystem();
    for(Error *e : errors) delete e;
    errors.clear();
    TRec.clear();
    TRec.activate();
  }

  template<MemoryModel MemMod,CB_T CB,class Event>
  bool TB<MemMod,CB,Event>::reset(){
    if(conf.debug_print_on_reset){
      llvm::dbgs() << " === POWERARMTraceBuilder reset ===\n";
      debug_print();
      llvm::dbgs() << " ==================================\n";
    }

    /* Find the latest event where a different parameter may be explored. */
    int i;
    for(i = prefix.size()-1; 0 <= i; --i){
      Event &evt = get_evt(prefix[i]);
      if(evt.new_params.size() || evt.new_branches.size()){
        break;
      }
    }
    if(i < 0) return false; // Nothing more to explore.

    int new_pfx_len;
    Event &evt = get_evt(prefix[i]);
    if(evt.new_params.size()){ // New parameters that are already available
      if(evt.in_locked_branch()){
        /* The only events in a locked branch where new parameters are
         * allowed are the store and load at the very end of the
         * branch.
         */
        assert(evt.branch_end == i || evt.branch_end == i+1);
        evt.cur_param = std::move(evt.new_params.back());
        evt.new_params.pop_back();
        new_pfx_len = evt.branch_end+1;
        if(i != evt.branch_end){
          // This is the store.
          // Reset the parameter choices for the load at the end of the branch
          assert(get_evt(prefix[i+1]).new_branches.empty());
          assert(get_evt(prefix[i+1]).new_params.empty());
          get_evt(prefix[i+1]).recalc_params = PAEvent::RECALC_LOAD;
          get_evt(prefix[i+1]).filled_status = Event::STATUS_PARAMETER;
          assert(get_evt(prefix[i+1]).recalc_params_load_src == prefix[i]);
        }
      }else{
        /* Ordinary exploration of available parameters. */
        evt.cur_param = std::move(evt.new_params.back());
        evt.new_params.pop_back();
        new_pfx_len = i+1;
      }
    }else{ // Lower priority than new parameters.
      assert(evt.new_branches.size());
      VecSet<Branch> branches(std::move(evt.new_branches));
      evt.new_branches.clear();
      const Branch &B = branches[0];
      new_pfx_len = i+int(B.branch.size());
      if(int(prefix.size()) < new_pfx_len){
        prefix.resize(new_pfx_len);
      }
      for(int j = 0; j < int(B.branch.size()); ++j){
        prefix[i+j] = B.branch[j].iid;
        assert(get_evt(B.branch[j].iid).iid == B.branch[j].iid);
        get_evt(B.branch[j].iid).cur_param = B.branch[j].param;
        get_evt(B.branch[j].iid).branch_start = i;
        get_evt(B.branch[j].iid).branch_end = i+B.branch.size()-1;
        get_evt(B.branch[j].iid).new_branches.clear();
        get_evt(B.branch[j].iid).new_params.clear();
        get_evt(B.branch[j].iid).filled_status = Event::STATUS_PARAMETER;
        get_evt(B.branch[j].iid).recalc_params = PAEvent::RECALC_PARAM;
      }
      /* Setup recalc_params for the tail of the branch */
      switch(B.param_type){
      case Branch::PARAM_WR:
        assert(2 <= B.branch.size());
        assert(B.branch[B.branch.size()-2].param.choices.empty());
        assert(B.branch[B.branch.size()-1].param.choices.empty());
        get_evt(B.branch[B.branch.size()-2].iid).recalc_params = PAEvent::RECALC_STAR;
        get_evt(B.branch[B.branch.size()-1].iid).recalc_params = PAEvent::RECALC_LOAD;
        get_evt(B.branch[B.branch.size()-1].iid).recalc_params_load_src = B.branch[B.branch.size()-2].iid;
        break;
      case Branch::PARAM_STAR:
        assert(1 <= B.branch.size());
        assert(B.branch[B.branch.size()-1].param.choices.empty());
        get_evt(B.branch[B.branch.size()-1].iid).recalc_params = PAEvent::RECALC_STAR;
        break;
      default:
        throw std::logic_error("POWERARMTraceBuilder: Unknown type of parameter choice in locked branch.");
      }
      // Add brances to get_evt(prefix[i]).new_branches except for branches[0]
      union_except_one(get_evt(prefix[i]).new_branches,branches);
      get_evt(prefix[i]).cur_branch = B;
    }

    /* Clear events from this computation */
    {
      VecSet<IID<int> > keep;
      for(int j = 0; j < new_pfx_len; ++j){
        keep.insert(prefix[j]);
      }
      for(unsigned p = 0; p < threads.size(); ++p){
        const int fch_count = threads[p].fch_count;
        for(int j = 0; j < fch_count; ++j){
          Event &evt = fch[p][j];
          if(keep.count(evt.iid)){
            /* Remove selectively */
            evt.committed = false;
            evt.accesses.clear();
            evt.baccesses.clear();
          }else{
            /* Remove the event entirely */
            evt.filled_status = Event::STATUS_EMPTY;
          }
        }
      }
    }
    prefix.resize(new_pfx_len);

    /* Clear various other data */
    sched_count = 0;
    fetch_count = 0;
    uncommitted_nonblocking_count = 0;
    threads.clear();
    threads.emplace_back();
    is_aborted = false;
    mem.clear();
    next_clock_index = 0;
    cpids.clear();
    cpids.emplace_back();
    CPS = CPidSystem();
    for(Error *e : errors) delete e;
    errors.clear();
    TRec.clear();
    TRec.deactivate();

    return true;
  }

} // End namespace PATB_impl
