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
#include "RFSCTraceBuilder.h"
#include "PrefixHeuristic.h"
#include "Timing.h"
#include "TraceUtil.h"

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <stdexcept>

#define ANSIRed "\x1b[91m"
#define ANSIRst "\x1b[m"

#ifndef NDEBUG
#  define IFDEBUG(X) X
#else
#  define IFDEBUG(X)
#endif

static Timing::Context analysis_context("analysis");
static Timing::Context vclocks_context("vclocks");
static Timing::Context unfolding_context("unfolding");
static Timing::Context neighbours_context("neighbours");
static Timing::Context try_read_from_context("try_read_from");
static Timing::Context ponder_mutex_context("ponder_mutex");
static Timing::Context graph_context("graph");
static Timing::Context sat_context("sat");

RFSCTraceBuilder::RFSCTraceBuilder(RFSCDecisionTree &desicion_tree_,
                                   RFSCUnfoldingTree &unfolding_tree_,
                                   const Configuration &conf)
    : TSOPSOTraceBuilder(conf),
      unfolding_tree(unfolding_tree_),
      decision_tree(desicion_tree_) {
    threads.push_back(Thread(CPid(), -1));
    prefix_idx = -1;
    replay = false;
    cancelled = false;
    last_full_memory_conflict = -1;
    last_md = 0;
    replay_point = 0;
    tasks_created = 0;
}

RFSCTraceBuilder::~RFSCTraceBuilder(){
}

bool RFSCTraceBuilder::schedule(int *proc, int *aux, int *alt, bool *dryrun){
  if (cancelled) {
    assert(!std::any_of(threads.begin(), threads.end(), [](const Thread &t) {
                                                          return t.available;
                                                        }));
    return false;
  }
  *dryrun = false;
  *alt = 0;
  *aux = -1; /* No auxiliary threads in SC */
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
      assert(prefix_idx < 0 || (curev().sym.empty() ^ seen_effect)
             || (errors.size() && errors.back()->get_location()
                 == IID<CPid>(threads[curev().iid.get_pid()].cpid,
                              curev().iid.get_index())));
      replay = false;
    } else {
      /* Go to the next event. */
      assert(prefix_idx < 0 || (curev().sym.empty() ^ seen_effect)
             || (errors.size() && errors.back()->get_location()
                 == IID<CPid>(threads[curev().iid.get_pid()].cpid,
                              curev().iid.get_index())));
      seen_effect = false;
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


  /* Find an available thread. */
  for(unsigned p = 0; p < threads.size(); ++p){ // Loop through real threads
    if(threads[p].available &&
       (conf.max_search_depth < 0 || threads[p].last_event_index() < conf.max_search_depth)){
      /* Create a new Event */
      seen_effect = false;
      ++prefix_idx;
      assert(prefix_idx == int(prefix.size()));
      threads[p].event_indices.push_back(prefix_idx);
      prefix.emplace_back(IID<IPid>(IPid(p),threads[p].last_event_index()));
      *proc = p;
      return true;
    }
  }

  return false; // No available threads
}

