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

#include "Debug.h"
#include "PSOTraceBuilder.h"
#include "TraceUtil.h"

#include <sstream>
#include <stdexcept>

PSOTraceBuilder::PSOTraceBuilder(const Configuration &conf) : TSOPSOTraceBuilder(conf) {
  threads.push_back(Thread(0,CPid(),{},-1));
  proc_to_ipid.push_back(0);
  prefix_idx = -1;
  dryrun = false;
  replay = false;
  last_full_memory_conflict = -1;
  available_threads.insert(0);
  last_md = 0;
}

PSOTraceBuilder::~PSOTraceBuilder(){
}

bool PSOTraceBuilder::schedule(int *proc, int *aux, int *alt, bool *dryrun){
  *dryrun = false;
  *alt = 0;
  this->dryrun = false;
  if(replay){
    /* Are we done with the current Event? */
    if(0 <= prefix_idx &&
       threads[curnode().iid.get_pid()].clock[curnode().iid.get_pid()] <
       curnode().iid.get_index() + curnode().size - 1){
      /* Continue executing the current Event */
      IPid pid = curnode().iid.get_pid();
      *proc = threads[pid].proc;
      if(threads[pid].cpid.is_auxiliary()) *aux = threads[pid].cpid.get_aux_index();
      else *aux = -1;
      *alt = 0;
      assert(threads[pid].available);
      ++threads[pid].clock[pid];
      return true;
    }else if(prefix_idx + 1 == int(prefix.size())){
      /* We are done replaying. Continue below... */
      replay = false;
    }else if(dry_sleepers < int(prefix[prefix_idx+1].sleep.size())){
      /* Before going to the next event, dry run the threads that are
       * being put to sleep.
       */
      IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers];
      ++dry_sleepers;
      threads[pid].sleeping = true;
      sleepers.insert(pid);
      *proc = threads[pid].proc;
      if(threads[pid].cpid.is_auxiliary()) *aux = threads[pid].cpid.get_aux_index();
      else *aux = -1;
      *alt = 0;
      *dryrun = true;
      this->dryrun = true;
      return true;
    }else{
      /* Go to the next event. */
      dry_sleepers = 0;
      ++prefix_idx;
      IPid pid = curnode().iid.get_pid();
      *proc = threads[pid].proc;
      if(threads[pid].cpid.is_auxiliary()) *aux = threads[pid].cpid.get_aux_index();
      else *aux = -1;
      *alt = curnode().alt;
      assert(threads[pid].available);
      ++threads[pid].clock[pid];
      curnode().clock = threads[pid].clock;
      return true;
    }
  }

  assert(!replay);
  /* Create a new Event */

  /* Should we merge the last two events? */
  if(prefix.size() > 1 &&
     prefix[prefix.size()-1].iid.get_pid() == prefix[prefix.size()-2].iid.get_pid() &&
     !prefix[prefix.size()-1].may_conflict &&
     prefix[prefix.size()-1].sleep.empty()){
    assert(prefix[prefix.size()-1].branch.empty());
    assert(prefix[prefix.size()-1].wakeup.empty());
    ++prefix[prefix.size()-2].size;
    prefix.pop_back();
    --prefix_idx;
  }

  /* Create a new Event */
  ++prefix_idx;
  assert(prefix_idx == int(prefix.size()));

  /* Find an available thread (auxiliary or real).
   *
   * Prioritize auxiliary before real, and older before younger
   * threads.
   */
  IPid p = -1;
  for(IPid p_aux : available_auxs){
    if(threads[p_aux].available && !threads[p_aux].sleeping &&
       (conf.max_search_depth < 0 || threads[p_aux].clock[p_aux] < conf.max_search_depth) &&
       is_aux_at_head(p_aux)){
      p = p_aux;
      *aux = threads[p].cpid.get_aux_index();
      break;
    }
  }
  if(p < 0){
    for(IPid p_real : available_threads){
      if(threads[p_real].available && !threads[p_real].sleeping &&
         (conf.max_search_depth < 0 || threads[p_real].clock[p_real] < conf.max_search_depth)){
        p = p_real;
        *aux = -1;
        break;
      }
    }
  }

  if(p < 0){ // No threads available
    return false;
  }

  ++threads[p].clock[p];
  prefix.push_back(Event(IID<IPid>(IPid(p),threads[p].clock[p]),
                         threads[p].clock));
  *proc = threads[p].proc;
  return true;
}

bool PSOTraceBuilder::is_aux_at_head(IPid pid) const{
  assert(threads[pid].cpid.is_auxiliary());
  IPid parent_pid = threads[pid].parent;
  int aux = threads[pid].cpid.get_aux_index();
  SymAddr b0 = threads[parent_pid].aux_to_byte[aux];
  auto it = threads[parent_pid].store_buffers.find(b0);
  if(it == threads[parent_pid].store_buffers.end()){
    return false;
  }
  assert(it->second.size());
  const SymAddrSize &ml = it->second.front().ml;
  for(SymAddr b : ml){
    if(b == b0) continue;
    assert(threads[parent_pid].store_buffers.count(b));
    assert(threads[parent_pid].store_buffers.at(b).size());
    if(threads[parent_pid].store_buffers.at(b).front().ml.addr != b0){
      return false;
    }
    assert(threads[parent_pid].store_buffers.at(b).front().ml == ml);
  }
  return true;
}

