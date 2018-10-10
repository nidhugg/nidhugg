/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
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
#include "WSCTraceBuilder.h"
#include "SaturatedGraph.h"
#include "PrefixHeuristic.h"
#include "Timing.h"

#include <sstream>
#include <stdexcept>

#define ANSIRed "\x1b[91m"
#define ANSIRst "\x1b[m"


static Timing::Context analysis_context("analysis");
static Timing::Context vclocks_context("vclocks");
static Timing::Context unfolding_context("unfolding");
static Timing::Context neighbours_context("neighbours");
static Timing::Context graph_context("graph");
static Timing::Context sat_context("sat");

WSCTraceBuilder::WSCTraceBuilder(const Configuration &conf) : TSOPSOTraceBuilder(conf) {
  threads.push_back(Thread(CPid(), -1));
  prefix_idx = -1;
  replay = false;
  last_full_memory_conflict = -1;
  last_md = 0;
  replay_point = 0;
}

WSCTraceBuilder::~WSCTraceBuilder(){
}

bool WSCTraceBuilder::schedule(int *proc, int *aux, int *alt, bool *dryrun){
  *dryrun = false;
  *alt = 0;
  *aux = -1; /* No auxilliary threads in SC */
  if(replay){
    /* Are we done with the current Event? */
    if (0 <= prefix_idx && threads[curev().iid.get_pid()].last_event_index() <
        curev().iid.get_index() + curev().size - 1) {
      /* Continue executing the current Event */
      IPid pid = curev().iid.get_pid();
      *proc = pid;
      *alt = 0;
      if(!(threads[pid].available)) {
        llvm::dbgs() << "Trying to play process " << threads[pid].cpid << ", but it is blocked\n";
        llvm::dbgs() << "At replay step " << prefix_idx << ", iid "
                     << iid_string(IID<IPid>(pid, threads[curev().iid.get_pid()].last_event_index()))
                     << "\n";
        abort();
      }
      threads[pid].event_indices.push_back(prefix_idx);
      return true;
    } else if(prefix_idx + 1 == int(prefix.size())) {
      /* We are done replaying. Continue below... */
      assert(prefix_idx < 0 || curev().sym.size() == sym_idx
             || (errors.size() && errors.back()->get_location()
                 == IID<CPid>(threads[curev().iid.get_pid()].cpid,
                              curev().iid.get_index())));
      replay = false;
    } else {
      /* Go to the next event. */
      assert(prefix_idx < 0 || curev().sym.size() == sym_idx
             || (errors.size() && errors.back()->get_location()
                 == IID<CPid>(threads[curev().iid.get_pid()].cpid,
                              curev().iid.get_index())));
      sym_idx = 0;
      ++prefix_idx;
      IPid pid = curev().iid.get_pid();
      *proc = pid;
      *alt = curev().alt;
      assert(threads[pid].available);
      threads[pid].event_indices.push_back(prefix_idx);
      return true;
    }
  }

  assert(!replay);
  /* Create a new Event */

  // TEMP: Until we have a SymEv for store()
  // assert(prefix_idx < 0 || !!curev().sym.size() == curev().may_conflict);

  /* Should we merge the last two events? */
  if(prefix.size() > 1 &&
     prefix[prefix.size()-1].iid.get_pid()
     == prefix[prefix.size()-2].iid.get_pid() &&
     !prefix[prefix.size()-1].may_conflict){
    assert(curev().sym.empty()); /* Would need to be copied */
    assert(curev().sym.empty()); /* Can't happen */
    prefix.pop_back();
    --prefix_idx;
    ++curev().size;
    assert(int(threads[curev().iid.get_pid()].event_indices.back()) == prefix_idx + 1);
    threads[curev().iid.get_pid()].event_indices.back() = prefix_idx;
  }

  /* Create a new Event */
  sym_idx = 0;
  ++prefix_idx;
  assert(prefix_idx == int(prefix.size()));

  /* Find an available thread. */
  const unsigned sz = threads.size();
  for(unsigned p = 0; p < sz; ++p){ // Loop through real threads
    if(threads[p].available &&
       (conf.max_search_depth < 0 || threads[p].last_event_index() < conf.max_search_depth)){
      threads[p].event_indices.push_back(prefix_idx);
      prefix.emplace_back(IID<IPid>(IPid(p),threads[p].last_event_index()));
      *proc = p;
      return true;
    }
  }

  compute_prefixes();

  return false; // No available threads
}

void WSCTraceBuilder::refuse_schedule(){
  assert(prefix_idx == int(prefix.size())-1);
  assert(curev().size == 1);
  assert(!curev().may_conflict);
  IPid last_pid = curev().iid.get_pid();
  prefix.pop_back();
  assert(int(threads[last_pid].event_indices.back()) == prefix_idx);
  threads[last_pid].event_indices.pop_back();
  --prefix_idx;
  mark_unavailable(last_pid);
}

void WSCTraceBuilder::mark_available(int proc, int aux){
  threads[ipid(proc,aux)].available = true;
}

void WSCTraceBuilder::mark_unavailable(int proc, int aux){
  threads[ipid(proc,aux)].available = false;
}

bool WSCTraceBuilder::is_replaying() const {
  return prefix_idx < replay_point;
}

void WSCTraceBuilder::cancel_replay(){
  abort(); /* What is this function used for? */
  if(!replay) return;
  replay = false;
  /* XXX: Reset won't work right if we delete some prefix event
   * corresponding to a decision node that will not be popped on reset.
   */
  while (prefix_idx + 1 < int(prefix.size())) prefix.pop_back();
}

void WSCTraceBuilder::metadata(const llvm::MDNode *md){
  if(curev().md == 0){
    curev().md = md;
  }
  last_md = md;
}

bool WSCTraceBuilder::sleepset_is_empty() const{
  return true;
}

