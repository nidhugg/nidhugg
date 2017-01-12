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
#include "TSOTraceBuilder.h"

#include <sstream>
#include <stdexcept>

TSOTraceBuilder::TSOTraceBuilder(const Configuration &conf) : TSOPSOTraceBuilder(conf) {
  threads.push_back(Thread(CPid(),{}));
  threads.push_back(Thread(CPS.new_aux(CPid()),{}));
  threads[1].available = false; // Store buffer is empty.
  prefix_idx = -1;
  dryrun = false;
  replay = false;
  last_full_memory_conflict = -1;
  last_md = 0;
}

TSOTraceBuilder::~TSOTraceBuilder(){
}

bool TSOTraceBuilder::schedule(int *proc, int *aux, int *alt, bool *dryrun){
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
      *proc = pid/2;
      *aux = pid % 2 - 1;
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
      *proc = pid/2;
      *aux = pid % 2 - 1;
      *alt = 0;
      *dryrun = true;
      this->dryrun = true;
      return true;
    }else{
      /* Go to the next event. */
      dry_sleepers = 0;
      ++prefix_idx;
      IPid pid = curnode().iid.get_pid();
      *proc = pid/2;
      *aux = pid % 2 - 1;
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
  unsigned p;
  for(p = 1; p < sz; p += 2){ // Loop through auxiliary threads
    if(threads[p].available && !threads[p].sleeping &&
       (conf.max_search_depth < 0 || threads[p].clock[p] < conf.max_search_depth)){
      ++threads[p].clock[p];
      prefix.push_back(Event(IID<IPid>(IPid(p),threads[p].clock[p]),
                             threads[p].clock));
      *proc = p/2;
      *aux = 0;
      return true;
    }
  }

  for(p = 0; p < sz; p += 2){ // Loop through real threads
    if(threads[p].available && !threads[p].sleeping &&
       (conf.max_search_depth < 0 || threads[p].clock[p] < conf.max_search_depth)){
      ++threads[p].clock[p];
      prefix.push_back(Event(IID<IPid>(IPid(p),threads[p].clock[p]),
                             threads[p].clock));
      *proc = p/2;
      *aux = -1;
      return true;
    }
  }

  return false; // No available threads
}

void TSOTraceBuilder::refuse_schedule(){
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
  mark_unavailable(last_pid/2,last_pid % 2 - 1);
}

void TSOTraceBuilder::mark_available(int proc, int aux){
  threads[ipid(proc,aux)].available = true;
}

void TSOTraceBuilder::mark_unavailable(int proc, int aux){
  threads[ipid(proc,aux)].available = false;
}

void TSOTraceBuilder::metadata(const llvm::MDNode *md){
  if(!dryrun && curnode().md == 0){
    curnode().md = md;
  }
  last_md = md;
}

bool TSOTraceBuilder::sleepset_is_empty() const{
  for(unsigned i = 0; i < threads.size(); ++i){
    if(threads[i].sleeping) return false;
  }
  return true;
}