void PSOTraceBuilder::refuse_schedule(){
  assert(prefix_idx == int(prefix.size())-1);
  assert(prefix.back().size == 1);
  assert(!prefix.back().may_conflict);
  assert(prefix.back().sleep.empty());
  assert(prefix.back().wakeup.empty());
  assert(prefix.back().branch.empty());
  IPid last_pid = prefix.back().iid.get_pid();
  prefix.pop_back();
  --prefix_idx;
  --threads[last_pid].clock[last_pid];
  mark_unavailable_ipid(last_pid);
}

void PSOTraceBuilder::mark_available_ipid(IPid pid){
  threads[pid].available = true;
  if(threads[pid].cpid.is_auxiliary()){
    available_auxs.insert(pid);
  }else{
    available_threads.insert(pid);
  }
}

void PSOTraceBuilder::mark_available(int proc, int aux){
  mark_available_ipid(ipid(proc,aux));
}

void PSOTraceBuilder::mark_unavailable_ipid(IPid pid){
  threads[pid].available = false;
  if(threads[pid].cpid.is_auxiliary()){
    available_auxs.erase(pid);
  }else{
    available_threads.erase(pid);
  }
}

void PSOTraceBuilder::mark_unavailable(int proc, int aux){
  mark_unavailable_ipid(ipid(proc,aux));
}

bool PSOTraceBuilder::is_replaying() const {
  return replay && (prefix_idx + 1 < int(prefix.size()));
}

void PSOTraceBuilder::cancel_replay(){
  if(!replay) return;
  replay = false;
  prefix.resize(prefix_idx+1,Event(IID<IPid>(),VClock<IPid>()));
}

void PSOTraceBuilder::metadata(const llvm::MDNode *md){
  if(curnode().md == 0){
    curnode().md = md;
  }
  last_md = md;
}

bool PSOTraceBuilder::sleepset_is_empty() const{
  return sleepers.empty();
}

bool PSOTraceBuilder::check_for_cycles(){
  IID<IPid> i_iid;
  if(!has_cycle(&i_iid)) return false;

  /* Report cycle */
  {
    assert(prefix.size());
    IID<CPid> c_iid(threads[i_iid.get_pid()].cpid,i_iid.get_index());
    errors.push_back(new RobustnessError(c_iid));
  }

  return true;
}

Trace *PSOTraceBuilder::get_trace() const{
  std::vector<IID<CPid> > cmp;
  SrcLocVectorBuilder cmp_md;
  std::vector<Error*> errs;
  for(unsigned i = 0; i < prefix.size(); ++i){
    cmp.push_back(IID<CPid>(threads[prefix[i].iid.get_pid()].cpid,prefix[i].iid.get_index()));
    cmp_md.push_from(prefix[i].md);
  }
  for(unsigned i = 0; i < errors.size(); ++i){
    errs.push_back(errors[i]->clone());
  }
  Trace *t = new IIDSeqTrace(cmp,cmp_md.build(),errs);
  t->set_blocked(!sleepset_is_empty());
  return t;
}

bool PSOTraceBuilder::reset(){
  if(conf.debug_print_on_reset){
    llvm::dbgs() << " === PSOTraceBuilder reset ===\n";
    debug_print();
    llvm::dbgs() << " =============================\n";
  }

  int i;
  for(i = int(prefix.size())-1; 0 <= i; --i){
    if(prefix[i].branch.size()){
      break;
    }
  }

  if(i < 0){
    /* No more branching is possible. */
    return false;
  }

  /* Setup the new Event at prefix[i] */
  {
    Branch br = prefix[i].branch[0];

    /* Find the index of br.pid. */
    int br_idx = 1;
    for(int j = i-1; br_idx == 1 && 0 <= j; --j){
      if(prefix[j].iid.get_pid() == br.pid){
        br_idx = prefix[j].iid.get_index() + prefix[j].size;
      }
    }

    Event evt(IID<IPid>(br.pid,br_idx),{});

    evt.alt = br.alt;
    evt.branch = prefix[i].branch;
    evt.branch.erase(br);
    evt.sleep = prefix[i].sleep;
    if(br.pid != prefix[i].iid.get_pid()){
      evt.sleep.insert(prefix[i].iid.get_pid());
    }
    evt.sleep_branch_trace_count =
      prefix[i].sleep_branch_trace_count + estimate_trace_count(i+1);

    prefix[i] = evt;

    prefix.resize(i+1,prefix[0]);
  }

  CPS = CPidSystem();
  threads.clear();
  threads.push_back(Thread(0,CPid(),{},-1));
  proc_to_ipid.clear();
  proc_to_ipid.push_back(0);
  mutexes.clear();
  cond_vars.clear();
  mem.clear();
  last_full_memory_conflict = -1;
  prefix_idx = -1;
  dryrun = false;
  replay = true;
  dry_sleepers = 0;
  sleepers.clear();
  available_threads.clear();
  available_auxs.clear();
  available_threads.insert(0);
  last_md = 0;
  reset_cond_branch_log();

  return true;
}