Trace *WSCTraceBuilder::get_trace() const{
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

bool WSCTraceBuilder::reset(){
  for(; !decisions.empty(); decisions.pop_back()) {
    auto &siblings = decisions.back().siblings;
    for (auto it = siblings.begin(); it != siblings.end();) {
      if (it->second.is_bottom()) {
        decisions.back().sleep.emplace(std::move(it->first));
        it = siblings.erase(it);
      } else {
        ++it;
      }
    }
    if(!siblings.empty()){
      break;
    }
  }

  if(decisions.empty()){
    /* No more branching is possible. */
    return false;
  }

  /* Insert current event in sleep */
  for (unsigned i = 0;; ++i) {
    if (prefix[i].decision == int(decisions.size()-1)) {
      assert(!prefix[i].pinned);
      decisions.back().sleep.emplace(prefix[i].event->read_from);
      break;
    }
    assert(i < prefix.size());
  }
  auto sit = decisions.back().siblings.begin();
  Leaf l = std::move(sit->second);

  if (conf.debug_print_on_reset)
      llvm::dbgs() << "Backtracking to decision node " << (decisions.size()-1)
                   << ", replaying " << l.prefix.size() << " events to read from "
                   << (sit->first ? std::to_string(sit->first->seqno) : "init") << "\n";
  decisions.back().siblings.erase(sit);

  assert(!l.is_bottom());

  replay_point = l.prefix.size();

  std::vector<Event> new_prefix;
  new_prefix.reserve(l.prefix.size());
  std::vector<int> iid_map = iid_map_at(0);
  for (Branch &b : l.prefix) {
    int index = iid_map[b.pid];
    IID<IPid> iid(b.pid, index);
    new_prefix.emplace_back(iid);
    new_prefix.back().size = b.size;
    new_prefix.back().sym = std::move(b.sym);
    new_prefix.back().pinned = b.pinned;
    new_prefix.back().decision = b.decision;
    iid_map_step(iid_map, new_prefix.back());
  }

#ifndef NDEBUG
  for (int d = 0; d < int(decisions.size()); ++d) {
    assert(std::any_of(new_prefix.begin(), new_prefix.end(),
                       [&](const Event &e) { return e.decision == d; }));
  }
#endif

  prefix = std::move(new_prefix);

  CPS = CPidSystem();
  threads.clear();
  threads.push_back(Thread(CPid(),-1));
  mutexes.clear();
  cond_vars.clear();
  mem.clear();
  last_full_memory_conflict = -1;
  prefix_idx = -1;
  replay = true;
  last_md = 0;
  reset_cond_branch_log();

  return true;
}

IID<CPid> WSCTraceBuilder::get_iid() const{
  IPid pid = curev().iid.get_pid();
  int idx = curev().iid.get_index();
  return IID<CPid>(threads[pid].cpid,idx);
}

static std::string rpad(std::string s, int n){
  while(int(s.size()) < n) s += " ";
  return s;
}

static std::string lpad(const std::string &s, int n){
  return std::string(std::max(0, n - int(s.size())), ' ') + s;
}

std::string WSCTraceBuilder::iid_string(std::size_t pos) const{
  return iid_string(prefix[pos]);
}

std::string WSCTraceBuilder::iid_string(const Event &event) const{
  IPid pid = event.iid.get_pid();
  int index = event.iid.get_index();
  std::stringstream ss;
  ss << "(" << threads[pid].cpid << "," << index;
  if(event.size > 1){
    ss << "-" << index + event.size - 1;
  }
  ss << ")";
  if(event.alt != 0){
    ss << "-alt:" << event.alt;
  }
  return ss.str();
}

static std::string events_to_string(const llvm::SmallVectorImpl<SymEv> &e) {
  if (e.size() == 0) return "None()";
  std::string res;
  for (unsigned i = 0; i < e.size(); ++i) {
    res += e[i].to_string();
    if (i != e.size()-1) res += ", ";
  }
  return res;
}

void WSCTraceBuilder::debug_print() const {
  llvm::dbgs() << "WSCTraceBuilder (debug print, replay until " << replay_point << "):\n";
  int idx_offs = 0;
  int iid_offs = 0;
  int dec_offs = 0;
  int unf_offs = 0;
  int rf_offs = 0;
  int clock_offs = 0;
  std::vector<std::string> lines(prefix.size(), "");

  for(unsigned i = 0; i < prefix.size(); ++i){
    IPid ipid = prefix[i].iid.get_pid();
    idx_offs = std::max(idx_offs,int(std::to_string(i).size()));
    iid_offs = std::max(iid_offs,2*ipid+int(iid_string(i).size()));
    dec_offs = std::max(dec_offs,int(std::to_string(prefix[i].decision).size())
                        + (prefix[i].pinned ? 1 : 0));
    unf_offs = std::max(unf_offs,int(std::to_string(prefix[i].event->seqno).size()));
    rf_offs = std::max(rf_offs,prefix[i].read_from ?
                       int(std::to_string(*prefix[i].read_from).size())
                       : 1);
    clock_offs = std::max(clock_offs,int(prefix[i].clock.to_string().size()));
    lines[i] = events_to_string(prefix[i].sym);
  }

  for(unsigned i = 0; i < prefix.size(); ++i){
    IPid ipid = prefix[i].iid.get_pid();
    llvm::dbgs() << " " << lpad(std::to_string(i),idx_offs)
                 << rpad("",1+ipid*2)
                 << rpad(iid_string(i),iid_offs-ipid*2)
                 << " " << lpad((prefix[i].pinned ? "*" : "")
                                + std::to_string(prefix[i].decision),dec_offs)
                 << " " << lpad(std::to_string(prefix[i].event->seqno),unf_offs)
                 << " " << lpad(prefix[i].read_from ? std::to_string(*prefix[i].read_from) : "-",rf_offs)
                 << " " << rpad(prefix[i].clock.to_string(),clock_offs)
                 << " " << lines[i] << "\n";
  }
  for (unsigned i = prefix.size(); i < lines.size(); ++i){
    llvm::dbgs() << std::string(2+iid_offs + 1+clock_offs, ' ') << lines[i] << "\n";
  }
  if(errors.size()){
    llvm::dbgs() << "Errors:\n";
    for(unsigned i = 0; i < errors.size(); ++i){
      llvm::dbgs() << "  Error #" << i+1 << ": "
                   << errors[i]->to_string() << "\n";
    }
  }
}

void WSCTraceBuilder::spawn(){
  IPid parent_ipid = curev().iid.get_pid();
  CPid child_cpid = CPS.spawn(threads[parent_ipid].cpid);
  threads.push_back(Thread(child_cpid,prefix_idx));
  curev().may_conflict = true;
  record_symbolic(SymEv::Spawn(threads.size() - 1));
}

void WSCTraceBuilder::store(const SymData &sd){
  assert(false && "Cannot happen");
  abort();
}

void WSCTraceBuilder::atomic_store(const SymData &sd){
  if (conf.observers)
    record_symbolic(SymEv::UnobsStore(sd));
  else
    record_symbolic(SymEv::Store(sd));
  do_atomic_store(sd);
}

void WSCTraceBuilder::do_atomic_store(const SymData &sd){
  const SymAddrSize &ml = sd.get_ref();
  IPid ipid = curev().iid.get_pid();
  curev().may_conflict = true;

  VecSet<int> seen_accesses;

  /* See previous updates reads to ml */
  for(SymAddr b : ml){
    ByteInfo &bi = mem[b];
    int lu = bi.last_update;
    assert(lu < int(prefix.size()));
    if(0 <= lu){
      IPid lu_tipid = prefix[lu].iid.get_pid();
      if(lu_tipid != ipid){
        seen_accesses.insert(bi.last_update);
      }
    }
    for(int i : bi.last_read){
      if(0 <= i && prefix[i].iid.get_pid() != ipid) seen_accesses.insert(i);
    }

    /* Register in memory */
    bi.last_update = prefix_idx;
    bi.last_update_ml = ml;
  }

  seen_accesses.insert(last_full_memory_conflict);

  see_events(seen_accesses);
}

void WSCTraceBuilder::load(const SymAddrSize &ml){
  record_symbolic(SymEv::Load(ml));
  do_load(ml);
}

void WSCTraceBuilder::do_load(const SymAddrSize &ml){
  curev().may_conflict = true;
  IPid ipid = curev().iid.get_pid();

  VecSet<int> seen_accesses;

  /* See all updates to the read bytes. */
  for(SymAddr b : ml){
    int lu = mem[b].last_update;
    const SymAddrSize &lu_ml = mem[b].last_update_ml;
    if(0 <= lu){
      IPid lu_tipid = prefix[lu].iid.get_pid() & ~0x1;
      if(lu_tipid == ipid && ml != lu_ml && lu != prefix_idx){
        add_happens_after(prefix_idx, lu);
      }
    }
    do_load(mem[b]);

    /* Register load in memory */
    mem[b].last_read[ipid] = prefix_idx;
  }

  seen_accesses.insert(last_full_memory_conflict);

  see_events(seen_accesses);
}

void WSCTraceBuilder::compare_exchange
(const SymData &sd, const SymData::block_type expected, bool success){
  if(success){
    record_symbolic(SymEv::CmpXhg(sd, expected));
    do_load(sd.get_ref());
    do_atomic_store(sd);
  }else{
    record_symbolic(SymEv::CmpXhgFail(sd, expected));
    do_load(sd.get_ref());
  }
}

void WSCTraceBuilder::full_memory_conflict(){
  record_symbolic(SymEv::Fullmem());
  curev().may_conflict = true;

  /* See all pervious memory accesses */
  VecSet<int> seen_accesses;
  for(auto it = mem.begin(); it != mem.end(); ++it){
    do_load(it->second);
    for(int i : it->second.last_read){
      seen_accesses.insert(i);
    }
  }
  for(auto it = mutexes.begin(); it != mutexes.end(); ++it){
    seen_accesses.insert(it->second.last_access);
  }
  seen_accesses.insert(last_full_memory_conflict);
  last_full_memory_conflict = prefix_idx;

  /* No later access can have a conflict with any earlier access */
  mem.clear();

  see_events(seen_accesses);
}

void WSCTraceBuilder::do_load(ByteInfo &m){
  IPid ipid = curev().iid.get_pid();
  VecSet<int> seen_accesses;
  VecSet<std::pair<int,int>> seen_pairs;

  int lu = m.last_update;
  curev().read_from = lu;
  if(0 <= lu){
    IPid lu_tipid = prefix[lu].iid.get_pid() & ~0x1;
    if(lu_tipid != ipid){
      seen_accesses.insert(lu);
    }
  }
  see_events(seen_accesses);
  see_event_pairs(seen_pairs);
}

void WSCTraceBuilder::fence(){
}

void WSCTraceBuilder::join(int tgt_proc){
  record_symbolic(SymEv::Join(tgt_proc));
  curev().may_conflict = true;
  add_happens_after_thread(prefix_idx, tgt_proc);
}

void WSCTraceBuilder::mutex_lock(const SymAddrSize &ml){
  record_symbolic(SymEv::MLock(ml));
  fence();
  curev().may_conflict = true;

  assert(!conf.mutex_require_init || mutexes.count(ml.addr));
  Mutex &mutex = mutexes[ml.addr];

  if(mutex.last_lock < 0){
    /* No previous lock */
    see_events({mutex.last_access,last_full_memory_conflict});
  }else{
    add_lock_suc_race(mutex.last_lock, mutex.last_access);
    see_events({last_full_memory_conflict});
  }

  mutex.last_lock = mutex.last_access = prefix_idx;
  mutex.locked = true;
}

void WSCTraceBuilder::mutex_lock_fail(const SymAddrSize &ml){
  assert(!conf.mutex_require_init || mutexes.count(ml.addr));
  Mutex &mutex = mutexes[ml.addr];
  assert(0 <= mutex.last_lock);
  add_lock_fail_race(mutex, mutex.last_lock);

  if(0 <= last_full_memory_conflict){
    add_lock_fail_race(mutex, last_full_memory_conflict);
  }
}

void WSCTraceBuilder::mutex_trylock(const SymAddrSize &ml){
  assert(!conf.mutex_require_init || mutexes.count(ml.addr));
  Mutex &mutex = mutexes[ml.addr];
  record_symbolic(mutex.locked ? SymEv::MTryLockFail(ml) : SymEv::MTryLock(ml));
  fence();
  curev().may_conflict = true;
  see_events({mutex.last_access,last_full_memory_conflict});

  mutex.last_access = prefix_idx;
  if(!mutex.locked){ // Mutex is free
    mutex.last_lock = prefix_idx;
    mutex.locked = true;
  }
}

void WSCTraceBuilder::mutex_unlock(const SymAddrSize &ml){
  record_symbolic(SymEv::MUnlock(ml));
  fence();
  assert(!conf.mutex_require_init || mutexes.count(ml.addr));
  Mutex &mutex = mutexes[ml.addr];
  curev().may_conflict = true;
  assert(0 <= mutex.last_access);

  see_events({mutex.last_access,last_full_memory_conflict});

  mutex.last_access = prefix_idx;
  mutex.locked = false;
}

void WSCTraceBuilder::mutex_init(const SymAddrSize &ml){
  record_symbolic(SymEv::MInit(ml));
  fence();
  assert(mutexes.count(ml.addr) == 0);
  curev().may_conflict = true;
  mutexes[ml.addr] = Mutex(prefix_idx);
  see_events({last_full_memory_conflict});
}

void WSCTraceBuilder::mutex_destroy(const SymAddrSize &ml){
  record_symbolic(SymEv::MDelete(ml));
  fence();
  if(!conf.mutex_require_init && !mutexes.count(ml.addr)){
    // Assume static initialization
    mutexes[ml.addr] = Mutex();
  }
  assert(mutexes.count(ml.addr));
  Mutex &mutex = mutexes[ml.addr];
  curev().may_conflict = true;

  see_events({mutex.last_access,last_full_memory_conflict});

  mutexes.erase(ml.addr);
}

bool WSCTraceBuilder::cond_init(const SymAddrSize &ml){
  record_symbolic(SymEv::CInit(ml));
  fence();
  if(cond_vars.count(ml.addr)){
    pthreads_error("Condition variable initiated twice.");
    return false;
  }
  curev().may_conflict = true;
  cond_vars[ml.addr] = CondVar(prefix_idx);
  see_events({last_full_memory_conflict});
  return true;
}

bool WSCTraceBuilder::cond_signal(const SymAddrSize &ml){
  record_symbolic(SymEv::CSignal(ml));
  fence();
  curev().may_conflict = true;

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
  assert(0 <= curev().alt);
  assert(cond_var.waiters.empty() || curev().alt < int(cond_var.waiters.size()));
  if(cond_var.waiters.size()){
    /* Wake up the alt:th waiter. */
    int i = cond_var.waiters[curev().alt];
    assert(0 <= i && i < prefix_idx);
    IPid ipid = prefix[i].iid.get_pid();
    assert(!threads[ipid].available);
    threads[ipid].available = true;
    seen_events.insert(i);

    /* Remove waiter from cond_var.waiters */
    for(int j = curev().alt; j < int(cond_var.waiters.size())-1; ++j){
      cond_var.waiters[j] = cond_var.waiters[j+1];
    }
    cond_var.waiters.pop_back();
  }
  cond_var.last_signal = prefix_idx;

  see_events(seen_events);

  return true;
}

bool WSCTraceBuilder::cond_broadcast(const SymAddrSize &ml){
  record_symbolic(SymEv::CBrdcst(ml));
  fence();
  curev().may_conflict = true;

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
    threads[ipid].available = true;
    seen_events.insert(i);
  }
  cond_var.waiters.clear();
  cond_var.last_signal = prefix_idx;

  see_events(seen_events);

  return true;
}