bool TSOTraceBuilder::check_for_cycles(){
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

Trace *TSOTraceBuilder::get_trace() const{
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
  Trace *t = new IIDSeqTrace(cmp,cmp_md,errs);
  t->set_blocked(!sleepset_is_empty());
  return t;
}

bool TSOTraceBuilder::reset(){
  if(conf.debug_print_on_reset){
    llvm::dbgs() << " === TSOTraceBuilder reset ===\n";
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
  threads.push_back(Thread(CPid(),{}));
  threads.push_back(Thread(CPS.new_aux(CPid()),{}));
  threads[1].available = false; // Store buffer is empty.
  mutexes.clear();
  cond_vars.clear();
  mem.clear();
  last_full_memory_conflict = -1;
  prefix_idx = -1;
  dryrun = false;
  replay = true;
  dry_sleepers = 0;
  last_md = 0;

  return true;
}

IID<CPid> TSOTraceBuilder::get_iid() const{
  IPid pid = curnode().iid.get_pid();
  int idx = curnode().iid.get_index();
  return IID<CPid>(threads[pid].cpid,idx);
}

static std::string rpad(std::string s, int n){
  while(int(s.size()) < n) s += " ";
  return s;
}

std::string TSOTraceBuilder::iid_string(const Event &evt) const{
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

void TSOTraceBuilder::debug_print() const {
  llvm::dbgs() << "TSOTraceBuilder (debug print):\n";
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

void TSOTraceBuilder::spawn(){
  IPid parent_ipid = curnode().iid.get_pid();
  CPid child_cpid = CPS.spawn(threads[parent_ipid].cpid);
  threads.push_back(Thread(child_cpid,threads[parent_ipid].clock));
  threads.push_back(Thread(CPS.new_aux(child_cpid),threads[parent_ipid].clock));
  threads.back().available = false; // Empty store buffer
}

void TSOTraceBuilder::store(const ConstMRef &ml){
  if(dryrun) return;
  IPid ipid = curnode().iid.get_pid();
  threads[ipid].store_buffer.push_back(PendingStore(ml,threads[ipid].clock,last_md));
  threads[ipid+1].available = true;
}

void TSOTraceBuilder::atomic_store(const ConstMRef &ml){
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
  bool is_update = ipid % 2;

  IPid uipid = ipid; // ID of the thread changing the memory
  IPid tipid = is_update ? ipid-1 : ipid; // ID of the (real) thread that issued the store

  if(is_update){ // Add the clock of the store instruction
    assert(threads[tipid].store_buffer.size());
    const PendingStore &pst = threads[tipid].store_buffer.front();
    curnode().clock += pst.clock;
    threads[uipid].clock += pst.clock;
    curnode().origin_iid = IID<IPid>(tipid,pst.clock[tipid]);
    curnode().md = pst.md;
  }else{ // Add the clock of the auxiliary thread (because of fence semantics)
    assert(threads[tipid].store_buffer.empty());
    threads[tipid].clock += threads[tipid+1].clock;
    curnode().clock += threads[tipid+1].clock;
  }

  VecSet<int> seen_accesses;

  /* See previous updates reads to ml */
  for(void const *b : ml){
    ByteInfo &bi = mem[b];
    int lu = bi.last_update;
    assert(lu < int(prefix.size()));
    if(0 <= lu){
      IPid lu_tipid = 2*(prefix[lu].iid.get_pid() / 2);
      if(lu_tipid != tipid){
        seen_accesses.insert(bi.last_update);
      }
    }
    for(int i : bi.last_read){
      if(0 <= i && prefix[i].iid.get_pid() != tipid) seen_accesses.insert(i);
    }
  }

  seen_accesses.insert(last_full_memory_conflict);

  see_events(seen_accesses);

  /* Register in memory */
  for(void const *b : ml){
    ByteInfo &bi = mem[b];
    bi.last_update = prefix_idx;
    bi.last_update_ml = ml;
    if(is_update && threads[tipid].store_buffer.front().last_rowe >= 0){
      bi.last_read[tipid/2] = threads[tipid].store_buffer.front().last_rowe;
    }
    wakeup(Access::W,b);
  }

  if(is_update){ /* Remove pending store from buffer */
    for(unsigned i = 0; i < threads[tipid].store_buffer.size()-1; ++i){
      threads[tipid].store_buffer[i] = threads[tipid].store_buffer[i+1];
    }
    threads[tipid].store_buffer.pop_back();
    if(threads[tipid].store_buffer.empty()){
      threads[uipid].available = false;
    }
  }
}

void TSOTraceBuilder::load(const ConstMRef &ml){
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
  for(int i = int(threads[ipid].store_buffer.size())-1; 0 <= i; --i){
    if(threads[ipid].store_buffer[i].ml.ref == ml.ref){
      /* ROWE */
      threads[ipid].store_buffer[i].last_rowe = prefix_idx;
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
      IPid lu_tipid = 2*(prefix[lu].iid.get_pid() / 2);
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
    mem[b].last_read[ipid/2] = prefix_idx;
    wakeup(Access::R,b);
  }
}

void TSOTraceBuilder::full_memory_conflict(){
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
}

void TSOTraceBuilder::fence(){
  if(dryrun) return;
  IPid ipid = curnode().iid.get_pid();
  assert(ipid % 2 == 0);
  assert(threads[ipid].store_buffer.empty());
  curnode().clock += threads[ipid+1].clock;
  threads[ipid].clock += threads[ipid+1].clock;
}

void TSOTraceBuilder::join(int tgt_proc){
  if(dryrun) return;
  IPid ipid = curnode().iid.get_pid();
  curnode().clock += threads[tgt_proc*2].clock;
  threads[ipid].clock += threads[tgt_proc*2].clock;
  curnode().clock += threads[tgt_proc*2+1].clock;
  threads[ipid].clock += threads[tgt_proc*2+1].clock;
}

void TSOTraceBuilder::mutex_lock(const ConstMRef &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.size()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.ref);
    return;
  }
  fence();
  if(!conf.mutex_require_init && !mutexes.count(ml.ref)){
    // Assume static initialization
    mutexes[ml.ref] = Mutex();
  }
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
}

void TSOTraceBuilder::mutex_lock_fail(const ConstMRef &ml){
  assert(!dryrun);
  if(!conf.mutex_require_init && !mutexes.count(ml.ref)){
    // Assume static initialization
    mutexes[ml.ref] = Mutex();
  }
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
}

void TSOTraceBuilder::mutex_trylock(const ConstMRef &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.size()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.ref);
    return;
  }
  fence();
  if(!conf.mutex_require_init && !mutexes.count(ml.ref)){
    // Assume static initialization
    mutexes[ml.ref] = Mutex();
  }
  assert(mutexes.count(ml.ref));
  curnode().may_conflict = true;
  wakeup(Access::W,ml.ref);
  Mutex &mutex = mutexes[ml.ref];
  see_events({mutex.last_access,last_full_memory_conflict});

  mutex.last_access = prefix_idx;
  if(mutex.last_lock < 0){ // Mutex is free
    mutex.last_lock = prefix_idx;
  }
}

void TSOTraceBuilder::mutex_unlock(const ConstMRef &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.size()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.ref);
    return;
  }
  fence();
  if(!conf.mutex_require_init && !mutexes.count(ml.ref)){
    // Assume static initialization
    mutexes[ml.ref] = Mutex();
  }
  assert(mutexes.count(ml.ref));
  Mutex &mutex = mutexes[ml.ref];
  curnode().may_conflict = true;
  wakeup(Access::W,ml.ref);
  assert(0 <= mutex.last_access);

  see_events({mutex.last_access,last_full_memory_conflict});

  mutex.last_access = prefix_idx;
}

