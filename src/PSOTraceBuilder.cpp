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

#include "Debug.h"
#include "PSOTraceBuilder.h"

#include <sstream>
#include <stdexcept>

PSOTraceBuilder::PSOTraceBuilder(const Configuration &conf) : TraceBuilder(conf) {
  threads.push_back(Thread(0,CPid(),{},-1));
  proc_to_ipid.push_back(0);
  prefix_idx = -1;
  dryrun = false;
  replay = false;
  last_full_memory_conflict = -1;
};

PSOTraceBuilder::~PSOTraceBuilder(){
};

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
  const unsigned sz = threads.size();
  unsigned p_real = sz, p_aux;
  for(p_aux = 0; p_aux < sz; ++p_aux){
    if(threads[p_aux].available && !threads[p_aux].sleeping &&
       (conf.max_search_depth < 0 || threads[p_aux].clock[p_aux] < conf.max_search_depth)){
      /* The thread is available. */
      if(threads[p_aux].cpid.is_auxiliary() && is_aux_at_head(p_aux)){
        break;
      }else{
        if(p_real == sz) p_real = p_aux;
      }
    }
  }

  unsigned p;
  if(p_aux < sz){ // Schedule auxiliary
    p = p_aux;
    *aux = threads[p].cpid.get_aux_index();
  }else if(p_real < sz){ // Schedule real
    p = p_real;
    *aux = -1;
  }else{ // No threads available
    return false;
  }

  ++threads[p].clock[p];
  prefix.push_back(Event(IID<IPid>(IPid(p),threads[p].clock[p]),
                         threads[p].clock));
  *proc = threads[p].proc;
  return true;
};

bool PSOTraceBuilder::is_aux_at_head(IPid pid) const{
  assert(threads[pid].cpid.is_auxiliary());
  IPid parent_pid = threads[pid].parent;
  int aux = threads[pid].cpid.get_aux_index();
  void const *b0 = threads[parent_pid].aux_to_byte[aux];
  auto it = threads[parent_pid].store_buffers.find(b0);
  if(it == threads[parent_pid].store_buffers.end()){
    return false;
  }
  assert(it->second.size());
  const ConstMRef &ml = it->second.front().ml;
  for(void const *b : ml){
    if(b == b0) continue;
    assert(threads[parent_pid].store_buffers.count(b));
    assert(threads[parent_pid].store_buffers.at(b).size());
    if(threads[parent_pid].store_buffers.at(b).front().ml.ref != b0){
      return false;
    }
    assert(threads[parent_pid].store_buffers.at(b).front().ml == ml);
  }
  return true;
};

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
  threads[last_pid].available = false;
};

void PSOTraceBuilder::mark_available(int proc, int aux){
  threads[ipid(proc,aux)].available = true;
};

void PSOTraceBuilder::mark_unavailable(int proc, int aux){
  threads[ipid(proc,aux)].available = false;
};

void PSOTraceBuilder::metadata(const llvm::MDNode *md){
  if(curnode().md == 0){
    curnode().md = md;
  }
};

bool PSOTraceBuilder::sleepset_is_empty() const{
  for(unsigned i = 0; i < threads.size(); ++i){
    if(threads[i].sleeping) return false;
  }
  return true;
};

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
};

Trace PSOTraceBuilder::get_trace() const{
  std::vector<IID<CPid> > cmp;
  std::vector<const llvm::MDNode*> cmp_md;
  std::vector<Error*> errs;
  for(unsigned i = 0; i < prefix.size(); ++i){
    cmp.push_back(IID<CPid>(threads[prefix[i].iid.get_pid()].cpid,prefix[i].iid.get_index()));
    cmp_md.push_back(prefix[i].md);
  };
  for(unsigned i = 0; i < errors.size(); ++i){
    errs.push_back(errors[i]->clone());
  }
  return Trace(cmp,cmp_md,errs);
};

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

    prefix[i] = evt;

    prefix.resize(i+1,prefix[0]);
  }

  CPS = CPidSystem();
  threads.clear();
  threads.push_back(Thread(0,CPid(),{},-1));
  proc_to_ipid.clear();
  proc_to_ipid.push_back(0);
  mutexes.clear();
  mem.clear();
  last_full_memory_conflict = -1;
  prefix_idx = -1;
  dryrun = false;
  replay = true;
  dry_sleepers = 0;

  return true;
};

static std::string rpad(std::string s, int n){
  while(int(s.size()) < n) s += " ";
  return s;
};

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
};

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
};

void PSOTraceBuilder::spawn(){
  IPid parent_ipid = curnode().iid.get_pid();
  CPid child_cpid = CPS.spawn(threads[parent_ipid].cpid);
  int proc = 0;
  for(unsigned i = 0; i < threads.size(); ++i){
    proc = std::max(proc,threads[i].proc+1);
  }
  proc_to_ipid.push_back(threads.size());
  threads.push_back(Thread(proc,child_cpid,threads[parent_ipid].clock,parent_ipid));
};