bool WSCTraceBuilder::cond_wait(const SymAddrSize &cond_ml, const SymAddrSize &mutex_ml){
  {
    auto it = mutexes.find(mutex_ml.addr);
    if(it == mutexes.end()){
      if(conf.mutex_require_init){
        pthreads_error("cond_wait called with uninitialized mutex object.");
      }else{
        pthreads_error("cond_wait called with unlocked mutex object.");
      }
      return false;
    }
    Mutex &mtx = it->second;
    if(mtx.last_lock < 0 || prefix[mtx.last_lock].iid.get_pid() != curev().iid.get_pid()){
      pthreads_error("cond_wait called with mutex which is not locked by the same thread.");
      return false;
    }
  }

  mutex_unlock(mutex_ml);
  record_symbolic(SymEv::CWait(cond_ml));
  fence();
  curev().may_conflict = true;

  IPid pid = curev().iid.get_pid();

  auto it = cond_vars.find(cond_ml.addr);
  if(it == cond_vars.end()){
    pthreads_error("cond_wait called with uninitialized condition variable.");
    return false;
  }
  it->second.waiters.push_back(prefix_idx);
  threads[pid].available = false;

  see_events({last_full_memory_conflict,it->second.last_signal});

  return true;
}

bool WSCTraceBuilder::cond_awake(const SymAddrSize &cond_ml, const SymAddrSize &mutex_ml){
  assert(cond_vars.count(cond_ml.addr));
  CondVar &cond_var = cond_vars[cond_ml.addr];
  add_happens_after(prefix_idx, cond_var.last_signal);

  mutex_lock(mutex_ml);
  record_symbolic(SymEv::CAwake(cond_ml));
  curev().may_conflict = true;

  return true;
}