void RFSCTraceBuilder::refuse_schedule(){
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

void RFSCTraceBuilder::mark_available(int proc, int aux){
  threads[ipid(proc,aux)].available = true;
}

void RFSCTraceBuilder::mark_unavailable(int proc, int aux){
  threads[ipid(proc,aux)].available = false;
}

bool RFSCTraceBuilder::is_replaying() const {
  return prefix_idx < replay_point;
}

void RFSCTraceBuilder::cancel_replay(){
  assert(replay == is_replaying());
  cancelled = true;
  replay = false;

  /* Find last decision, the one that caused this failure, and then
   * prune all later decisions. */
  int blame = -1, max_depth = -1; /* -1: All executions lead to this failure */
  for (int i = 0; i <= prefix_idx; ++i) {
    if (prefix[i].get_decision_depth() > max_depth) {
      blame = i;
      max_depth = prefix[i].get_decision_depth();
    }
  }
  if (blame != -1) {
    prefix[blame].decision_ptr->prune_decisions();
  }
}

void RFSCTraceBuilder::metadata(const llvm::MDNode *md){
  curev().md = last_md = md;
}

bool RFSCTraceBuilder::sleepset_is_empty() const{
  return true;
}

Trace *RFSCTraceBuilder::get_trace() const{
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

bool RFSCTraceBuilder::reset(){
  work_item = decision_tree.get_next_work_task();

  while (work_item != nullptr && work_item->is_pruned()) {
    tasks_created--;
    work_item = decision_tree.get_next_work_task();
  }

  if (work_item == nullptr) {
    /* DecisionNodes have been pruned and its trailing thread-task need to return. */
    return false;
  }

  Leaf l = std::move(work_item->leaf);
  auto unf = std::move(work_item->unfold_node);

  assert(!l.is_bottom());
  if (conf.debug_print_on_reset && !l.prefix.empty())
    llvm::dbgs() << "Backtracking to decision node " << (work_item->depth)
                 << ", replaying " << l.prefix.size() << " events to read from "
                 << (unf ? std::to_string(unf->seqno) : "init") << "\n";

  assert(l.prefix.empty() == (work_item->depth == -1));

  replay_point = l.prefix.size();

  std::vector<Event> new_prefix;
  new_prefix.reserve(l.prefix.size());
  std::vector<int> iid_map;
  for (Branch &b : l.prefix) {
    int index = (int(iid_map.size()) <= b.pid) ? 1 : iid_map[b.pid];
    IID<IPid> iid(b.pid, index);
    new_prefix.emplace_back(iid);
    new_prefix.back().size = b.size;
    new_prefix.back().sym = std::move(b.sym);
    new_prefix.back().pinned = b.pinned;
    new_prefix.back().set_branch_decision(b.decision_depth, work_item);
    iid_map_step(iid_map, new_prefix.back());
  }

#ifndef NDEBUG
  for (int d = 0; d < work_item->depth; ++d) {
    assert(std::any_of(new_prefix.begin(), new_prefix.end(),
                       [&](const Event &e) { return e.get_decision_depth() == d; }));
  }
#endif

  prefix = std::move(new_prefix);

  CPS = CPidSystem();
  threads.clear();
  threads.push_back(Thread(CPid(),-1));
  mutexes.clear();
  cond_vars.clear();
  mem.clear();
  mutex_deadlocks.clear();
  last_full_memory_conflict = -1;
  prefix_idx = -1;
  replay = true;
  cancelled = false;
  last_md = 0;
  tasks_created = 0;
  reset_cond_branch_log();

  return true;
}

IID<CPid> RFSCTraceBuilder::get_iid() const{
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

std::string RFSCTraceBuilder::iid_string(std::size_t pos) const{
  return iid_string(prefix[pos]);
}

std::string RFSCTraceBuilder::iid_string(const Event &event) const{
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

std::string RFSCTraceBuilder::iid_string(IID<IPid> iid) const{
  return iid_string(find_process_event(iid.get_pid(), iid.get_index()));
}

void RFSCTraceBuilder::debug_print() const {
  llvm::dbgs() << "RFSCTraceBuilder (debug print, replay until " << replay_point << "):\n";
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
    dec_offs = std::max(dec_offs,int(std::to_string(prefix[i].get_decision_depth()).size())
                        + (prefix[i].pinned ? 1 : 0));
    unf_offs = std::max(unf_offs,int(std::to_string(prefix[i].event->seqno).size()));
    rf_offs = std::max(rf_offs,prefix[i].read_from ?
                       int(std::to_string(*prefix[i].read_from).size())
                       : 1);
    clock_offs = std::max(clock_offs,int(prefix[i].clock.to_string().size()));
    lines[i] = prefix[i].sym.to_string();
  }

  for(unsigned i = 0; i < prefix.size(); ++i){
    IPid ipid = prefix[i].iid.get_pid();
    llvm::dbgs() << " " << lpad(std::to_string(i),idx_offs)
                 << rpad("",1+ipid*2)
                 << rpad(iid_string(i),iid_offs-ipid*2)
                 << " " << lpad((prefix[i].pinned ? "*" : "")
                                + std::to_string(prefix[i].get_decision_depth()),dec_offs)
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

bool RFSCTraceBuilder::spawn(){
  curev().may_conflict = true;
  if (!record_symbolic(SymEv::Spawn(threads.size() - 1))) return false;
  IPid parent_ipid = curev().iid.get_pid();
  CPid child_cpid = CPS.spawn(threads[parent_ipid].cpid);
  threads.push_back(Thread(child_cpid,prefix_idx));
  return true;
}

bool RFSCTraceBuilder::store(const SymData &sd){
  assert(false && "Cannot happen");
  abort();
  return true;
}

bool RFSCTraceBuilder::atomic_store(const SymData &sd){
  if (!record_symbolic(SymEv::Store(sd))) return false;
  do_atomic_store(sd);
  return true;
}

void RFSCTraceBuilder::do_atomic_store(const SymData &sd){
  const SymAddrSize &ml = sd.get_ref();
  curev().may_conflict = true;

  /* See previous updates reads to ml */
  for(SymAddr b : ml){
    ByteInfo &bi = mem[b];

    /* Register in memory */
    bi.last_update = prefix_idx;
  }
}

bool RFSCTraceBuilder::atomic_rmw(const SymData &sd, RmwAction action){
  if (!record_symbolic(SymEv::Rmw(sd, std::move(action)))) return false;
  do_load(sd.get_ref());
  do_atomic_store(sd);
  return true;
}

bool RFSCTraceBuilder::load(const SymAddrSize &ml){
  if (!record_symbolic(SymEv::Load(ml))) return false;
  do_load(ml);
  return true;
}

void RFSCTraceBuilder::do_load(const SymAddrSize &ml){
  curev().may_conflict = true;
  int lu = mem[ml.addr].last_update;
  curev().read_from = lu;

  assert(lu == -1 || get_addr(lu) == ml);
  assert(std::all_of(ml.begin(), ml.end(), [lu,this](SymAddr b) {
             return mem[b].last_update == lu;
           }));
}

bool RFSCTraceBuilder::compare_exchange
(const SymData &sd, const SymData::block_type expected, bool success){
  if(success){
    if (!record_symbolic(SymEv::CmpXhg(sd, expected))) return false;
    do_load(sd.get_ref());
    do_atomic_store(sd);
  }else{
    if (!record_symbolic(SymEv::CmpXhgFail(sd, expected))) return false;
    do_load(sd.get_ref());
  }
  return true;
}

bool RFSCTraceBuilder::full_memory_conflict(){
  invalid_input_error("RFSC does not support black-box functions with memory effects");
  return false;
  if (!record_symbolic(SymEv::Fullmem())) return false;
  curev().may_conflict = true;

  // /* See all previous memory accesses */
  // for(auto it = mem.begin(); it != mem.end(); ++it){
  //   do_load(it->second);
  // }
  // last_full_memory_conflict = prefix_idx;

  // /* No later access can have a conflict with any earlier access */
  // mem.clear();
  return true;
}

bool RFSCTraceBuilder::fence(){
  return true;
}

bool RFSCTraceBuilder::join(int tgt_proc){
  if (!record_symbolic(SymEv::Join(tgt_proc))) return false;
  curev().may_conflict = true;
  add_happens_after_thread(prefix_idx, tgt_proc);
  return true;
}

bool RFSCTraceBuilder::mutex_lock(const SymAddrSize &ml){
  if (!record_symbolic(SymEv::MLock(ml))) return false;

  Mutex &mutex = mutexes[ml.addr];
  curev().may_conflict = true;
  curev().read_from = mutex.last_access;

  mutex.last_lock = mutex.last_access = prefix_idx;
  mutex.locked = true;
  return true;
}

bool RFSCTraceBuilder::mutex_lock_fail(const SymAddrSize &ml){
  assert(!conf.mutex_require_init || mutexes.count(ml.addr));
  IFDEBUG(Mutex &mutex = mutexes[ml.addr];)
  assert(0 <= mutex.last_lock && mutex.locked);

  IPid current = curev().iid.get_pid();
  auto &deadlocks = mutex_deadlocks[ml.addr];
  assert(!std::any_of(deadlocks.cbegin(), deadlocks.cend(),
                      [current](IPid blocked) {
                        return blocked == current;
                      }));
  deadlocks.push_back(current);
  return true;
}

bool RFSCTraceBuilder::mutex_trylock(const SymAddrSize &ml){
  Mutex &mutex = mutexes[ml.addr];
  if (!record_symbolic(mutex.locked ? SymEv::MTryLockFail(ml) : SymEv::MTryLock(ml)))
    return false;
  curev().read_from = mutex.last_access;
  curev().may_conflict = true;

  mutex.last_access = prefix_idx;
  if(!mutex.locked){ // Mutex is free
    mutex.last_lock = prefix_idx;
    mutex.locked = true;
  }
  return true;
}

bool RFSCTraceBuilder::mutex_unlock(const SymAddrSize &ml){
  if (!record_symbolic(SymEv::MUnlock(ml))) return false;
  Mutex &mutex = mutexes[ml.addr];
  curev().read_from = mutex.last_access;
  curev().may_conflict = true;
  assert(0 <= mutex.last_access);

  mutex.last_access = prefix_idx;
  mutex.locked = false;

  /* No one is blocking anymore! Yay! */
  mutex_deadlocks.erase(ml.addr);
  return true;
}

bool RFSCTraceBuilder::mutex_init(const SymAddrSize &ml){
  if (!record_symbolic(SymEv::MInit(ml))) return false;
  assert(mutexes.count(ml.addr) == 0);
  curev().read_from = -1;
  curev().may_conflict = true;
  mutexes[ml.addr] = Mutex(prefix_idx);
  return true;
}

bool RFSCTraceBuilder::mutex_destroy(const SymAddrSize &ml){
  if (!record_symbolic(SymEv::MDelete(ml))) return false;
  Mutex &mutex = mutexes[ml.addr];
  curev().read_from = mutex.last_access;
  curev().may_conflict = true;

  mutex.last_access = prefix_idx;
  mutex.locked = false;
  return true;
}

bool RFSCTraceBuilder::cond_init(const SymAddrSize &ml){
  invalid_input_error("RFSC does not support condition variables");
  return false;
  if (!record_symbolic(SymEv::CInit(ml))) return false;
  if(cond_vars.count(ml.addr)){
    pthreads_error("Condition variable initiated twice.");
    return false;
  }
  curev().may_conflict = true;
  cond_vars[ml.addr] = CondVar(prefix_idx);
  return true;
}

bool RFSCTraceBuilder::cond_signal(const SymAddrSize &ml){
  invalid_input_error("RFSC does not support condition variables");
  return false;
  if (!record_symbolic(SymEv::CSignal(ml))) return false;
  curev().may_conflict = true;

  auto it = cond_vars.find(ml.addr);
  if(it == cond_vars.end()){
    pthreads_error("cond_signal called with uninitialized condition variable.");
    return false;
  }
  CondVar &cond_var = it->second;
  VecSet<int> seen_events = {last_full_memory_conflict};
  if(cond_var.waiters.size() > 1){
    if (!register_alternatives(cond_var.waiters.size())) return false;
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

  return true;
}

bool RFSCTraceBuilder::cond_broadcast(const SymAddrSize &ml){
  invalid_input_error("RFSC does not support condition variables");
  return false;
  if (!record_symbolic(SymEv::CBrdcst(ml))) return false;
  curev().may_conflict = true;

  auto it = cond_vars.find(ml.addr);
  if(it == cond_vars.end()){
    pthreads_error("cond_broadcast called with uninitialized condition variable.");
    return false;
  }
  CondVar &cond_var = it->second;
  for(int i : cond_var.waiters){
    assert(0 <= i && i < prefix_idx);
    IPid ipid = prefix[i].iid.get_pid();
    assert(!threads[ipid].available);
    threads[ipid].available = true;
    // seen_events.insert(i);
  }
  cond_var.waiters.clear();
  cond_var.last_signal = prefix_idx;

  return true;
}

bool RFSCTraceBuilder::cond_wait(const SymAddrSize &cond_ml, const SymAddrSize &mutex_ml){
  invalid_input_error("RFSC does not support condition variables");
  return false;
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

  if (!mutex_unlock(mutex_ml)) return false;
  if (!record_symbolic(SymEv::CWait(cond_ml))) return false;
  curev().may_conflict = true;

  IPid pid = curev().iid.get_pid();

  auto it = cond_vars.find(cond_ml.addr);
  if(it == cond_vars.end()){
    pthreads_error("cond_wait called with uninitialized condition variable.");
    return false;
  }
  it->second.waiters.push_back(prefix_idx);
  threads[pid].available = false;

  return true;
}

bool RFSCTraceBuilder::cond_awake(const SymAddrSize &cond_ml, const SymAddrSize &mutex_ml){
  invalid_input_error("RFSC does not support condition variables");
  return false;
  assert(cond_vars.count(cond_ml.addr));
  CondVar &cond_var = cond_vars[cond_ml.addr];
  add_happens_after(prefix_idx, cond_var.last_signal);

  if (!mutex_lock(mutex_ml)) return false;
  if (!record_symbolic(SymEv::CAwake(cond_ml))) return false;
  curev().may_conflict = true;

  return true;
}

int RFSCTraceBuilder::cond_destroy(const SymAddrSize &ml){
  invalid_input_error("RFSC does not support condition variables");
  return false;
  const int err = (EBUSY == 1) ? 2 : 1; // Chose an error value different from EBUSY
  if (!record_symbolic(SymEv::CDelete(ml))) return err;

  curev().may_conflict = true;

  auto it = cond_vars.find(ml.addr);
  if(it == cond_vars.end()){
    pthreads_error("cond_destroy called on uninitialized condition variable.");
    return err;
  }
  CondVar &cond_var = it->second;

  int rv = cond_var.waiters.size() ? EBUSY : 0;
  cond_vars.erase(ml.addr);
  return rv;
}

bool RFSCTraceBuilder::register_alternatives(int alt_count){
  invalid_input_error("RFSC does not support nondeterministic events");
  return false;
  curev().may_conflict = true;
  if (!record_symbolic(SymEv::Nondet(alt_count))) return false;
  // if(curev().alt == 0) {
  //   for(int i = curev().alt+1; i < alt_count; ++i){
  //     curev().races.push_back(Race::Nondet(prefix_idx, i));
  //   }
  // }
  return true;
}

template <class Iter>
static void rev_recompute_data
(SymData &data, VecSet<SymAddr> &needed, Iter end, Iter begin){
  for (auto pi = end; !needed.empty() && (pi != begin);){
    const SymEv &p = *(--pi);
    switch(p.kind){
    case SymEv::STORE:
    case SymEv::UNOBS_STORE:
    case SymEv::RMW:
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

void RFSCTraceBuilder::add_happens_after(unsigned second, unsigned first){
  assert(first != ~0u);
  assert(second != ~0u);
  assert(first != second);
  assert(first < second);
  assert((int_fast64_t)second <= prefix_idx);

  std::vector<unsigned> &vec = prefix[second].happens_after;
  if (vec.size() && vec.back() == first) return;

  vec.push_back(first);
}

void RFSCTraceBuilder::add_happens_after_thread(unsigned second, IPid thread){
  assert((int_fast64_t)second == prefix_idx);
  if (threads[thread].event_indices.empty()) return;
  add_happens_after(second, threads[thread].event_indices.back());
}

/* Filter the sequence first..last from all elements that are less than
 * any other item. The sequence is modified in place and an iterator to
 * the position beyond the last included element is returned.
 *
 * Assumes less is transitive and asymmetric (a strict partial order)
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

int RFSCTraceBuilder::compute_above_clock(unsigned i) {
  int last = -1;
  IPid ipid = prefix[i].iid.get_pid();
  int iidx = prefix[i].iid.get_index();
  if (iidx > 1) {
    last = find_process_event(ipid, iidx-1);
    prefix[i].clock = prefix[last].clock;
  } else {
    prefix[i].clock = VClock<IPid>();
    const Thread &t = threads[ipid];
    if (t.spawn_event >= 0)
      add_happens_after(i, t.spawn_event);
  }
  prefix[i].clock[ipid] = iidx;

  /* First add the non-reversible edges */
  for (unsigned j : prefix[i].happens_after){
    assert(j < i);
    prefix[i].clock += prefix[j].clock;
  }

  prefix[i].above_clock = prefix[i].clock;
  return last;
}

void RFSCTraceBuilder::compute_vclocks(){
  Timing::Guard timing_guard(vclocks_context);
  /* The first event of a thread happens after the spawn event that
   * created it.
   */
  std::vector<llvm::SmallVector<unsigned,2>> happens_after(prefix.size());
  for (unsigned i = 0; i < prefix.size(); i++){
    /* First add the non-reversible edges */
    int last = compute_above_clock(i);

    if (last != -1) happens_after[last].push_back(i);
    for (unsigned j : prefix[i].happens_after){
      happens_after[j].push_back(i);
    }

    /* Then add read-from */
    if (prefix[i].read_from && *prefix[i].read_from != -1) {
      prefix[i].clock += prefix[*prefix[i].read_from].clock;
      happens_after[*prefix[i].read_from].push_back(i);
    }
  }

  /* The top of our vector clock lattice, initial value of the below
   * clock (excluding itself) for every event */
  VClock<IPid> top;
  for (unsigned i = 0; i < threads.size(); ++i)
    top[i] = threads[i].event_indices.size();
  below_clocks.assign(threads.size(), prefix.size(), top);

  for (unsigned i = prefix.size();;) {
    if (i == 0) break;
    auto clock = below_clocks[--i];
    Event &e = prefix[i];
    clock[e.iid.get_pid()] = e.iid.get_index();
    for (unsigned j : happens_after[i]) {
      assert (i < j);
      clock -= below_clocks[j];
    }
  }
}

void RFSCTraceBuilder::compute_unfolding() {
  auto deepest_node = work_item;

  Timing::Guard timing_guard(unfolding_context);
  for (unsigned i = 0; i < prefix.size(); ++i) {
    auto last_decision = prefix[i].decision_ptr;
    if (last_decision && last_decision->depth > deepest_node->depth) {
      deepest_node = last_decision;
    }

    const RFSCUnfoldingTree::NodePtr null_ptr;
    const RFSCUnfoldingTree::NodePtr *parent;
    IID<IPid> iid = prefix[i].iid;
    IPid p = iid.get_pid();
    if (iid.get_index() == 1) {
      parent = &null_ptr;
    } else {
      int par_idx = find_process_event(p, iid.get_index()-1);
      parent = &prefix[par_idx].event;
    }
    if (!prefix[i].may_conflict && (*parent != nullptr)) {
      prefix[i].event = *parent;
      continue;
    }
    const RFSCUnfoldingTree::NodePtr *read_from = &null_ptr;
    if (prefix[i].read_from && *prefix[i].read_from != -1) {
      read_from = &prefix[*prefix[i].read_from].event;
    }

    prefix[i].event = unfolding_tree.find_unfolding_node
      (threads[p].cpid, *parent, *read_from);

    if (int(i) >= replay_point) {
      if (is_load(i)) {
        const RFSCUnfoldingTree::NodePtr &decision
        = is_lock_type(i) ? prefix[i].event : prefix[i].event->read_from;
        deepest_node = decision_tree.new_decision_node(std::move(deepest_node), decision);
        prefix[i].set_decision(deepest_node);
      }
    }
  }
  work_item = std::move(deepest_node);
}

RFSCUnfoldingTree::NodePtr RFSCTraceBuilder::
unfold_find_unfolding_node(IPid p, int index, Option<int> prefix_rf) {
  const RFSCUnfoldingTree::NodePtr null_ptr;
  const RFSCUnfoldingTree::NodePtr *parent;
  if (index == 1) {
    parent = &null_ptr;
  } else {
    int par_idx = find_process_event(p, index-1);
    parent = &prefix[par_idx].event;
  }
  const RFSCUnfoldingTree::NodePtr *read_from = &null_ptr;
  if (prefix_rf && *prefix_rf != -1) {
    read_from = &prefix[*prefix_rf].event;
  }
  return unfolding_tree.find_unfolding_node(threads[p].cpid, *parent,
                                            *read_from);
}


RFSCUnfoldingTree::NodePtr
RFSCTraceBuilder::unfold_alternative
(unsigned i, const RFSCUnfoldingTree::NodePtr &read_from) {
  const RFSCUnfoldingTree::NodePtr &parent
    = prefix[i].event->parent;
  IPid p = prefix[i].iid.get_pid();

  return unfolding_tree.find_unfolding_node(threads[p].cpid, parent, read_from);
}

bool RFSCTraceBuilder::record_symbolic(SymEv event){
  if (!replay) {
    assert(!seen_effect);
    /* New event */
    curev().sym = std::move(event);
    seen_effect = true;
  } else {
    /* Replay. SymEv::set() asserts that this is the same event as last time. */
    SymEv &last = curev().sym;
    assert(!seen_effect);
    if (!last.is_compatible_with(event)) {
      auto pid_str = [this](IPid p) {
          return p >= threads.size() ? std::to_string(p)
              : threads[p].cpid.to_string();
      };
      nondeterminism_error("Event with effect " + last.to_string(pid_str)
                           + " became " + event.to_string(pid_str)
                           + " when replayed");
      return false;
    }
    last = event;
    seen_effect = true;
  }
  return true;
}

static bool symev_is_lock_type(const SymEv &e) {
  return e.kind == SymEv::M_INIT  || e.kind == SymEv::M_LOCK
    || e.kind == SymEv::M_TRYLOCK || e.kind == SymEv::M_TRYLOCK_FAIL
    || e.kind == SymEv::M_UNLOCK  || e.kind == SymEv::M_DELETE;
}

bool RFSCTraceBuilder::is_load(unsigned i) const {
  const SymEv &e = prefix[i].sym;
  return e.kind == SymEv::LOAD || e.kind == SymEv::RMW
    || e.kind == SymEv::CMPXHG || e.kind == SymEv::CMPXHGFAIL
    || symev_is_lock_type(e);
}

bool RFSCTraceBuilder::is_lock(unsigned i) const {
  return prefix[i].sym.kind == SymEv::M_LOCK;
}

bool RFSCTraceBuilder::does_lock(unsigned i) const {
  auto kind = prefix[i].sym.kind;
  return kind == SymEv::M_LOCK || kind == SymEv::M_TRYLOCK;
}

bool RFSCTraceBuilder::is_trylock_fail(unsigned i) const {
  return prefix[i].sym.kind == SymEv::M_TRYLOCK_FAIL;
}

bool RFSCTraceBuilder::is_unlock(unsigned i) const {
  return prefix[i].sym.kind == SymEv::M_UNLOCK;
}

bool RFSCTraceBuilder::is_minit(unsigned i) const {
  return prefix[i].sym.kind == SymEv::M_INIT;
}

bool RFSCTraceBuilder::is_mdelete(unsigned i) const {
  return prefix[i].sym.kind == SymEv::M_DELETE;
}

bool RFSCTraceBuilder::is_lock_type(unsigned i) const {
  return symev_is_lock_type(prefix[i].sym);
}

bool RFSCTraceBuilder::is_store(unsigned i) const {
  const SymEv &e = prefix[i].sym;
  return e.kind == SymEv::STORE || e.kind == SymEv::CMPXHG
    || e.kind == SymEv::RMW || symev_is_lock_type(e);
}

bool RFSCTraceBuilder::is_cmpxhgfail(unsigned i) const {
  return prefix[i].sym.kind == SymEv::CMPXHGFAIL;
}

bool RFSCTraceBuilder::is_store_when_reading_from(unsigned i, int read_from) const {
  const SymEv &e = prefix[i].sym;
  if (e.kind == SymEv::STORE || e.kind == SymEv::RMW || symev_is_lock_type(e))
    return true;
  if (e.kind != SymEv::CMPXHG && e.kind != SymEv::CMPXHGFAIL) return false;
  SymData expected = e.expected();
  SymData actual = get_data(read_from, e.addr());
  assert(e.addr() == actual.get_ref());
  return memcmp(expected.get_block(), actual.get_block(), e.addr().size) == 0;
}

SymAddrSize RFSCTraceBuilder::get_addr(unsigned i) const {
  const SymEv &e = prefix[i].sym;
  if (e.has_addr()) {
    return e.addr();
  }
  abort();
}

SymData RFSCTraceBuilder::get_data(int i, const SymAddrSize &addr) const {
  if (i == -1) {
    SymData ret(addr, addr.size);
    memset(ret.get_block(), 0, addr.size);
    return ret;
  }
  const SymEv &e = prefix[i].sym;
  assert(e.has_data());
  assert(e.addr() == addr);
  return e.data();
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

RFSCTraceBuilder::CmpXhgUndoLog RFSCTraceBuilder::
recompute_cmpxhg_success(unsigned idx, std::vector<int> &writes) {
  auto kind = CmpXhgUndoLog::NONE;
  SymEv &e =  prefix[idx].sym;
  if (e.kind == SymEv::M_TRYLOCK || e.kind == SymEv::M_TRYLOCK_FAIL) {
    bool before = e.kind == SymEv::M_TRYLOCK;
    bool after = *prefix[idx].read_from == -1 || !does_lock(*prefix[idx].read_from);
    unsigned pos = 0;
    if (after && !before) {
      e = SymEv::MTryLock(e.addr());
      kind = CmpXhgUndoLog::M_FAIL;
      pos = writes.size();
      writes.push_back(idx);
    } else if (before && !after) {
      e = SymEv::MTryLockFail(e.addr());
      kind = CmpXhgUndoLog::M_SUCCEED;
      pos = delete_from_back(writes, idx);
    }

    return CmpXhgUndoLog{kind, idx, pos, &e};
  }
  if (e.kind != SymEv::CMPXHG && e.kind != SymEv::CMPXHGFAIL) {
      return CmpXhgUndoLog{CmpXhgUndoLog::NONE, idx, 0, nullptr};
  }
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
  unsigned pos = 0;
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

void RFSCTraceBuilder::
undo_cmpxhg_recomputation(CmpXhgUndoLog log, std::vector<int> &writes) {
  if (log.kind == CmpXhgUndoLog::NONE) return;
  SymEv &e = *log.e;
  switch (log.kind) {
  case CmpXhgUndoLog::M_SUCCEED:
    e = SymEv::MTryLock(e.addr());
    goto succeed_cont;
  case CmpXhgUndoLog::M_FAIL:
    e = SymEv::MTryLockFail(e.addr());
    goto fail_cont;
  case CmpXhgUndoLog::SUCCEED: {
    e = SymEv::CmpXhg(e.data(), e.expected().get_shared_block());
  succeed_cont:
    writes.push_back(log.idx);
    assert(writes.size() > log.pos);
    std::swap(writes.back(), writes[log.pos]);
  } break;
  default: assert(false && "Unreachable");
  case CmpXhgUndoLog::FAIL: {
    e = SymEv::CmpXhgFail(e.data(), e.expected().get_shared_block());
  fail_cont:
    assert(writes.back() == int(log.idx));
    assert(writes.size()-1 == log.pos);
    writes.pop_back();
  } break;
  }
}

bool RFSCTraceBuilder::happens_before(const Event &e,
                                     const VClock<int> &c) const {
  return c.includes(e.iid);
}

bool RFSCTraceBuilder::can_rf_by_vclocks
(int r, int ow, int w) const {
  /* Is the write after the read? */
  if (w != -1 && happens_before(prefix[r], prefix[w].clock)) abort();

  /* Is the original write always before the read, and the new write
   * before the original?
   */
  if (ow != -1 && (w == -1 || happens_before(prefix[w], prefix[ow].clock))) {
    if (happens_before(prefix[ow], prefix[r].above_clock)) return false;
  }

  return true;
}

bool RFSCTraceBuilder::can_swap_by_vclocks(int r, int w) const {
  if (happens_before(prefix[r], prefix[w].above_clock)) return false;
  return true;
}

bool RFSCTraceBuilder::can_swap_lock_by_vclocks(int f, int u, int s) const {
  if (happens_before(prefix[f], prefix[s].above_clock)) return false;
  return true;
}

void RFSCTraceBuilder::compute_prefixes() {
  Timing::Guard analysis_timing_guard(analysis_context);
  compute_vclocks();

  compute_unfolding();

  if(conf.debug_print_on_reset){
    llvm::dbgs() << " === RFSCTraceBuilder state ===\n";
    debug_print();
    llvm::dbgs() << " ==============================\n";
  }

  Timing::Guard neighbours_timing_guard(neighbours_context);
  if (conf.debug_print_on_reset)
    llvm::dbgs() << "Computing prefixes\n";

  auto pretty_index = [&] (int i) -> std::string {
    if (i == -1) return "init event";
    return std::to_string(prefix[i].get_decision_depth()) + "("
      + std::to_string(prefix[i].event->seqno) + "):"
      + iid_string(i) + prefix[i].sym.to_string();
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
    auto try_swap = [&](int i, int j) {
        int original_read_from = *prefix[i].read_from;
        RFSCUnfoldingTree::NodePtr alt
          = unfold_alternative(j, prefix[i].event->read_from);
        DecisionNode &decision = *prefix[i].decision_ptr;
        // Returns false if unfolding node is already known and therefore does not have to be further evaluated
        if (!decision.try_alloc_unf(alt)) return;
        if (!can_swap_by_vclocks(i, j)) return;
        if (conf.debug_print_on_reset) {
          llvm::dbgs() << "Trying to swap " << pretty_index(i)
                       << " and " << pretty_index(j)
                       << ", after " << pretty_index(original_read_from);
        }
        prefix[j].read_from = original_read_from;
        prefix[i].decision_swap(prefix[j]);

        Leaf solution = try_sat({unsigned(j)}, writes_by_address);
        if (!solution.is_bottom()) {
          decision_tree.construct_sibling(decision, std::move(alt),
                                          std::move(solution));
          tasks_created++;
        }
        /* Reset read-from and decision */
        prefix[j].read_from = i;
        prefix[i].decision_swap(prefix[j]);
      };
    auto try_swap_lock = [&](int i, int unlock, int j) {
        assert(does_lock(i) && is_unlock(unlock) && is_lock(j)
               && *prefix[j].read_from == int(unlock));
        RFSCUnfoldingTree::NodePtr alt
          = unfold_alternative(j, prefix[i].event->read_from);
        DecisionNode &decision = *prefix[i].decision_ptr;
        if (!decision.try_alloc_unf(alt)) return;
        if (!can_swap_lock_by_vclocks(i, unlock, j)) return;
        int original_read_from = *prefix[i].read_from;
        if (conf.debug_print_on_reset)
          llvm::dbgs() << "Trying to swap " << pretty_index(i)
                       << " and " << pretty_index(j)
                       << ", after " << pretty_index(original_read_from);
        prefix[j].read_from = original_read_from;
        prefix[i].decision_swap(prefix[j]);

        Leaf solution = try_sat({unsigned(j)}, writes_by_address);
        if (!solution.is_bottom()) {
          decision_tree.construct_sibling(decision, std::move(alt),
                                          std::move(solution));
          tasks_created++;
        }
        /* Reset read-from and decision */
        prefix[j].read_from = unlock;
        prefix[i].decision_swap(prefix[j]);
      };
    auto try_swap_blocked = [&](int i, IPid jp, SymEv sym) {
        int original_read_from = *prefix[i].read_from;
        auto jidx = threads[jp].last_event_index()+1;
        RFSCUnfoldingTree::NodePtr alt
          = unfold_find_unfolding_node(jp, jidx, original_read_from);
        DecisionNode &decision = *prefix[i].decision_ptr;
        if (!decision.try_alloc_unf(alt)) return;
        int j = ++prefix_idx;
        assert(prefix.size() == j);
        prefix.emplace_back(IID<IPid>(jp, jidx), 0, std::move(sym));
        prefix[j].event = alt; // Only for debug print
        threads[jp].event_indices.push_back(j); // Not needed?
        compute_above_clock(j);

        if (can_swap_by_vclocks(i, j)) {

          if (conf.debug_print_on_reset)
            llvm::dbgs() << "Trying replace " << pretty_index(i)
                         << " with deadlocked " << pretty_index(j)
                         << ", after " << pretty_index(original_read_from);
          prefix[j].read_from = original_read_from;
          prefix[i].decision_swap(prefix[j]);
          assert(!prefix[i].pinned);
          prefix[i].pinned = true;

          Leaf solution = try_sat({unsigned(j)}, writes_by_address);
          if (!solution.is_bottom()) {
            decision_tree.construct_sibling(decision, std::move(alt),
                                            std::move(solution));
            tasks_created++;
          }

          /* Reset decision */
          prefix[i].decision_swap(prefix[j]);
          prefix[i].pinned = false;
        }
        /* Delete j */
        threads[jp].event_indices.pop_back();
        prefix.pop_back();
        --prefix_idx;
      };
    if (!prefix[i].pinned && is_lock_type(i)) {
      Timing::Guard ponder_mutex_guard(ponder_mutex_context);
      auto addr = get_addr(i);
      const std::vector<int> &accesses = writes_by_address[addr.addr];
      auto next = std::upper_bound(accesses.begin(), accesses.end(), i);
      if (next == accesses.end()) {
        if (does_lock(i)) {
          for (IPid other : mutex_deadlocks[addr.addr]) {
            try_swap_blocked(i, other, SymEv::MLock(addr));
          }
        }
        continue;
      }
      unsigned j = *next;
      assert(*prefix[j].read_from == int(i));
      if (is_unlock(i) && is_lock(j)) continue;

      try_swap(i, j);

      if (!does_lock(i)) continue;
      while (is_trylock_fail(*next)) {
        if (++next == accesses.end()) break;
      }
      if (next == accesses.end() || !is_unlock(*next)) continue;
      unsigned unlock = *next;
      if (++next == accesses.end()) continue;
      unsigned relock = *next;
      if (!is_lock(relock)) continue;
      try_swap_lock(i, unlock, relock);
    } else if (!prefix[i].pinned && is_load(i)) {
      auto addr = get_addr(i);
      std::vector<int> &possible_writes = writes_by_address[addr.addr];
      int original_read_from = *prefix[i].read_from;
      assert(std::any_of(possible_writes.begin(), possible_writes.end(),
             [=](int i) { return i == original_read_from; })
           || original_read_from == -1);

      DecisionNode &decision = *prefix[i].decision_ptr;

      auto try_read_from_rmw = [&](int j) {
        Timing::Guard analysis_timing_guard(try_read_from_context);
        assert(j != -1 && j > int(i) && is_store(i) && is_load(j)
               && is_store_when_reading_from(j, original_read_from));
        /* Can only swap adjacent RMWs */
        if (*prefix[j].read_from != int(i)) return;
        RFSCUnfoldingTree::NodePtr read_from
          = unfold_alternative(j, prefix[i].event->read_from);
        if (!decision.try_alloc_unf(read_from)) return;
        if (!can_swap_by_vclocks(i, j)) return;
        if (conf.debug_print_on_reset)
          llvm::dbgs() << "Trying to swap " << pretty_index(i)
                       << " and " << pretty_index(j)
                       << ", reading from " << pretty_index(original_read_from);
        prefix[j].read_from = original_read_from;
        prefix[i].read_from = j;
        auto undoj = recompute_cmpxhg_success(j, possible_writes);
        auto undoi = recompute_cmpxhg_success(i, possible_writes);

        Leaf solution = try_sat({unsigned(j), i}, writes_by_address);
        if (!solution.is_bottom()) {
          decision_tree.construct_sibling(decision, std::move(read_from),
                                          std::move(solution));
          tasks_created++;
        }

        /* Reset read-from */
        prefix[j].read_from = i;
        undo_cmpxhg_recomputation(undoi, possible_writes);
        undo_cmpxhg_recomputation(undoj, possible_writes);
      };
      auto try_read_from = [&](int j) {
        Timing::Guard analysis_timing_guard(try_read_from_context);
        if (j == original_read_from || j == int(i)) return;
        if (j != -1 && j > int(i) && is_store(i) && is_load(j)) {
          if (!is_store_when_reading_from(j, original_read_from)) return;
          /* RMW pair */
          try_read_from_rmw(j);
        } else {
          const RFSCUnfoldingTree::NodePtr &read_from =
            j == -1 ? nullptr : prefix[j].event;
          if (!decision.try_alloc_unf(read_from)) return;
          if (!can_rf_by_vclocks(i, original_read_from, j)) return;
          if (conf.debug_print_on_reset)
            llvm::dbgs() << "Trying to make " << pretty_index(i)
                         << " read from " << pretty_index(j)
                         << " instead of " << pretty_index(original_read_from);
          prefix[i].read_from = j;
          auto undoi = recompute_cmpxhg_success(i, possible_writes);

          Leaf solution = try_sat({i}, writes_by_address);
          if (!solution.is_bottom()) {
            decision_tree.construct_sibling(decision, read_from, std::move(solution));
            tasks_created++;
          }

          undo_cmpxhg_recomputation(undoi, possible_writes);
        }
      };

      const VClock<IPid> &above = prefix[i].above_clock;
      const auto below = below_clocks[i];
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

void RFSCTraceBuilder::output_formula
(SatSolver &sat,
 std::map<SymAddr,std::vector<int>> &writes_by_address,
 const std::vector<bool> &keep){
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

template<typename T, typename F> auto map(const std::vector<T> &vec, F f)
  -> std::vector<typename std::result_of<F(const T&)>::type> {
  std::vector<typename std::result_of<F(const T&)>::type> ret;
  ret.reserve(vec.size());
  for (const T &e : vec) ret.emplace_back(f(e));
  return ret;
}

void RFSCTraceBuilder::add_event_to_graph(SaturatedGraph &g, unsigned i) const {
  SaturatedGraph::EventKind kind = SaturatedGraph::NONE;
  SymAddr addr;
  if (is_load(i))
    kind = (is_store(i)) ? SaturatedGraph::RMW : SaturatedGraph::LOAD;
  else if (is_store(i))
    kind = SaturatedGraph::STORE;
  if (kind != SaturatedGraph::NONE) addr = get_addr(i).addr;
  Option<IID<IPid>> read_from;
  if (prefix[i].read_from && *prefix[i].read_from != -1)
    read_from = prefix[*prefix[i].read_from].iid;
  IID<IPid> iid = prefix[i].iid;
  g.add_event(iid.get_pid(), iid, kind, addr,
              read_from, map(prefix[i].happens_after,
                             [this](unsigned j){return prefix[j].iid;}));
}

const SaturatedGraph &RFSCTraceBuilder::get_cached_graph
(DecisionNode &decision) {
  const int depth = decision.depth;
  return decision.get_saturated_graph(
    [depth, this](SaturatedGraph &g) {
      std::vector<bool> keep = causal_past(depth-1);
      for (unsigned i = 0; i < prefix.size(); ++i) {
        if (keep[i] && !g.has_event(prefix[i].iid)) {
          add_event_to_graph(g, i);
        }
      }

      IFDEBUG(bool g_acyclic = ) g.saturate();
      assert(g_acyclic);
    });
}

Leaf
RFSCTraceBuilder::try_sat
(std::initializer_list<unsigned> changed_events,
 std::map<SymAddr,std::vector<int>> &writes_by_address){
  Timing::Guard timing_guard(graph_context);
  unsigned last_change = changed_events.end()[-1];
  DecisionNode &decision = *prefix[last_change].decision_ptr;
  int decision_depth = decision.depth;
  std::vector<bool> keep = causal_past(decision_depth);

  SaturatedGraph g(get_cached_graph(decision).clone());
  for (unsigned i = 0; i < prefix.size(); ++i) {
    if (keep[i] && i != last_change && !g.has_event(prefix[i].iid)) {
      add_event_to_graph(g, i);
    }
  }
  add_event_to_graph(g, last_change);
  if (!g.saturate()) {
    if (conf.debug_print_on_reset)
      llvm::dbgs() << ": Saturation yielded cycle\n";
    return Leaf();
  }
  std::vector<IID<int>> current_exec
    = map(prefix, [](const Event &e) { return e.iid; });
  /* We need to preserve g */
  if (Option<std::vector<IID<int>>> res
      = try_generate_prefix(std::move(g), std::move(current_exec))) {
    if (conf.debug_print_on_reset) {
      llvm::dbgs() << ": Heuristic found prefix\n";
      llvm::dbgs() << "[";
      for (IID<int> iid : *res) {
        llvm::dbgs() << iid_string(iid) << ",";
      }
      llvm::dbgs() << "]\n";
    }
    std::vector<unsigned> order = map(*res, [this](IID<int> iid) {
        return find_process_event(iid.get_pid(), iid.get_index());
      });
    return order_to_leaf(decision_depth, changed_events, std::move(order));
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

  return order_to_leaf(decision_depth, changed_events, std::move(order));
}

Leaf RFSCTraceBuilder::order_to_leaf
(int decision, std::initializer_list<unsigned> changed,
 const std::vector<unsigned> order) const{
  std::vector<Branch> new_prefix;
  new_prefix.reserve(order.size());
  for (unsigned i : order) {
    bool is_changed = std::any_of(changed.begin(), changed.end(),
                                  [i](unsigned c) { return c == i; });
    bool new_pinned = prefix[i].pinned || (prefix[i].get_decision_depth() > decision);
    int new_decision = prefix[i].get_decision_depth();
    if (new_decision > decision) {
      new_pinned = true;
      new_decision = -1;
    }
    new_prefix.emplace_back(prefix[i].iid.get_pid(),
                            is_changed ? 1 : prefix[i].size,
                            new_decision,
                            new_pinned,
                            prefix[i].sym);
  }

  return Leaf(new_prefix);
}

std::vector<bool> RFSCTraceBuilder::causal_past(int decision) const {
  std::vector<bool> acc(prefix.size());
  for (unsigned i = 0; i < prefix.size(); ++i) {
    assert(!((prefix[i].get_decision_depth() != -1) && prefix[i].pinned));
    assert(is_load(i) == ((prefix[i].get_decision_depth() != -1) || prefix[i].pinned));
    if (prefix[i].get_decision_depth() != -1 && prefix[i].get_decision_depth() <= decision) {
      causal_past_1(acc, i);
    }
  }
  return acc;
}

void RFSCTraceBuilder::causal_past_1(std::vector<bool> &acc, unsigned i) const{
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

std::vector<int> RFSCTraceBuilder::iid_map_at(int event) const{
  std::vector<int> map(threads.size(), 1);
  for (int i = 0; i < event; ++i) {
    iid_map_step(map, prefix[i]);
  }
  return map;
}

void RFSCTraceBuilder::iid_map_step(std::vector<int> &iid_map, const Event &event) const{
  if (iid_map.size() <= unsigned(event.iid.get_pid())) iid_map.resize(event.iid.get_pid()+1, 1);
  iid_map[event.iid.get_pid()] += event.size;
}

void RFSCTraceBuilder::iid_map_step_rev(std::vector<int> &iid_map, const Event &event) const{
  iid_map[event.iid.get_pid()] -= event.size;
}

inline Option<unsigned> RFSCTraceBuilder::
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

inline unsigned RFSCTraceBuilder::find_process_event(IPid pid, int index) const{
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

long double RFSCTraceBuilder::estimate_trace_count() const{
  return estimate_trace_count(0);
}

bool RFSCTraceBuilder::check_for_cycles() {
  return false;
}

long double RFSCTraceBuilder::estimate_trace_count(int idx) const{
  if(idx > int(prefix.size())) return 0;
  if(idx == int(prefix.size())) return 1;

  long double count = 42;
  for(int i = int(prefix.size())-1; idx <= i; --i){
    count += prefix[i].sleep_branch_trace_count;
    // count += std::max(0, int(prefix.children_after(i)))
    //   * (count / (1 + prefix[i].sleep.size()));
  }

  return count;
}