IID<CPid> PSOTraceBuilder::get_iid() const{
  IPid pid = curnode().iid.get_pid();
  int idx = curnode().iid.get_index();
  return IID<CPid>(threads[pid].cpid,idx);
}

static std::string rpad(std::string s, int n){
  while(int(s.size()) < n) s += " ";
  return s;
}

std::string PSOTraceBuilder::iid_string(const Event &evt) const{
  std::stringstream ss;
  ss << "(" << threads[evt.iid.get_pid()].cpid << "," << evt.iid.get_index();
  if(evt.size > 1){
    ss << "-" << evt.iid.get_index() + evt.size - 1;
  }
  ss << ")";
  if(evt.alt != 0){
    ss << "-alt:" << evt.alt;
  }
  return ss.str();
}

void PSOTraceBuilder::debug_print() const {
  llvm::dbgs() << "PSOTraceBuilder (debug print):\n";
  int iid_offs = 0;
  int clock_offs = 0;
  VecSet<IPid> sleep_set;
  for(unsigned i = 0; i < prefix.size(); ++i){
    IPid ipid = prefix[i].iid.get_pid();
    iid_offs = std::max(iid_offs,2*ipid+int(iid_string(prefix[i]).size()));
    clock_offs = std::max(clock_offs,int(prefix[i].clock.to_string().size()));
  }
  for(unsigned i = 0; i < prefix.size(); ++i){
    IPid ipid = prefix[i].iid.get_pid();
    llvm::dbgs() << rpad("",2+ipid*2)
                 << rpad(iid_string(prefix[i]),iid_offs-ipid*2)
                 << " " << rpad(prefix[i].clock.to_string(),clock_offs);
    sleep_set.insert(prefix[i].sleep);
    llvm::dbgs() << " SLP:{";
    for(int i = 0; i < sleep_set.size(); ++i){
      if(i != 0) llvm::dbgs() << ", ";
      llvm::dbgs() << threads[sleep_set[i]].cpid;
    }
    llvm::dbgs() << "}";
    if(prefix[i].branch.size()){
      llvm::dbgs() << " branch: ";
      for(Branch b : prefix[i].branch){
        llvm::dbgs() << threads[b.pid].cpid;
        if(b.alt != 0){
          llvm::dbgs() << "-alt:" << b.alt;
        }
        llvm::dbgs() << " ";
      }
    }
    llvm::dbgs() << "\n";
    for(IPid p : prefix[i].wakeup){
      sleep_set.erase(p);
    }
  }
  if(errors.size()){
    llvm::dbgs() << "Errors:\n";
    for(unsigned i = 0; i < errors.size(); ++i){
      llvm::dbgs() << "  Error #" << i+1 << ": "
                   << errors[i]->to_string() << "\n";
    }
  }
}

void PSOTraceBuilder::spawn(){
  IPid parent_ipid = curnode().iid.get_pid();
  IPid child_ipid = threads.size();
  CPid child_cpid = CPS.spawn(threads[parent_ipid].cpid);
  int proc = 0;
  for(unsigned i = 0; i < threads.size(); ++i){
    proc = std::max(proc,threads[i].proc+1);
  }
  proc_to_ipid.push_back(child_ipid);
  threads.push_back(Thread(proc,child_cpid,threads[parent_ipid].clock,parent_ipid));
  mark_available_ipid(child_ipid);
}

void PSOTraceBuilder::store(const SymData &sd){
  if(dryrun) return;
  const SymAddrSize &ml = sd.get_ref();
  IPid ipid = curnode().iid.get_pid();
  for(SymAddr b : ml){
    threads[ipid].store_buffers[b].push_back(PendingStoreByte(ml,threads[ipid].clock,last_md));
  }
  IPid upd_ipid;
  auto it = threads[ipid].byte_to_aux.find(ml.addr);
  if(it == threads[ipid].byte_to_aux.end()){
    /* Create new auxiliary thread */
    int aux_idx = int(threads[ipid].aux_to_byte.size());
    upd_ipid = int(threads.size());
    threads.push_back(Thread(threads[ipid].proc,CPS.new_aux(threads[ipid].cpid),threads[ipid].clock,ipid));
    threads[ipid].byte_to_aux[ml.addr] = aux_idx;
    threads[ipid].aux_to_byte.push_back(ml.addr);
    threads[ipid].aux_to_ipid.push_back(upd_ipid);
  }else{
    upd_ipid = threads[ipid].aux_to_ipid[it->second];
  }
  mark_available_ipid(upd_ipid);
}