int WSCTraceBuilder::cond_destroy(const SymAddrSize &ml){
  record_symbolic(SymEv::CDelete(ml));
  fence();

  int err = (EBUSY == 1) ? 2 : 1; // Chose an error value different from EBUSY

  curev().may_conflict = true;

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

void WSCTraceBuilder::register_alternatives(int alt_count){
  curev().may_conflict = true;
  record_symbolic(SymEv::Nondet(alt_count));
  if(curev().alt == 0) {
    for(int i = curev().alt+1; i < alt_count; ++i){
      curev().races.push_back(Race::Nondet(prefix_idx, i));
    }
  }
}

template <class Iter>
static void rev_recompute_data
(SymData &data, VecSet<SymAddr> &needed, Iter end, Iter begin){
  for (auto pi = end; !needed.empty() && (pi != begin);){
    const SymEv &p = *(--pi);
    switch(p.kind){
    case SymEv::STORE:
    case SymEv::UNOBS_STORE:
    case SymEv::CMPXHG:
      if (data.get_ref().overlaps(p.addr())) {
        for (SymAddr a : p.addr()) {
          if (needed.erase(a)) {
            data[a] = p.data()[a];
          }
        }
      }
      break;
    default:
      break;
    }
  }
}

void WSCTraceBuilder::see_events(const VecSet<int> &seen_accesses){
  for(int i : seen_accesses){
    if(i < 0) continue;
    if (i == prefix_idx) continue;
    add_noblock_race(i);
  }
}

void WSCTraceBuilder::see_event_pairs
(const VecSet<std::pair<int,int>> &seen_accesses){
  for (std::pair<int,int> p : seen_accesses){
    add_observed_race(p.first, p.second);
  }
}

void WSCTraceBuilder::add_noblock_race(int event){
  assert(0 <= event);
  assert(event < prefix_idx);
  assert(do_events_conflict(event, prefix_idx));

  std::vector<Race> &races = curev().races;
  if (races.size()) {
    const Race &prev = races.back();
    if (prev.kind == Race::NONBLOCK
        && prev.first_event == event
        && prev.second_event == prefix_idx) return;
  }

  races.push_back(Race::Nonblock(event,prefix_idx));
}

void WSCTraceBuilder::add_lock_suc_race(int lock, int unlock){
  assert(0 <= lock);
  assert(lock < unlock);
  assert(unlock < prefix_idx);

  curev().races.push_back(Race::LockSuc(lock,prefix_idx,unlock));
}

void WSCTraceBuilder::add_lock_fail_race(const Mutex &m, int event){
  assert(0 <= event);
  assert(event < prefix_idx);

  lock_fail_races.push_back(Race::LockFail(event,prefix_idx,curev().iid,&m));
}

void WSCTraceBuilder::add_observed_race(int first, int second){
  assert(0 <= first);
  assert(first < second);
  assert(second < prefix_idx);
  assert(do_events_conflict(first, second));
  assert(do_events_conflict(second, prefix_idx));

  std::vector<Race> &races = prefix[second].races;
  if (races.size()) {
    const Race &prev = races.back();
    if (prev.kind == Race::OBSERVED
        && prev.first_event == first
        && prev.second_event == second
        && prev.witness_event == prefix_idx)
      return;
  }

  races.push_back(Race::Observed(first,second,prefix_idx));
}

void WSCTraceBuilder::add_happens_after(unsigned second, unsigned first){
  assert(first != ~0u);
  assert(second != ~0u);
  assert(first != second);
  assert(first < second);
  assert((long long)second <= prefix_idx);

  std::vector<unsigned> &vec = prefix[second].happens_after;
  if (vec.size() && vec.back() == first) return;

  vec.push_back(first);
}

void WSCTraceBuilder::add_happens_after_thread(unsigned second, IPid thread){
  assert((int)second == prefix_idx);
  if (threads[thread].event_indices.empty()) return;
  add_happens_after(second, threads[thread].event_indices.back());
}

/* Filter the sequence first..last from all elements that are less than
 * any other item. The sequence is modified in place and an iterator to
 * the position beyond the last included element is returned.
 *
 * Assumes less is transitive and asymetric (a strict partial order)
 */
template< class It, class LessFn >
static It frontier_filter(It first, It last, LessFn less){
  It fill = first;
  for (It current = first; current != last; ++current){
    bool include = true;
    for (It check = first; include && check != fill;){
      if (less(*current, *check)){
        include = false;
        break;
      }
      if (less(*check, *current)){
        /* Drop check from fill set */
        --fill;
        std::swap(*check, *fill);
      }else{
        ++check;
      }
    }
    if (include){
      /* Add current to fill set */
      if (fill != current) std::swap(*fill, *current);
      ++fill;
    }
  }
  return fill;
}

void WSCTraceBuilder::compute_vclocks(){
  Timing::Guard timing_guard(vclocks_context);
  /* The first event of a thread happens after the spawn event that
   * created it.
   */
  for (const Thread &t : threads) {
    if (t.spawn_event >= 0 && t.event_indices.size() > 0){
      add_happens_after(t.event_indices[0], t.spawn_event);
    }
  }

  std::vector<std::vector<unsigned>> happens_after(prefix.size());
  for (unsigned i = 0; i < prefix.size(); i++){
    IPid ipid = prefix[i].iid.get_pid();
    if (prefix[i].iid.get_index() > 1) {
      unsigned last = find_process_event(prefix[i].iid.get_pid(), prefix[i].iid.get_index()-1);
      prefix[i].clock = prefix[last].clock;
      happens_after[last].push_back(i);
    } else {
      prefix[i].clock = {};
    }
    prefix[i].clock[ipid] = prefix[i].iid.get_index();

    /* First add the non-reversible edges */
    for (unsigned j : prefix[i].happens_after){
      assert(j < i);
      prefix[i].clock += prefix[j].clock;
      happens_after[j].push_back(i);
    }

    prefix[i].above_clock = prefix[i].clock;

    /* Then add read-from */
    if (prefix[i].read_from && *prefix[i].read_from != -1) {
      prefix[i].clock += prefix[*prefix[i].read_from].clock;
      happens_after[*prefix[i].read_from].push_back(i);
    }
  }

  /* The top of our vector clock lattice, initial value of the below
   * clock (excluding itself) for evey event */
  VClock<IPid> top;
  for (unsigned i = 0; i < threads.size(); ++i)
    top[i] = threads[i].event_indices.size();

  for (unsigned i = prefix.size();;) {
    if (i == 0) break;
    Event &e = prefix[--i];
    e.below_clock = top;
    e.below_clock[e.iid.get_pid()] = e.iid.get_index();
    for (unsigned j : happens_after[i]) {
      assert (i < j);
      prefix[i].below_clock -= prefix[j].below_clock;
    }
  }
}

void WSCTraceBuilder::compute_unfolding() {
  Timing::Guard timing_guard(unfolding_context);
  for (unsigned i = 0; i < prefix.size(); ++i) {
    UnfoldingNodeChildren *parent_list;
    const std::shared_ptr<UnfoldingNode> null_ptr;
    const std::shared_ptr<UnfoldingNode> *parent;
    IID<IPid> iid = prefix[i].iid;
    IPid p = iid.get_pid();
    if (iid.get_index() == 1) {
      parent = &null_ptr;
      parent_list = &first_events[threads[p].cpid];
    } else {
      int par_idx = find_process_event(p, iid.get_index()-1);
      parent = &prefix[par_idx].event;
      parent_list = &(*parent)->children;
    }
    if (!prefix[i].may_conflict && (*parent != nullptr)) {
      prefix[i].event = *parent;
      continue;
    }
    const std::shared_ptr<UnfoldingNode> *read_from = &null_ptr;
    if (prefix[i].read_from && *prefix[i].read_from != -1) {
      read_from = &prefix[*prefix[i].read_from].event;
    }

    prefix[i].event = find_unfolding_node(*parent_list, *parent, *read_from);

    if (int(i) >= replay_point) {
      if (is_load(i)) {
        int decision = decisions.size();
        decisions.emplace_back();
        prefix[i].decision = decision;
      }
    }
  }
}

std::shared_ptr<WSCTraceBuilder::UnfoldingNode> WSCTraceBuilder::
find_unfolding_node(UnfoldingNodeChildren &parent_list,
                    const std::shared_ptr<UnfoldingNode> &parent,
                    const std::shared_ptr<UnfoldingNode> &read_from) {
  for (unsigned ci = 0; ci < parent_list.size();) {
    std::shared_ptr<UnfoldingNode> c = parent_list[ci].lock();
    if (!c) {
      /* Delete the null element and continue */
      std::swap(parent_list[ci], parent_list.back());
      parent_list.pop_back();
      continue;
    }

    if (c->read_from == read_from) {
      assert(parent == c->parent);
      return c;
    }
    ++ci;
  }

  /* Did not exist, create it. */
  std::shared_ptr<UnfoldingNode> c =
    std::make_shared<UnfoldingNode>(parent, read_from);
  parent_list.emplace_back(c);
  return c;
}

std::shared_ptr<WSCTraceBuilder::UnfoldingNode> WSCTraceBuilder::alternative
(unsigned i, const std::shared_ptr<UnfoldingNode> &read_from) {
  std::shared_ptr<UnfoldingNode> &parent = prefix[i].event->parent;
  UnfoldingNodeChildren *parent_list;
  if (parent) {
    parent_list = &parent->children;
  } else {
    IPid p = prefix[i].iid.get_pid();
    parent_list = &first_events[threads[p].cpid];
  }

  return find_unfolding_node(*parent_list, parent, read_from);
}

void WSCTraceBuilder::record_symbolic(SymEv event){
  if (!replay) {
    assert(sym_idx == curev().sym.size());
    /* New event */
    curev().sym.push_back(event);
    sym_idx++;
  } else {
    /* Replay. SymEv::set() asserts that this is the same event as last time. */
    assert(sym_idx < curev().sym.size());
    curev().sym[sym_idx++].set(event);
  }
}

bool WSCTraceBuilder::do_events_conflict(int i, int j) const{
  return do_events_conflict(prefix[i], prefix[j]);
}

bool WSCTraceBuilder::do_events_conflict
(const Event &fst, const Event &snd) const{
  return do_events_conflict(fst.iid.get_pid(), fst.sym,
                            snd.iid.get_pid(), snd.sym);
}

static bool symev_has_pid(const SymEv &e) {
  return e.kind == SymEv::SPAWN || e.kind == SymEv::JOIN;
}

static bool symev_is_load(const SymEv &e) {
  return e.kind == SymEv::LOAD || e.kind == SymEv::CMPXHGFAIL;
}

static bool symev_is_unobs_store(const SymEv &e) {
  return e.kind == SymEv::UNOBS_STORE;
}

bool WSCTraceBuilder::do_symevs_conflict
(IPid fst_pid, const SymEv &fst,
 IPid snd_pid, const SymEv &snd) const {
  if (fst.kind == SymEv::NONDET || snd.kind == SymEv::NONDET) return false;
  if (fst.kind == SymEv::FULLMEM || snd.kind == SymEv::FULLMEM) return true;
  if (symev_is_load(fst) && symev_is_load(snd)) return false;
  if (symev_is_unobs_store(fst) && symev_is_unobs_store(snd)
      && fst.addr() == snd.addr()) return false;

  /* Really crude. Is it enough? */
  if (fst.has_addr()) {
    if (snd.has_addr()) {
      return fst.addr().overlaps(snd.addr());
    } else {
      return false;
    }
  } else {
    return false;
  }
}

bool WSCTraceBuilder::do_events_conflict
(IPid fst_pid, const sym_ty &fst,
 IPid snd_pid, const sym_ty &snd) const{
  if (fst_pid == snd_pid) return true;
  for (const SymEv &fe : fst) {
    if (symev_has_pid(fe) && fe.num() == snd_pid) return true;
    for (const SymEv &se : snd) {
      if (do_symevs_conflict(fst_pid, fe, snd_pid, se)) {
        return true;
      }
    }
  }
  for (const SymEv &se : snd) {
    if (symev_has_pid(se) && se.num() == fst_pid) return true;
  }
  return false;
}

bool WSCTraceBuilder::is_observed_conflict
(const Event &fst, const Event &snd, const Event &thd) const{
  return is_observed_conflict(fst.iid.get_pid(), fst.sym,
                              snd.iid.get_pid(), snd.sym,
                              thd.iid.get_pid(), thd.sym);
}

bool WSCTraceBuilder::is_observed_conflict
(IPid fst_pid, const sym_ty &fst,
 IPid snd_pid, const sym_ty &snd,
 IPid thd_pid, const sym_ty &thd) const{
  if (fst_pid == snd_pid) return false;
  for (const SymEv &fe : fst) {
    if (fe.kind != SymEv::UNOBS_STORE) continue;
    for (const SymEv &se : snd) {
      if (se.kind != SymEv::STORE
          || !fe.addr().overlaps(se.addr())) continue;
      for (const SymEv &te : thd) {
        if (is_observed_conflict(fst_pid, fe, snd_pid, se, thd_pid, te)) {
          return true;
        }
      }
    }
  }
  return false;
}

bool WSCTraceBuilder::is_load(unsigned i) const {
  return std::any_of(prefix[i].sym.begin(), prefix[i].sym.end(),
    [](const SymEv &e) {
      return e.kind == SymEv::LOAD || e.kind == SymEv::CMPXHG
        || e.kind == SymEv::CMPXHGFAIL; });
}

bool WSCTraceBuilder::is_store(unsigned i) const {
  return std::any_of(prefix[i].sym.begin(), prefix[i].sym.end(),
    [](const SymEv &e) {
      return e.kind == SymEv::STORE || e.kind == SymEv::CMPXHG;
    });
}

bool WSCTraceBuilder::is_cmpxhgfail(unsigned i) const {
  return std::any_of(prefix[i].sym.begin(), prefix[i].sym.end(),
    [](const SymEv &e) { return e.kind == SymEv::CMPXHGFAIL; });
}

bool WSCTraceBuilder::is_store_when_reading_from(unsigned i, int read_from) const {
  return std::any_of(prefix[i].sym.begin(), prefix[i].sym.end(),
    [read_from,this](const SymEv &e) {
      if (e.kind == SymEv::STORE) return true;
      if (e.kind != SymEv::CMPXHG && e.kind != SymEv::CMPXHGFAIL) return false;
      SymData expected = e.expected();
      SymData actual = get_data(read_from, e.addr());
      assert(e.addr() == actual.get_ref());
      return memcmp(expected.get_block(), actual.get_block(), e.addr().size) == 0;
    });
}

SymAddrSize WSCTraceBuilder::get_addr(unsigned i) const {
  for (const SymEv &e : prefix[i].sym) {
    if (e.has_addr()) {
      assert(std::all_of(prefix[i].sym.begin(), prefix[i].sym.end(),
                         [&e](const SymEv &f) {
                           return !f.has_addr() || f.addr() == e.addr();
                         }));
      return e.addr();
    }
  }
  abort();
}

SymData WSCTraceBuilder::get_data(int i, const SymAddrSize &addr) const {
  if (i == -1) {
    SymData ret(addr, addr.size);
    memset(ret.get_block(), 0, addr.size);
    return ret;
  }
  for (auto eit = prefix[i].sym.begin(); eit != prefix[i].sym.end(); ++eit) {
    const SymEv &e = *eit;
    if (e.has_data()) {
      assert(std::all_of(eit+1, prefix[i].sym.end(),
                         [&e](const SymEv &f) {
                           return !f.has_data() /* || f.data() == e.data() */;
                         }));
      assert(e.addr() == addr);
      return e.data();
    }
  }
  abort();
}

static std::ptrdiff_t delete_from_back(std::vector<int> &vec, int val) {
  for (auto it = vec.end();;) {
    assert(it != vec.begin() && "Element must exist");
    --it;
    if (val == *it) {
      std::swap(*it, vec.back());
      auto pos = it - vec.begin();
      vec.pop_back();
      return pos;
    }
  }
}

WSCTraceBuilder::CmpXhgUndoLog WSCTraceBuilder::
recompute_cmpxhg_success(unsigned idx, std::vector<int> &writes) {
  for (SymEv &e : prefix[idx].sym) {
    if (e.kind != SymEv::CMPXHG && e.kind != SymEv::CMPXHGFAIL) continue;
    bool before = e.kind == SymEv::CMPXHG;
    SymData expected = e.expected();
    SymData actual = get_data(*prefix[idx].read_from, e.addr());
    bool after = memcmp(expected.get_block(), actual.get_block(), e.addr().size) == 0;
    if (after) {
      e = SymEv::CmpXhg(e.data(), e.expected().get_shared_block());
    } else {
      e = SymEv::CmpXhgFail(e.data(), e.expected().get_shared_block());
    }
    // assert(recompute_cmpxhg_success(idx, writes).kind == CmpXhgUndoLog::NONE);
    auto kind = CmpXhgUndoLog::NONE;
    unsigned pos;
    if (after && !before) {
      kind = CmpXhgUndoLog::FAIL;
      pos = writes.size();
      writes.push_back(idx);
    } else if (before && !after) {
      kind = CmpXhgUndoLog::SUCCEED;
      pos = delete_from_back(writes, idx);
    }
    return CmpXhgUndoLog{kind, idx, pos, &e};
  }
  return CmpXhgUndoLog{CmpXhgUndoLog::NONE, idx, 0, nullptr};
}

void WSCTraceBuilder::
undo_cmpxhg_recomputation(CmpXhgUndoLog log, std::vector<int> &writes) {
  if (log.kind == CmpXhgUndoLog::NONE) return;
  SymEv &e = *log.e;
  if (log.kind == CmpXhgUndoLog::SUCCEED) {
    e = SymEv::CmpXhg(e.data(), e.expected().get_shared_block());
    writes.push_back(log.idx);
    std::swap(writes.back(), writes[log.pos]);
  } else {
    assert(log.kind == CmpXhgUndoLog::FAIL);
    e = SymEv::CmpXhgFail(e.data(), e.expected().get_shared_block());
    assert(writes.back() == int(log.idx));
    assert(writes.size()-1 == log.pos);
    writes.pop_back();
  }
}

bool WSCTraceBuilder::is_observed_conflict
(IPid fst_pid, const SymEv &fst,
 IPid snd_pid, const SymEv &snd,
 IPid thd_pid, const SymEv &thd) const {
  assert(fst_pid != snd_pid);
  assert(fst.kind == SymEv::UNOBS_STORE);
  assert(snd.kind == SymEv::STORE);
  assert(fst.addr().overlaps(snd.addr()));
  if (thd.kind == SymEv::FULLMEM) return true;
  return thd.kind == SymEv::LOAD && thd.addr().overlaps(snd.addr());
}

bool WSCTraceBuilder::can_rf_by_vclocks
(int r, int ow, int w) const {
  /* Is the write after the read? */
  if (w != -1 && prefix[r].clock.leq(prefix[w].clock)) return false;

  /* Is the original write always before the read, and the new write
   * before the original?
   */
  if (ow != -1 && (w == -1 || prefix[w].clock.leq(prefix[ow].clock))) {
    if (prefix[r].iid.get_index() != 1) {
      unsigned r_po_pred = find_process_event(prefix[r].iid.get_pid(),
                                              prefix[r].iid.get_index()-1);
      if (prefix[ow].clock.leq(prefix[r_po_pred].clock)) return false;
    }
    for (unsigned pred : prefix[r].happens_after) {
      if (prefix[ow].clock.leq(prefix[pred].clock)) return false;
    }
  }

  return true;
}

bool WSCTraceBuilder::can_swap_by_vclocks(int r, int w) const {
  if (prefix[w].iid.get_index() != 1) {
    unsigned w_po_pred = find_process_event(prefix[w].iid.get_pid(),
                                            prefix[w].iid.get_index()-1);
    if (prefix[r].clock.leq(prefix[w_po_pred].clock)) return false;
  }
  for (unsigned pred : prefix[w].happens_after) {
    if (prefix[r].clock.leq(prefix[pred].clock)) return false;
  }
  return true;
}

void WSCTraceBuilder::compute_prefixes() {
  Timing::Guard analysis_timing_guard(analysis_context);
  compute_vclocks();

  compute_unfolding();

  if(conf.debug_print_on_reset){
    llvm::dbgs() << " === WSCTraceBuilder state ===\n";
    debug_print();
    llvm::dbgs() << " =============================\n";
  }

  Timing::Guard neighbours_timing_guard(neighbours_context);
  if (conf.debug_print_on_reset)
    llvm::dbgs() << "Computing prefixes\n";

  auto pretty_index = [&] (int i) -> std::string {
    if (i==-1) return "init event";
    return std::to_string(prefix[i].decision) + "("
      + std::to_string(prefix[i].event->seqno) + "):"
      + iid_string(i) + events_to_string(prefix[i].sym);
  }; 

  std::map<SymAddr,std::vector<int>> writes_by_address;
  std::map<SymAddr,std::vector<int>> cmpxhgfail_by_address;
  std::vector<std::unordered_map<SymAddr,std::vector<unsigned>>>
    writes_by_process_and_address(threads.size());
  for (unsigned j = 0; j < prefix.size(); ++j) {
    if (is_store(j))      writes_by_address    [get_addr(j).addr].push_back(j);
    if (is_store(j))
      writes_by_process_and_address[prefix[j].iid.get_pid()][get_addr(j).addr]
        .push_back(j);
    if (is_cmpxhgfail(j)) cmpxhgfail_by_address[get_addr(j).addr].push_back(j);
  }
  // for (std::pair<SymAddr,std::vector<int>> &pair : writes_by_address) {
  //   pair.second.push_back(-1);
  // }

  for (unsigned i = 0; i < prefix.size(); ++i) {
    if (!prefix[i].pinned && is_load(i)) {
      auto addr = get_addr(i);
      std::vector<int> &possible_writes = writes_by_address[addr.addr];
      int original_read_from = *prefix[i].read_from;
      assert(std::any_of(possible_writes.begin(), possible_writes.end(),
             [=](int i) { return i == original_read_from; })
           || original_read_from == -1);      

      DecisionNode &decision = decisions[prefix[i].decision];

      auto try_read_from_rmw = [&](int j) {
        assert(j != -1 && j > int(i) && is_store(i) && is_load(j)
               && is_store_when_reading_from(j, original_read_from));
        /* Can only swap ajacent RMWs */
        if (*prefix[j].read_from != int(i)) return;
        std::shared_ptr<UnfoldingNode> read_from
          = alternative(j, prefix[i].event->read_from);
        if (decision.siblings.count(read_from)) return;
        if (decision.sleep.count(read_from)) return;
        if (!can_swap_by_vclocks(i, j)) {
          decision.siblings.emplace(read_from, Leaf());
          return;
        }
        if (conf.debug_print_on_reset)
          llvm::dbgs() << "Trying to swap " << pretty_index(i)
                       << " and " << pretty_index(j)
                       << ", reading from " << pretty_index(original_read_from);
        prefix[j].read_from = original_read_from;
        prefix[i].read_from = j;
        auto undoj = recompute_cmpxhg_success(j, possible_writes);
        auto undoi = recompute_cmpxhg_success(i, possible_writes);

        Leaf solution = try_sat(i, writes_by_address);
        decision.siblings.emplace(read_from, std::move(solution));

        /* Reset read-from */
        prefix[j].read_from = i;
        undo_cmpxhg_recomputation(undoi, possible_writes);
        undo_cmpxhg_recomputation(undoj, possible_writes);
      };
      auto try_read_from = [&](int j) {
        if (j == original_read_from || j == int(i)) return;
        if (j != -1 && j > int(i) && is_store(i) && is_load(j)) {
          if (!is_store_when_reading_from(j, original_read_from)) return;
          /* RMW pair */
          try_read_from_rmw(j);
        } else {
          const std::shared_ptr<UnfoldingNode> &read_from =
            j == -1 ? nullptr : prefix[j].event;
          if (decision.siblings.count(read_from)) return;
          if (decision.sleep.count(read_from)) return;
          if (!can_rf_by_vclocks(i, original_read_from, j)) {
            decision.siblings.emplace(read_from, Leaf());
            return;
          }
          if (conf.debug_print_on_reset)
            llvm::dbgs() << "Trying to make " << pretty_index(i)
                         << " read from " << pretty_index(j)
                         << " instead of " << pretty_index(original_read_from);
          prefix[i].read_from = j;
          auto undoi = recompute_cmpxhg_success(i, possible_writes);

          Leaf solution = try_sat(i, writes_by_address);
          decision.siblings.emplace(read_from, std::move(solution));

          undo_cmpxhg_recomputation(undoi, possible_writes);
        }
      };

      const VClock<IPid> &above = prefix[i].above_clock;
      const VClock<IPid> &below = prefix[i].below_clock;
      for (unsigned p = 0; p < threads.size(); ++p) {
        const std::vector<unsigned> &writes
          = writes_by_process_and_address[p][addr.addr];
        auto start = std::upper_bound(writes.begin(), writes.end(), above[p],
                                      [this](int index, unsigned w) {
                                        return index < prefix[w].iid.get_index();
                                      });
        if (start != writes.begin()) --start;

        auto end = std::lower_bound(writes.begin(), writes.end(), below[p],
                                    [this](unsigned w, int index) {
                                      return prefix[w].iid.get_index() < index;
                                    });
        /* Ugly hack:
         * If i and *end are rmw's, we need to consider swapping i and
         * *end. Since this is considered by calling
         * try_read_from(*end), we make sure that *end is not filtered
         * here, even though it's not in the visible set.
         */
        if (is_store(i) && end != writes.end() && is_load(*end)) ++end;
        assert(start <= end);
        for (auto it = start; it != end; ++it) {
          try_read_from(*it);
        }
      }
      try_read_from(-1);
      if (is_store(i)) {
        for (int j : cmpxhgfail_by_address[addr.addr]) {
          if (j > int(i) && is_store_when_reading_from(j, original_read_from)) {
            try_read_from_rmw(j);
          }
        }
      }

      /* Reset read from, but leave pinned = true */
      prefix[i].read_from = original_read_from;
    }
  }
}

void WSCTraceBuilder::output_formula
(SatSolver &sat,
 std::map<SymAddr,std::vector<int>> &writes_by_address,
 const std::vector<bool> &keep){
  auto get_addr = [&](unsigned i) {
    for (SymEv &e : prefix[i].sym) {
      if (e.has_addr()) return e.addr();
    }
    abort();
  };

  unsigned no_keep = 0;
  std::vector<unsigned> var;
  for (unsigned i = 0; i < prefix.size(); ++i) {
    var.push_back(no_keep);
    if (keep[i]) no_keep++;
  }

  sat.alloc_variables(no_keep);
  /* PO */
  for (unsigned i = 0; i < prefix.size(); ++i) {
    if (!keep[i]) continue;
    if (Option<unsigned> pred = po_predecessor(i)) {
      assert(*pred != i);
      sat.add_edge(var[*pred], var[i]);
    }
  }

  /* Read-from and SC consistency */
  for (unsigned r = 0; r < prefix.size(); ++r) {
    if (!keep[r] || !prefix[r].read_from) continue;
    int w = *prefix[r].read_from;
    assert(int(r) != w);
    if (w == -1) {
      for (int j : writes_by_address[get_addr(r).addr]) {
        if (j == int(r) || !keep[j]) continue;
        sat.add_edge(var[r], var[j]);
      }
    } else {
      sat.add_edge(var[w], var[r]);
      for (int j : writes_by_address[get_addr(r).addr]) {
        if (j == w || j == int(r) || !keep[j]) continue;
        sat.add_edge_disj(var[j], var[w],
                          var[r], var[j]);
      }
    }
  }

  /* Other happens-after edges (such as thread spawn and join) */
  for (unsigned i = 0; i < prefix.size(); ++i) {
    if (!keep[i]) continue;
    for (unsigned j : prefix[i].happens_after) {
      sat.add_edge(var[j], var[i]);
    }
  }
}

WSCTraceBuilder::Leaf
WSCTraceBuilder::try_sat
(int changed_event, std::map<SymAddr,std::vector<int>> &writes_by_address){
  Timing::Guard timing_guard(graph_context);
  int decision = prefix[changed_event].decision;
  std::vector<bool> keep = causal_past(decision);

  {
    SaturatedGraph g;
    const auto add_event = [&g,this](unsigned i) {
        SaturatedGraph::EventKind kind = SaturatedGraph::NONE;
        SymAddr addr;
        if (is_load(i)) {
          if (is_store(i)) kind = SaturatedGraph::RMW;
          else kind = SaturatedGraph::LOAD;
        } else if (is_store(i)) kind = SaturatedGraph::STORE;
        if (kind != SaturatedGraph::NONE) addr = get_addr(i).addr;
        Option<unsigned> read_from;
        if (prefix[i].read_from && *prefix[i].read_from != -1)
          read_from = unsigned(*prefix[i].read_from);
        g.add_event(unsigned(prefix[i].iid.get_pid()), i, kind, addr,
                    read_from, prefix[i].happens_after);
      };
    for (unsigned i = 0; i < prefix.size(); ++i) {
      if (keep[i] && int(i) != changed_event) {
        add_event(i);
      }
    }
    assert(keep[changed_event]);
    add_event(changed_event);
    if (!g.saturate()) {
      if (conf.debug_print_on_reset)
        llvm::dbgs() << ": Saturation yielded cycle\n";
      return Leaf();
    }

    if (Option<std::vector<unsigned>> res = try_generate_prefix(std::move(g))) {
      if (conf.debug_print_on_reset) {
        llvm::dbgs() << ": Heuristic found prefix\n";
        llvm::dbgs() << "[";
        for (unsigned i : *res) {
          llvm::dbgs() << i << ",";
        }
        llvm::dbgs() << "]\n";
      }
      return order_to_leaf(decision, *res);
    }
  }

  std::unique_ptr<SatSolver> sat = conf.get_sat_solver();
  {
    Timing::Guard timing_guard(sat_context);

    output_formula(*sat, writes_by_address, keep);
    //output_formula(std::cerr, writes_by_address, keep);

    if (!sat->check_sat()) {
      if (conf.debug_print_on_reset) llvm::dbgs() << ": UNSAT\n";
      return Leaf();
    }
    if (conf.debug_print_on_reset) llvm::dbgs() << ": SAT\n";

  }
  std::vector<unsigned> model = sat->get_model();

  unsigned no_keep = 0;
  for (unsigned i = 0; i < prefix.size(); ++i) {
    if (keep[i]) no_keep++;
  }
  std::vector<unsigned> order(no_keep);
  for (unsigned i = 0, var = 0; i < prefix.size(); ++i) {
    if (keep[i]) {
      unsigned pos = model[var++];
      order[pos] = i;
    }
  }

  if (conf.debug_print_on_reset) {
    llvm::dbgs() << "[";
    for (unsigned i : order) {
      llvm::dbgs() << i << ",";
    }
    llvm::dbgs() << "]\n";
  }

  return order_to_leaf(decision, order);
}

WSCTraceBuilder::Leaf WSCTraceBuilder::order_to_leaf
(int decision, const std::vector<unsigned> order) const{
  std::vector<Branch> new_prefix;
  new_prefix.reserve(order.size());
  for (unsigned i : order) {
    bool is_the_changed_read = prefix[i].decision == decision
      && !prefix[i].pinned;
    bool new_pinned = prefix[i].pinned || (prefix[i].decision > decision);
    int new_decision = prefix[i].decision;
    if (new_decision > decision) {
      new_pinned = true;
      new_decision = -1;
    }
    new_prefix.emplace_back(prefix[i].iid.get_pid(),
                            is_the_changed_read ? 1 : prefix[i].size,
                            new_decision,
                            new_pinned,
                            prefix[i].sym);
  }

  return Leaf(new_prefix);
}

std::vector<bool> WSCTraceBuilder::causal_past(int decision) const {
  std::vector<bool> acc(prefix.size());
  for (unsigned i = 0; i < prefix.size(); ++i) {
    assert(is_load(i) == ((prefix[i].decision != -1) ^ prefix[i].pinned));
    if (prefix[i].decision != -1 && prefix[i].decision <= decision) {
      causal_past_1(acc, i);
    }
  };
  return acc;
}

void WSCTraceBuilder::causal_past_1(std::vector<bool> &acc, unsigned i) const{
  if (acc[i] == true) return;
  acc[i] = true;
  if (prefix[i].read_from && *prefix[i].read_from != -1) {
    causal_past_1(acc, *prefix[i].read_from);
  }
  for (unsigned j : prefix[i].happens_after) {
    causal_past_1(acc, j);
  }
  if (Option<unsigned> pred = po_predecessor(i)) {
    causal_past_1(acc, *pred);
  }
}

std::vector<int> WSCTraceBuilder::iid_map_at(int event) const{
  std::vector<int> map(threads.size(), 1);
  for (int i = 0; i < event; ++i) {
    iid_map_step(map, prefix[i]);
  }
  return map;
}

void WSCTraceBuilder::iid_map_step(std::vector<int> &iid_map, const Event &event) const{
  if (iid_map.size() <= unsigned(event.iid.get_pid())) iid_map.resize(event.iid.get_pid()+1, 1);
  iid_map[event.iid.get_pid()] += event.size;
}

void WSCTraceBuilder::iid_map_step_rev(std::vector<int> &iid_map, const Event &event) const{
  iid_map[event.iid.get_pid()] -= event.size;
}

WSCTraceBuilder::Event WSCTraceBuilder::
reconstruct_lock_event(const Race &race) const {
  assert(race.kind == Race::LOCK_FAIL);
  Event ret(race.second_process);
  /* Compute the clock of the locking process (event k in prefix is
   * something unrelated since this is a lock probe) */
  /* Find last event of p before this mutex probe */
  IPid p = race.second_process.get_pid();
  if (race.second_process.get_index() != 1) {
    int last = find_process_event(p, race.second_process.get_index()-1);
    ret.clock = prefix[last].clock;
  }
  /* Recompute the clock of this mutex_lock_fail */
  ++ret.clock[p];

  assert(std::any_of(prefix[race.first_event].sym.begin(),
                     prefix[race.first_event].sym.end(),
                     [](const SymEv &e){ return e.kind == SymEv::M_LOCK
                         || e.kind == SymEv::FULLMEM; }));
  ret.sym = prefix[race.first_event].sym;
  return ret;
}

inline Option<unsigned> WSCTraceBuilder::
try_find_process_event(IPid pid, int index) const{
  assert(pid >= 0 && pid < int(threads.size()));
  assert(index >= 1);
  if (index > int(threads[pid].event_indices.size())){
    return nullptr;
  }

  unsigned k = threads[pid].event_indices[index-1];
  assert(k < prefix.size());
  assert(prefix[k].size > 0);
  assert(prefix[k].iid.get_pid() == pid
         && prefix[k].iid.get_index() <= index
         && (prefix[k].iid.get_index() + prefix[k].size) > index);

  return k;
}

inline unsigned WSCTraceBuilder::find_process_event(IPid pid, int index) const{
  assert(pid >= 0 && pid < int(threads.size()));
  assert(index >= 1 && index <= int(threads[pid].event_indices.size()));
  unsigned k = threads[pid].event_indices[index-1];
  assert(k < prefix.size());
  assert(prefix[k].size > 0);
  assert(prefix[k].iid.get_pid() == pid
         && prefix[k].iid.get_index() <= index
         && (prefix[k].iid.get_index() + prefix[k].size) > index);

  return k;
}

int WSCTraceBuilder::estimate_trace_count() const{
  return estimate_trace_count(0);
}

bool WSCTraceBuilder::check_for_cycles() {
  return false;
}

int WSCTraceBuilder::estimate_trace_count(int idx) const{
  if(idx > int(prefix.size())) return 0;
  if(idx == int(prefix.size())) return 1;

  int count = 42;
  for(int i = int(prefix.size())-1; idx <= i; --i){
    count += prefix[i].sleep_branch_trace_count;
    // count += std::max(0, int(prefix.children_after(i)))
    //   * (count / (1 + prefix[i].sleep.size()));
  }

  return count;
}