void TSOTraceBuilder::mutex_init(const ConstMRef &ml){
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
}

void TSOTraceBuilder::mutex_destroy(const ConstMRef &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.size()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.ref);
    return;
  }
  fence();
  if(!conf.mutex_require_init && !mutexes.count(ml.ref)){
    // Assume static initialization
    mutexes[ml.ref] = Mutex();
  }
  assert(mutexes.count(ml.ref));
  Mutex &mutex = mutexes[ml.ref];
  curnode().may_conflict = true;
  wakeup(Access::W,ml.ref);

  see_events({mutex.last_access,last_full_memory_conflict});

  mutexes.erase(ml.ref);
}

bool TSOTraceBuilder::cond_init(const ConstMRef &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.size()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.ref);
    return true;
  }
  fence();
  if(cond_vars.count(ml.ref)){
    pthreads_error("Condition variable initiated twice.");
    return false;
  }
  curnode().may_conflict = true;
  cond_vars[ml.ref] = CondVar(prefix_idx);
  see_events({last_full_memory_conflict});
  return true;
}

bool TSOTraceBuilder::cond_signal(const ConstMRef &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.size()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.ref);
    return true;
  }
  fence();
  curnode().may_conflict = true;
  wakeup(Access::W,ml.ref);

  auto it = cond_vars.find(ml.ref);
  if(it == cond_vars.end()){
    pthreads_error("cond_signal called with uninitialized condition variable.");
    return false;
  }
  CondVar &cond_var = it->second;
  VecSet<int> seen_events = {last_full_memory_conflict};
  if(curnode().alt < int(cond_var.waiters.size())-1){
    assert(curnode().alt == 0);
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
    threads[ipid].available = true;
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

bool TSOTraceBuilder::cond_broadcast(const ConstMRef &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.size()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.ref);
    return true;
  }
  fence();
  curnode().may_conflict = true;
  wakeup(Access::W,ml.ref);

  auto it = cond_vars.find(ml.ref);
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
    threads[ipid].available = true;
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