void PSOTraceBuilder::atomic_store(const SymData &sd){
  const SymAddrSize &ml = sd.get_ref();
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.size()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    VecSet<SymAddr> &A = threads[pid].sleep_accesses_w;
    for(SymAddr b : ml){
      A.insert(b);
    }
    return;
  }
  IPid ipid = curnode().iid.get_pid();
  curnode().may_conflict = true;
  bool is_update = threads[ipid].cpid.is_auxiliary();

  IPid uipid = ipid; // ID of the thread changing the memory
  IPid tipid = is_update ? threads[ipid].parent : ipid; // ID of the (real) thread that issued the store

  if(is_update){ // Add the clock of the store instruction
    assert(threads[tipid].store_buffers.count(ml.addr));
    assert(threads[tipid].store_buffers[ml.addr].size());
    assert(threads[tipid].store_buffers[ml.addr].front().ml == ml);
    const PendingStoreByte &pst = threads[tipid].store_buffers[ml.addr].front();
    curnode().clock += pst.clock;
    threads[uipid].clock += pst.clock;
    curnode().origin_iid = IID<IPid>(tipid,pst.clock[tipid]);
    curnode().md = pst.md;
  }else{ // Add the clock of auxiliary threads (because of fencing semantics)
    assert(threads[tipid].all_buffers_empty());
    for(IPid p : threads[tipid].aux_to_ipid){
      threads[tipid].clock += threads[p].clock;
      curnode().clock += threads[p].clock;
    }
  }

  VecSet<int> seen_accesses;

  /* See previous updates reads to ml */
  for(SymAddr b : ml){
    ByteInfo &bi = mem[b];
    int lu = bi.last_update;
    if(0 <= lu){
      IPid lu_tipid = prefix[lu].iid.get_pid();
      assert(0 <= lu_tipid && lu_tipid < int(threads.size()));
      if(threads[lu_tipid].cpid.is_auxiliary()){
        lu_tipid = threads[lu_tipid].parent;
      }
      if(lu_tipid != tipid){
        seen_accesses.insert(bi.last_update);
      }else{
        curnode().clock += prefix[lu].clock;
        threads[uipid].clock += prefix[lu].clock;
      }
    }
    for(int i : bi.last_read){
      if(0 <= i && prefix[i].iid.get_pid() != tipid) seen_accesses.insert(i);
    }
  }

  seen_accesses.insert(last_full_memory_conflict);

  see_events(seen_accesses);

  /* Register in memory */
  int last_rowe = is_update ? threads[tipid].store_buffers[ml.addr].front().last_rowe : -1;
  for(SymAddr b : ml){
    ByteInfo &bi = mem[b];
    bi.last_update = prefix_idx;
    bi.last_update_ml = ml;
    if(0 <= last_rowe){
      bi.last_read[threads[tipid].proc] = last_rowe;
    }
    wakeup(Access::W,b);
  }

  if(is_update){ /* Remove pending store from buffer */
    for(SymAddr b : ml){
      std::vector<PendingStoreByte> &sb = threads[tipid].store_buffers[b];
      for(unsigned i = 0; i < sb.size() - 1; ++i){
        sb[i] = sb[i+1];
      }
      sb.pop_back();
      if(sb.empty()){
        threads[tipid].store_buffers.erase(b);
        mark_unavailable_ipid(uipid);
      }
    }
    threads[tipid].aux_clock_sum += curnode().clock;
  }
}

void PSOTraceBuilder::compare_exchange
(const SymData &sd, const SymData::block_type expected, bool success){
  if(success){
    atomic_store(sd);
  }else{
    load(sd.get_ref());
  }
}

void PSOTraceBuilder::load(const SymAddrSize &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.size()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    VecSet<SymAddr> &A = threads[pid].sleep_accesses_r;
    for(SymAddr b : ml){
      A.insert(b);
    }
    return;
  }
  curnode().may_conflict = true;
  IPid ipid = curnode().iid.get_pid();

  /* Check if this is a ROWE */
  {
    auto it = threads[ipid].store_buffers.find(ml.addr);
    if(it != threads[ipid].store_buffers.end()){
      std::vector<PendingStoreByte> &sb = it->second;
      assert(sb.size());
      assert(sb.back().ml == ml);
      sb.back().last_rowe = prefix_idx;
      return;
    }
  }

  /* Load from memory */

  VecSet<int> seen_accesses;

  /* See all updates to the read bytes. */
  for(SymAddr b : ml){
    int lu = mem[b].last_update;
    const SymAddrSize &lu_ml = mem[b].last_update_ml;
    if(0 <= lu){
      IPid lu_tipid = prefix[lu].iid.get_pid();
      if(threads[lu_tipid].cpid.is_auxiliary()){
        lu_tipid = threads[lu_tipid].parent;
      }
      if(lu_tipid != ipid){
        seen_accesses.insert(lu);
      }else if(ml != lu_ml){
        const VClock<IPid> &clk = prefix[lu].clock;
        curnode().clock += clk;
        threads[ipid].clock += clk;
      }
    }
  }

  seen_accesses.insert(last_full_memory_conflict);

  see_events(seen_accesses);

  /* Register load in memory */
  for(SymAddr b : ml){
    mem[b].last_read[threads[ipid].proc] = prefix_idx;
    wakeup(Access::R,b);
  }
}

void PSOTraceBuilder::full_memory_conflict(){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.size()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_full_memory_conflict = true;
    return;
  }
  curnode().may_conflict = true;

  /* See all pervious memory accesses */
  VecSet<int> seen_accesses;
  for(auto it = mem.begin(); it != mem.end(); ++it){
    seen_accesses.insert(it->second.last_update);
    for(int i : it->second.last_read){
      seen_accesses.insert(i);
    }
  }
  for(auto it = mutexes.begin(); it != mutexes.end(); ++it){
    seen_accesses.insert(it->second.last_access);
  }
  seen_accesses.insert(last_full_memory_conflict);

  see_events(seen_accesses);

  wakeup(Access::W_ALL_MEMORY,{SymMBlock::Global(0),0});
  last_full_memory_conflict = prefix_idx;

  /* No later access can have a conflict with any earlier access */
  mem.clear();
}