void PSOTraceBuilder::store(const ConstMRef &ml){
  if(dryrun) return;
  IPid ipid = curnode().iid.get_pid();
  for(void const *b : ml){
    threads[ipid].store_buffers[b].push_back(PendingStoreByte(ml,threads[ipid].clock,curnode().md));
  }
  IPid upd_ipid;
  auto it = threads[ipid].byte_to_aux.find(ml.ref);
  if(it == threads[ipid].byte_to_aux.end()){
    /* Create new auxiliary thread */
    int aux_idx = int(threads[ipid].aux_to_byte.size());
    upd_ipid = int(threads.size());
    threads.push_back(Thread(threads[ipid].proc,CPS.new_aux(threads[ipid].cpid),threads[ipid].clock,ipid));
    threads[ipid].byte_to_aux[ml.ref] = aux_idx;
    threads[ipid].aux_to_byte.push_back(ml.ref);
    threads[ipid].aux_to_ipid.push_back(upd_ipid);
  }else{
    upd_ipid = threads[ipid].aux_to_ipid[it->second];
  }
  threads[upd_ipid].available = true;
};

void PSOTraceBuilder::atomic_store(const ConstMRef &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.size()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    VecSet<void const*> &A = threads[pid].sleep_accesses_w;
    for(void const *b : ml){
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
    assert(threads[tipid].store_buffers.count(ml.ref));
    assert(threads[tipid].store_buffers[ml.ref].size());
    assert(threads[tipid].store_buffers[ml.ref].front().ml == ml);
    const PendingStoreByte &pst = threads[tipid].store_buffers[ml.ref].front();
    curnode().clock += pst.clock;
    threads[uipid].clock += pst.clock;
    curnode().origin_iid = IID<IPid>(tipid,pst.clock[tipid]);
  }else{ // Add the clock of auxiliary threads (because of fencing semantics)
    assert(threads[tipid].all_buffers_empty());
    for(IPid p : threads[tipid].aux_to_ipid){
      threads[tipid].clock += threads[p].clock;
      curnode().clock += threads[p].clock;
    }
  }

  VecSet<int> seen_accesses;

  /* See previous updates reads to ml */
  for(void const *b : ml){
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
  int last_rowe = is_update ? threads[tipid].store_buffers[ml.ref].front().last_rowe : -1;
  for(void const *b : ml){
    ByteInfo &bi = mem[b];
    bi.last_update = prefix_idx;
    bi.last_update_ml = ml;
    if(0 <= last_rowe){
      bi.last_read[threads[tipid].proc] = last_rowe;
    }
    wakeup(Access::W,b);
  }

  if(is_update){ /* Remove pending store from buffer */
    for(void const *b : ml){
      std::vector<PendingStoreByte> &sb = threads[tipid].store_buffers[b];
      for(unsigned i = 0; i < sb.size() - 1; ++i){
        sb[i] = sb[i+1];
      }
      sb.pop_back();
      if(sb.empty()){
        threads[tipid].store_buffers.erase(b);
        threads[uipid].available = false;
      }
    }
  }
};

void PSOTraceBuilder::load(const ConstMRef &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.size()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    VecSet<void const*> &A = threads[pid].sleep_accesses_r;
    for(void const *b : ml){
      A.insert(b);
    }
    return;
  }
  curnode().may_conflict = true;
  IPid ipid = curnode().iid.get_pid();

  /* Check if this is a ROWE */
  {
    auto it = threads[ipid].store_buffers.find(ml.ref);
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
  for(void const *b : ml){
    int lu = mem[b].last_update;
    const ConstMRef &lu_ml = mem[b].last_update_ml;
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
  for(void const *b : ml){
    mem[b].last_read[threads[ipid].proc] = prefix_idx;
    wakeup(Access::R,b);
  }
};

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

  wakeup(Access::W_ALL_MEMORY,0);
  last_full_memory_conflict = prefix_idx;

  /* No later access can have a conflict with any earlier access */
  mem.clear();
};

void PSOTraceBuilder::fence(){
  if(dryrun) return;
  IPid ipid = curnode().iid.get_pid();
  assert(!threads[ipid].cpid.is_auxiliary());
  assert(threads[ipid].all_buffers_empty());
  for(IPid p : threads[ipid].aux_to_ipid){
    curnode().clock += threads[p].clock;
    threads[ipid].clock += threads[p].clock;
  }
};

void PSOTraceBuilder::join(int tgt_proc){
  if(dryrun) return;
  assert(0 <= tgt_proc && tgt_proc < int(proc_to_ipid.size()));
  IPid ipid = curnode().iid.get_pid();
  IPid tgt_ipid = proc_to_ipid[tgt_proc];
  curnode().clock += threads[tgt_ipid].clock;
  threads[ipid].clock += threads[tgt_ipid].clock;
  for(IPid p : threads[tgt_ipid].aux_to_ipid){
    curnode().clock += threads[p].clock;
    threads[ipid].clock += threads[p].clock;
  }
};

void PSOTraceBuilder::mutex_lock(const ConstMRef &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.size()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.ref);
    return;
  }
  fence();
  assert(mutexes.count(ml.ref));
  curnode().may_conflict = true;
  wakeup(Access::W,ml.ref);

  Mutex &mutex = mutexes[ml.ref];
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
};