bool TSOTraceBuilder::cond_wait(const ConstMRef &cond_ml, const ConstMRef &mutex_ml){
  {
    auto it = mutexes.find(mutex_ml.ref);
    if(!dryrun && it == mutexes.end()){
      if(conf.mutex_require_init){
        pthreads_error("cond_wait called with uninitialized mutex object.");
      }else{
        pthreads_error("cond_wait called with unlocked mutex object.");
      }
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
    threads[pid].sleep_accesses_r.insert(cond_ml.ref);
    return true;
  }
  fence();
  curnode().may_conflict = true;
  wakeup(Access::R,cond_ml.ref);

  IPid pid = curnode().iid.get_pid();

  auto it = cond_vars.find(cond_ml.ref);
  if(it == cond_vars.end()){
    pthreads_error("cond_wait called with uninitialized condition variable.");
    return false;
  }
  it->second.waiters.push_back(prefix_idx);
  threads[pid].available = false;

  see_events({last_full_memory_conflict,it->second.last_signal});

  return true;
}

int TSOTraceBuilder::cond_destroy(const ConstMRef &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.size()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.ref);
    return 0;
  }
  fence();

  int err = (EBUSY == 1) ? 2 : 1; // Chose an error value different from EBUSY

  curnode().may_conflict = true;
  wakeup(Access::W,ml.ref);

  auto it = cond_vars.find(ml.ref);
  if(it == cond_vars.end()){
    pthreads_error("cond_destroy called on uninitialized condition variable.");
    return err;
  }
  CondVar &cond_var = it->second;
  VecSet<int> seen_events = {cond_var.last_signal,last_full_memory_conflict};
  for(int i : cond_var.waiters) seen_events.insert(i);
  see_events(seen_events);

  int rv = cond_var.waiters.size() ? EBUSY : 0;
  cond_vars.erase(ml.ref);
  return rv;
}

void TSOTraceBuilder::register_alternatives(int alt_count){
  curnode().may_conflict = true;
  for(int i = curnode().alt+1; i < alt_count; ++i){
    curnode().branch.insert(Branch({curnode().iid.get_pid(),i}));
  }
}

VecSet<TSOTraceBuilder::IPid> TSOTraceBuilder::sleep_set_at(int i){
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

void TSOTraceBuilder::see_events(const VecSet<int> &seen_accesses){
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

void TSOTraceBuilder::add_branch(int i, int j){
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

bool TSOTraceBuilder::has_pending_store(IPid pid, void const *ml) const {
  const std::vector<PendingStore> &sb = threads[pid].store_buffer;
  for(unsigned i = 0; i < sb.size(); ++i){
    if(sb[i].ml.includes(ml)){
      return true;
    }
  }
  return false;
}

void TSOTraceBuilder::wakeup(Access::Type type, void const *ml){
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
           (int(p) != pid+1 &&
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
           (int(p) + 1 != pid &&
            (threads[p].sleep_accesses_w.count(ml) ||
             (threads[p].sleep_accesses_r.count(ml) &&
              !has_pending_store(p,ml))))){
          wakeup.push_back(p);
        }
      }
      break;
    }
  default:
    throw std::logic_error("TSOTraceBuilder::wakeup: Unknown type of memory access.");
  }

  for(IPid p : wakeup){
    assert(threads[p].sleeping);
    threads[p].sleep_accesses_r.clear();
    threads[p].sleep_accesses_w.clear();
    threads[p].sleep_full_memory_conflict = false;
    threads[p].sleeping = false;
    curnode().wakeup.insert(p);
  }
}