void PSOTraceBuilder::fence(){
  if(dryrun) return;
  IPid ipid = curnode().iid.get_pid();
  assert(!threads[ipid].cpid.is_auxiliary());
  assert(threads[ipid].all_buffers_empty());
  curnode().clock += threads[ipid].aux_clock_sum;
  threads[ipid].clock += threads[ipid].aux_clock_sum;
}

void PSOTraceBuilder::join(int tgt_proc){
  if(dryrun) return;
  assert(0 <= tgt_proc && tgt_proc < int(proc_to_ipid.size()));
  IPid ipid = curnode().iid.get_pid();
  IPid tgt_ipid = proc_to_ipid[tgt_proc];
  curnode().clock += threads[tgt_ipid].clock;
  curnode().clock += threads[tgt_ipid].aux_clock_sum;
  threads[ipid].clock += curnode().clock;
}

void PSOTraceBuilder::mutex_lock(const SymAddrSize &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.size()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.addr);
    return;
  }
  fence();
  curnode().may_conflict = true;
  wakeup(Access::W,ml.addr);

  Mutex &mutex = mutexes[ml.addr];
  IPid ipid = curnode().iid.get_pid();

  if(mutex.last_lock < 0){
    /* No previous lock */
    see_events({mutex.last_access,last_full_memory_conflict});
  }else{
    /* Register conflict with last preceding lock */
    if(!prefix[mutex.last_lock].clock.leq(curnode().clock)){
      add_branch(mutex.last_lock,prefix_idx);
    }
    curnode().clock += prefix[mutex.last_access].clock;
    threads[ipid].clock += prefix[mutex.last_access].clock;
    see_events({last_full_memory_conflict});
  }

  mutex.last_lock = mutex.last_access = prefix_idx;
  mutex.locked = true;
}

void PSOTraceBuilder::mutex_lock_fail(const SymAddrSize &ml){
  assert(!dryrun);
  assert(mutexes.count(ml.addr));
  Mutex &mutex = mutexes[ml.addr];
  assert(0 <= mutex.last_lock);
  if(!prefix[mutex.last_lock].clock.leq(curnode().clock)){
    add_branch(mutex.last_lock,prefix_idx);
  }

  if(0 <= last_full_memory_conflict &&
     !prefix[last_full_memory_conflict].clock.leq(curnode().clock)){
    add_branch(last_full_memory_conflict,prefix_idx);
  }
}

void PSOTraceBuilder::mutex_unlock(const SymAddrSize &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.size()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.addr);
    return;
  }
  fence();
  assert(mutexes.count(ml.addr));
  Mutex &mutex = mutexes[ml.addr];
  curnode().may_conflict = true;
  wakeup(Access::W,ml.addr);
  assert(0 <= mutex.last_access);

  see_events({mutex.last_access,last_full_memory_conflict});

  mutex.last_access = prefix_idx;
  mutex.locked = false;
}

void PSOTraceBuilder::mutex_trylock(const SymAddrSize &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.size()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.addr);
    return;
  }
  fence();
  assert(mutexes.count(ml.addr));
  curnode().may_conflict = true;
  wakeup(Access::W,ml.addr);
  Mutex &mutex = mutexes[ml.addr];
  see_events({mutex.last_access,last_full_memory_conflict});

  mutex.last_access = prefix_idx;
  if(!mutex.locked){ // Mutex is free
    mutex.last_lock = prefix_idx;
    mutex.locked = true;
  }
}

void PSOTraceBuilder::mutex_init(const SymAddrSize &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.size()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.addr);
    return;
  }
  fence();
  curnode().may_conflict = true;
  Mutex &mutex = mutexes[ml.addr];
  see_events({mutex.last_access, last_full_memory_conflict});

  mutex.last_access = prefix_idx;
}

void PSOTraceBuilder::mutex_destroy(const SymAddrSize &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.size()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.addr);
    return;
  }
  fence();
  assert(mutexes.count(ml.addr));
  Mutex &mutex = mutexes[ml.addr];
  curnode().may_conflict = true;
  wakeup(Access::W,ml.addr);

  see_events({mutex.last_access,last_full_memory_conflict});

  mutexes.erase(ml.addr);
}

bool PSOTraceBuilder::cond_init(const SymAddrSize &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.size()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.addr);
    return true;
  }
  fence();
  if(cond_vars.count(ml.addr)){
    pthreads_error("Condition variable initiated twice.");
    return false;
  }
  curnode().may_conflict = true;
  cond_vars[ml.addr] = CondVar(prefix_idx);
  see_events({last_full_memory_conflict});
  return true;
}