void PSOTraceBuilder::mutex_lock_fail(const ConstMRef &ml){
  assert(!dryrun);
  assert(mutexes.count(ml.ref));
  Mutex &mutex = mutexes[ml.ref];
  assert(0 <= mutex.last_lock);
  if(!prefix[mutex.last_lock].clock.leq(curnode().clock)){
    add_branch(mutex.last_lock,prefix_idx);
  }

  if(0 <= last_full_memory_conflict &&
     !prefix[last_full_memory_conflict].clock.leq(curnode().clock)){
    add_branch(last_full_memory_conflict,prefix_idx);
  }
};

void PSOTraceBuilder::mutex_unlock(const ConstMRef &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.size()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.ref);
    return;
  }
  fence();
  assert(mutexes.count(ml.ref));
  Mutex &mutex = mutexes[ml.ref];
  curnode().may_conflict = true;
  wakeup(Access::W,ml.ref);
  assert(0 <= mutex.last_access);

  see_events({mutex.last_access,last_full_memory_conflict});

  mutex.last_access = prefix_idx;
};

void PSOTraceBuilder::mutex_trylock(const ConstMRef &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.size()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.ref);
    return;
  }
  fence();
  assert(mutexes.count(ml.ref));
  curnode().may_conflict = true;
  wakeup(Access::W,ml.ref);
  Mutex &mutex = mutexes[ml.ref];
  see_events({mutex.last_access,last_full_memory_conflict});

  mutex.last_access = prefix_idx;
  if(mutex.last_lock < 0){ // Mutex is free
    mutex.last_lock = prefix_idx;
  }
};

void PSOTraceBuilder::mutex_init(const ConstMRef &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.size()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.ref);
    return;
  }
  fence();
  assert(mutexes.count(ml.ref) == 0);
  curnode().may_conflict = true;
  mutexes[ml.ref] = Mutex(prefix_idx);
  see_events({last_full_memory_conflict});
};

void PSOTraceBuilder::mutex_destroy(const ConstMRef &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.size()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.ref);
    return;
  }
  fence();
  assert(mutexes.count(ml.ref));
  Mutex &mutex = mutexes[ml.ref];
  curnode().may_conflict = true;
  wakeup(Access::W,ml.ref);

  see_events({mutex.last_access,last_full_memory_conflict});

  mutexes.erase(ml.ref);
};

void PSOTraceBuilder::register_alternatives(int alt_count){
  curnode().may_conflict = true;
  for(int i = curnode().alt+1; i < alt_count; ++i){
    curnode().branch.insert(Branch({curnode().iid.get_pid(),i}));
  }
};

void PSOTraceBuilder::dealloc(const ConstMRef &ml){
  Debug::warn("PSOTB::dealloc")
    << "WARNING: PSOTraceBuilder::dealloc: Should become deprecated.\n";
  // Remove this method entirely when new memory management has been implemented
  for(void const *b : ml){
    mem.erase(b);
    for(unsigned p = 0; p < threads.size(); ++p){
      threads[p].byte_to_aux.erase(b);
      threads[p].sleep_accesses_w.erase(b);
      threads[p].sleep_accesses_r.erase(b);
    }
  }
};

void PSOTraceBuilder::assertion_error(std::string cond){
  IPid pid = curnode().iid.get_pid();
  int idx = curnode().iid.get_index();
  errors.push_back(new AssertionError(IID<CPid>(threads[pid].cpid,idx),cond));
};

void PSOTraceBuilder::pthreads_error(std::string msg){
  IPid pid = curnode().iid.get_pid();
  int idx = curnode().iid.get_index();
  errors.push_back(new PthreadsError(IID<CPid>(threads[pid].cpid,idx),msg));
};

void PSOTraceBuilder::segmentation_fault_error(){
  IPid pid = curnode().iid.get_pid();
  int idx = curnode().iid.get_index();
  errors.push_back(new SegmentationFaultError(IID<CPid>(threads[pid].cpid,idx)));
};

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
};

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
};

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
};

bool PSOTraceBuilder::has_pending_store(IPid pid, void const *ml) const {
  return threads[pid].store_buffers.count(ml);
};

void PSOTraceBuilder::wakeup(Access::Type type, void const *ml){
  IPid pid = curnode().iid.get_pid();
  std::vector<IPid> wakeup; // Wakeup these
  switch(type){
  case Access::W_ALL_MEMORY:
    {
      for(unsigned p = 0; p < threads.size(); ++p){
        if(threads[p].sleep_full_memory_conflict ||
           threads[p].sleep_accesses_w.size()){
          wakeup.push_back(p);
        }else{
          for(void const *b : threads[p].sleep_accesses_r){
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
      for(unsigned p = 0; p < threads.size(); ++p){
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
      for(unsigned p = 0; p < threads.size(); ++p){
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
    curnode().wakeup.insert(p);
  }
};

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
};