bool TSOTraceBuilder::has_cycle(IID<IPid> *loc) const{
  int proc_count = threads.size();
  int pfx_size = prefix.size();

  /* Identify all store events */
  struct stupd_t{
    /* The index part of the IID identifying a store event. */
    int store;
    /* The index in prefix of the corresponding update event. */
    int update;
  };
  /* stores[proc] is all store events of process proc, ordered by
   * store index.
   */
  std::vector<std::vector<stupd_t> > stores(proc_count);
  for(int i = 0; i < pfx_size; ++i){
    if(prefix[i].iid.get_pid() % 2){ // Update
      assert(prefix[i].origin_iid.get_pid() == prefix[i].iid.get_pid()-1);
      stores[prefix[i].iid.get_pid() / 2].push_back({prefix[i].origin_iid.get_index(),i});
    }
  }

  /* Attempt to replay computation under SC */
  struct proc_t {
    proc_t()
      : pc(0), pfx_index(0), store_index(0), blocked(false), block_clock() {};
    int pc; // Current program counter
    int pfx_index; // Index into prefix
    int store_index; // Index into stores
    bool blocked; // Is the process currently blocked?
    VClock<IPid> block_clock; // If blocked, what are we waiting for?
  };
  std::vector<proc_t> procs(proc_count);

  int proc = 0; // The next scheduled process
  /* alive keeps track of whether any process has been successfully
   * scheduled lately
   */
  bool alive = false;
  while(true){
    // Advance pfx_index to the right Event in prefix
    while(procs[proc].pfx_index < pfx_size &&
          prefix[procs[proc].pfx_index].iid.get_pid() != proc*2){
      ++procs[proc].pfx_index;
    }
    if(pfx_size <= procs[proc].pfx_index){
      // This process is finished
      proc = (proc+1)%proc_count;
      if(proc == 0){
        if(!alive) break;
        alive = false;
      }
      continue;
    }

    int next_pc = procs[proc].pc+1;
    const Event &evt = prefix[procs[proc].pfx_index];

    if(!procs[proc].blocked){
      assert(evt.iid.get_pid() == 2*proc);
      assert(evt.iid.get_index() <= next_pc);
      assert(next_pc < evt.iid.get_index() + evt.size);
      procs[proc].block_clock = evt.clock;
      assert(procs[proc].block_clock[proc*2] <= next_pc);
      procs[proc].block_clock[proc*2] = next_pc;
      if(procs[proc].store_index < int(stores[proc].size()) &&
         stores[proc][procs[proc].store_index].store == next_pc){
        // This is a store. Also consider the update's clock.
        procs[proc].block_clock += prefix[stores[proc][procs[proc].store_index].update].clock;
        ++procs[proc].store_index;
      }
    }

    // Is this process blocked?
    // Is there some process we have to wait for?
    {
      int i;
      procs[proc].blocked = false;
      for(i = 0; i < proc_count; ++i){
        if(i != proc && procs[i].pc < procs[proc].block_clock[i*2]){
          procs[proc].blocked = true;
          break;
        }
      }
    }

    // Are we still blocked?
    if(procs[proc].blocked){
      proc = (proc+1)%proc_count; // Try another process
      if(proc == 0){
        if(!alive) break;
        alive = false;
      }
    }else{
      alive = true;
      procs[proc].pc = next_pc;
      assert(next_pc == procs[proc].block_clock[proc*2]);

      // Advance pc to next interesting event
      next_pc = evt.iid.get_index() + evt.size - 1;
      if(procs[proc].store_index < int(stores[proc].size()) &&
         stores[proc][procs[proc].store_index].store-1 < next_pc){
        next_pc = stores[proc][procs[proc].store_index].store-1;
      }
      assert(procs[proc].pc <= next_pc);
      procs[proc].pc = next_pc;

      if(next_pc + 1 == evt.iid.get_index() + evt.size){
        // We are done with this Event
        ++procs[proc].pfx_index;
      }
    }
  }

  // Did all processes finish, or are some still blocked?
  {
    int upd_idx = -1; // Index of the latest update involved in a cycle
    bool has_cycle = false;
    for(int i = 0; i < proc_count; ++i){
      if(procs[i].blocked){
        // There is a cycle
        has_cycle = true;
        int next_pc = procs[i].pc+1;
        if(0 < procs[i].store_index && stores[i][procs[i].store_index-1].store == next_pc){
          if(stores[i][procs[i].store_index-1].update > upd_idx){
            upd_idx = stores[i][procs[i].store_index-1].update;
            *loc = prefix[upd_idx].iid;
          }
        }
      }
    }
    assert(!has_cycle || 0 <= upd_idx);
    return has_cycle;
  }
}

int TSOTraceBuilder::estimate_trace_count() const{
  return estimate_trace_count(0);
}

int TSOTraceBuilder::estimate_trace_count(int idx) const{
  if(idx > int(prefix.size())) return 0;
  if(idx == int(prefix.size())) return 1;

  int count = 1;
  for(int i = int(prefix.size())-1; idx <= i; --i){
    count += prefix[i].sleep_branch_trace_count;
    count += prefix[i].branch.size()*(count / (1 + prefix[i].sleep.size()));
  }

  return count;
}