bool PSOTraceBuilder::cond_signal(const SymAddrSize &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.size()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.addr);
    return true;
  }
  fence();
  curnode().may_conflict = true;
  wakeup(Access::W,ml.addr);

  auto it = cond_vars.find(ml.addr);
  if(it == cond_vars.end()){
    pthreads_error("cond_signal called with uninitialized condition variable.");
    return false;
  }
  CondVar &cond_var = it->second;
  VecSet<int> seen_events = {last_full_memory_conflict};
  if(cond_var.waiters.size() > 1){
    register_alternatives(cond_var.waiters.size());
  }
  assert(0 <= curnode().alt);
  assert(cond_var.waiters.empty() || curnode().alt < int(cond_var.waiters.size()));
  if(cond_var.waiters.size()){
    /* Wake up the alt:th waiter. */
    int i = cond_var.waiters[curnode().alt];
    assert(0 <= i && i < prefix_idx);
    IPid ipid = prefix[i].iid.get_pid();
    assert(!threads[ipid].available);
    mark_available_ipid(ipid);
    /* The next instruction by the thread ipid should be ordered after
     * this signal.
     */
    threads[ipid].clock += curnode().clock;
    seen_events.insert(i);

    /* Remove waiter from cond_var.waiters */
    for(int j = curnode().alt; j < int(cond_var.waiters.size())-1; ++j){
      cond_var.waiters[j] = cond_var.waiters[j+1];
    }
    cond_var.waiters.pop_back();
  }
  cond_var.last_signal = prefix_idx;

  see_events(seen_events);

  return true;
}

bool PSOTraceBuilder::cond_broadcast(const SymAddrSize &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.size()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.addr);
    return true;
  }
  fence();
  curnode().may_conflict = true;
  wakeup(Access::W,ml.addr);

  auto it = cond_vars.find(ml.addr);
  if(it == cond_vars.end()){
    pthreads_error("cond_broadcast called with uninitialized condition variable.");
    return false;
  }
  CondVar &cond_var = it->second;
  VecSet<int> seen_events = {last_full_memory_conflict};
  for(int i : cond_var.waiters){
    assert(0 <= i && i < prefix_idx);
    IPid ipid = prefix[i].iid.get_pid();
    assert(!threads[ipid].available);
    mark_available_ipid(ipid);
    /* The next instruction by the thread ipid should be ordered after
     * this broadcast.
     */
    threads[ipid].clock += curnode().clock;
    seen_events.insert(i);
  }
  cond_var.waiters.clear();
  cond_var.last_signal = prefix_idx;

  see_events(seen_events);

  return true;
}

bool PSOTraceBuilder::cond_wait(const SymAddrSize &cond_ml, const SymAddrSize &mutex_ml){
  {
    auto it = mutexes.find(mutex_ml.addr);
    if(!dryrun && it == mutexes.end()){
      pthreads_error("cond_wait called with uninitialized mutex object.");
      return false;
    }
    Mutex &mtx = it->second;
    if(!dryrun && (mtx.last_lock < 0 || prefix[mtx.last_lock].iid.get_pid() != curnode().iid.get_pid())){
      pthreads_error("cond_wait called with mutex which is not locked by the same thread.");
      return false;
    }
  }

  mutex_unlock(mutex_ml);
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.size()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_r.insert(cond_ml.addr);
    return true;
  }
  fence();
  curnode().may_conflict = true;
  wakeup(Access::R,cond_ml.addr);

  IPid pid = curnode().iid.get_pid();

  auto it = cond_vars.find(cond_ml.addr);
  if(it == cond_vars.end()){
    pthreads_error("cond_wait called with uninitialized condition variable.");
    return false;
  }
  it->second.waiters.push_back(prefix_idx);
  mark_unavailable_ipid(pid);

  see_events({last_full_memory_conflict,it->second.last_signal});

  return true;
}

bool PSOTraceBuilder::cond_awake(const SymAddrSize &cond_ml, const SymAddrSize &mutex_ml){
  mutex_lock(mutex_ml);
  return true;
}

int PSOTraceBuilder::cond_destroy(const SymAddrSize &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.size()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.addr);
    return 0;
  }
  fence();

  int err = (EBUSY == 1) ? 2 : 1; // Chose an error value different from EBUSY

  curnode().may_conflict = true;
  wakeup(Access::W,ml.addr);

  auto it = cond_vars.find(ml.addr);
  if(it == cond_vars.end()){
    pthreads_error("cond_destroy called on uninitialized condition variable.");
    return err;
  }
  CondVar &cond_var = it->second;
  VecSet<int> seen_events = {cond_var.last_signal,last_full_memory_conflict};
  for(int i : cond_var.waiters) seen_events.insert(i);
  see_events(seen_events);

  int rv = cond_var.waiters.size() ? EBUSY : 0;
  cond_vars.erase(ml.addr);
  return rv;
}

void PSOTraceBuilder::register_alternatives(int alt_count){
  curnode().may_conflict = true;
  if(curnode().alt == 0){
    for(int i = curnode().alt+1; i < alt_count; ++i){
      curnode().branch.insert(Branch({curnode().iid.get_pid(),i}));
    }
  }
}

VecSet<PSOTraceBuilder::IPid> PSOTraceBuilder::sleep_set_at(int i){
  VecSet<IPid> sleep;
  for(int j = 0; j < i; ++j){
    sleep.insert(prefix[j].sleep);
    for(auto it = prefix[j].wakeup.begin(); it != prefix[j].wakeup.end(); ++it){
      sleep.erase(*it);
    }
  }
  sleep.insert(prefix[i].sleep);
  return sleep;
}

void PSOTraceBuilder::see_events(const VecSet<int> &seen_accesses){
  /* Register new branches */
  std::vector<int> branch;
  for(int i : seen_accesses){
    if(i < 0) continue;
    const VClock<IPid> &iclock = prefix[i].clock;
    if(iclock.leq(curnode().clock)) continue;
    if(std::any_of(seen_accesses.begin(),seen_accesses.end(),
                   [i,&iclock,this](int j){
                     return 0 <= j && i != j && iclock.leq(this->prefix[j].clock);
                   })) continue;
    branch.push_back(i);
  }

  /* Add clocks from seen accesses */
  IPid ipid = curnode().iid.get_pid();
  for(int i : seen_accesses){
    if(i < 0) continue;
    assert(0 <= i && i <= prefix_idx);
    curnode().clock += prefix[i].clock;
    threads[ipid].clock += prefix[i].clock;
  }

  for(int i : branch){
    add_branch(i,prefix_idx);
  }
}

void PSOTraceBuilder::add_branch(int i, int j){
  assert(0 <= i);
  assert(i < j);
  assert(j <= prefix_idx);

  VecSet<IPid> isleep = sleep_set_at(i);

  /* candidates is a map from IPid p to event index i such that the
   * IID (p,i) identifies an event between prefix[i] (exclusive) and
   * prefix[j] (inclusive) such that (p,i) does not happen-after any
   * other IID between prefix[i] (inclusive) and prefix[j]
   * (inclusive).
   *
   * If no such event has yet been found for a thread p, then
   * candidates[p] is out of bounds, or has the value -1.
   */
  std::vector<int> candidates;
  Branch cand = {-1,0};
  const VClock<IPid> &iclock = prefix[i].clock;
  for(int k = i+1; k <= j; ++k){
    IPid p = prefix[k].iid.get_pid();
    /* Did we already cover p? */
    if(p < int(candidates.size()) && 0 <= candidates[p]) continue;
    const VClock<IPid> &pclock = prefix[k].clock;
    /* Is p after prefix[i]? */
    if(k != j && iclock.leq(pclock)) continue;
    /* Is p after some other candidate? */
    bool is_after = false;
    for(int q = 0; !is_after && q < int(candidates.size()); ++q){
      if(0 <= candidates[q] && candidates[q] <= pclock[q]){
        is_after = true;
      }
    }
    if(is_after) continue;
    if(int(candidates.size()) <= p){
      candidates.resize(p+1,-1);
    }
    candidates[p] = prefix[k].iid.get_index();
    cand.pid = p;
    if(prefix[i].branch.count(cand)){
      /* There is already a satisfactory candidate branch */
      return;
    }
    if(isleep.count(cand.pid)){
      /* This candidate is already sleeping (has been considered) at
       * prefix[i]. */
      return;
    }
  }

  assert(0 <= cand.pid);
  prefix[i].branch.insert(cand);
}

bool PSOTraceBuilder::has_pending_store(IPid pid, SymAddr ml) const {
  return threads[pid].store_buffers.count(ml);
}

void PSOTraceBuilder::wakeup(Access::Type type, SymAddr ml){
  IPid pid = curnode().iid.get_pid();
  std::vector<IPid> wakeup; // Wakeup these
  switch(type){
  case Access::W_ALL_MEMORY:
    {
      for(unsigned p : sleepers){
        if(threads[p].sleep_full_memory_conflict ||
           threads[p].sleep_accesses_w.size()){
          wakeup.push_back(p);
        }else{
          for(SymAddr b : threads[p].sleep_accesses_r){
            if(!has_pending_store(p,b)){
              wakeup.push_back(p);
              break;
            }
          }
        }
      }
      break;
    }
  case Access::R:
    {
      for(unsigned p : sleepers){
        if(threads[p].sleep_full_memory_conflict ||
           (threads[p].proc != threads[pid].proc &&
            threads[p].sleep_accesses_w.count(ml))){
          wakeup.push_back(p);
        }
      }
      break;
    }
  case Access::W:
    {
      for(unsigned p : sleepers){
        if(threads[p].sleep_full_memory_conflict ||
           (threads[p].proc != threads[pid].proc &&
            (threads[p].sleep_accesses_w.count(ml) ||
             (threads[p].sleep_accesses_r.count(ml) &&
              !has_pending_store(p,ml))))){
          wakeup.push_back(p);
        }
      }
      break;
    }
  default:
    throw std::logic_error("PSOTraceBuilder::wakeup: Unknown type of memory access.");
  }

  for(IPid p : wakeup){
    assert(threads[p].sleeping);
    threads[p].sleep_accesses_r.clear();
    threads[p].sleep_accesses_w.clear();
    threads[p].sleep_full_memory_conflict = false;
    threads[p].sleeping = false;
    sleepers.erase(p);
    curnode().wakeup.insert(p);
  }
}

bool PSOTraceBuilder::has_cycle(IID<IPid> *loc) const{
  int real_thread_count = proc_to_ipid.size();
  int pfx_size = prefix.size();

  /* Identify all store events */
  struct stupd_t{
    /* The index part of the IID identifying a store event. */
    int store;
    /* The index in prefix of the corresponding update event. */
    int update;
    bool operator<(const stupd_t &su) const{
      return store < su.store;
    };
  };
  /* stores[proc] is all store events of thread proc, ordered by
   * store index.
   */
  std::vector<std::vector<stupd_t> > stores(real_thread_count);
  for(int i = 0; i < pfx_size; ++i){
    if(threads[prefix[i].iid.get_pid()].cpid.is_auxiliary()){ // Update
      assert(threads[prefix[i].origin_iid.get_pid()].cpid ==
             threads[prefix[i].iid.get_pid()].cpid.parent());
      int proc = threads[prefix[i].iid.get_pid()].proc;
      stores[proc].push_back({prefix[i].origin_iid.get_index(),i});
    }
  }
  for(int i = 0; i < real_thread_count; ++i){
    std::sort(stores[i].begin(),stores[i].end());
  }

  /* Attempt to replay computation under SC */
  struct thread_t {
    thread_t()
      : pc(0), pfx_index(0), store_index(0), blocked(false), block_clock() {};
    int pc; // Current program counter
    int pfx_index; // Index into prefix
    int store_index; // Index into stores
    bool blocked; // Is the thread currently blocked?
    VClock<IPid> block_clock; // If blocked, what are we waiting for?
  };
  std::vector<thread_t> threads(real_thread_count);

  int thread = 0; // The next scheduled thread
  /* alive keeps track of whether any thread has been successfully
   * scheduled lately
   */
  bool alive = false;
  while(true){
    IPid thread_ipid = proc_to_ipid[thread];

    // Advance pfx_index to the right Event in prefix
    while(threads[thread].pfx_index < pfx_size &&
          prefix[threads[thread].pfx_index].iid.get_pid() != thread_ipid){
      ++threads[thread].pfx_index;
    }
    if(pfx_size <= threads[thread].pfx_index){
      // This thread is finished
      thread = (thread+1)%real_thread_count;
      if(thread == 0){
        if(!alive) break;
        alive = false;
      }
      continue;
    }

    int next_pc = threads[thread].pc+1;
    const Event &evt = prefix[threads[thread].pfx_index];

    if(!threads[thread].blocked){
      assert(evt.iid.get_pid() == thread_ipid);
      assert(evt.iid.get_index() <= next_pc);
      assert(next_pc < evt.iid.get_index() + evt.size);
      threads[thread].block_clock = evt.clock;
      assert(threads[thread].block_clock[thread_ipid] <= next_pc);
      threads[thread].block_clock[thread_ipid] = next_pc;
      if(threads[thread].store_index < int(stores[thread].size()) &&
         stores[thread][threads[thread].store_index].store == next_pc){
        // This is a store. Also consider the update's clock.
        threads[thread].block_clock +=
          prefix[stores[thread][threads[thread].store_index].update].clock;
        ++threads[thread].store_index;
      }
    }

    // Is this process blocked?
    // Is there some process we have to wait for?
    {
      int i;
      threads[thread].blocked = false;
      for(i = 0; i < real_thread_count; ++i){
        if(i != thread && threads[i].pc < threads[thread].block_clock[proc_to_ipid[i]]){
          threads[thread].blocked = true;
          break;
        }
      }
    }

    // Are we still blocked?
    if(threads[thread].blocked){
      thread = (thread+1)%real_thread_count; // Try another thread
      if(thread == 0){
        if(!alive) break;
        alive = false;
      }
    }else{
      alive = true;
      threads[thread].pc = next_pc;
      assert(next_pc == threads[thread].block_clock[thread_ipid]);

      // Advance pc to next interesting event
      next_pc = evt.iid.get_index() + evt.size - 1;
      if(threads[thread].store_index < int(stores[thread].size()) &&
         stores[thread][threads[thread].store_index].store-1 < next_pc){
        next_pc = stores[thread][threads[thread].store_index].store-1;
      }
      assert(threads[thread].pc <= next_pc);
      threads[thread].pc = next_pc;

      if(next_pc + 1 == evt.iid.get_index() + evt.size){
        // We are done with this Event
        ++threads[thread].pfx_index;
      }
    }
  }

  // Did all threads finish, or are some still blocked?
  {
    int upd_idx = -1; // Index of the latest update involved in a cycle
    bool has_cycle = false;
    for(int i = 0; i < real_thread_count; ++i){
      if(threads[i].blocked){
        // There is a cycle
        has_cycle = true;
        int next_pc = threads[i].pc+1;
        if(0 < threads[i].store_index && stores[i][threads[i].store_index-1].store == next_pc){
          if(stores[i][threads[i].store_index-1].update > upd_idx){
            upd_idx = stores[i][threads[i].store_index-1].update;
            *loc = prefix[upd_idx].iid;
          }
        }
      }
    }
    assert(!has_cycle || 0 <= upd_idx);
    return has_cycle;
  }
}

long double PSOTraceBuilder::estimate_trace_count() const{
  return estimate_trace_count(0);
}

long double PSOTraceBuilder::estimate_trace_count(int idx) const{
  if(idx > int(prefix.size())) return 0;
  if(idx == int(prefix.size())) return 1;

  long double count = 1;
  for(int i = int(prefix.size())-1; idx <= i; --i){
    count += prefix[i].sleep_branch_trace_count;
    count += prefix[i].branch.size()*(count / (1 + prefix[i].sleep.size()));
  }

  return count;
}
