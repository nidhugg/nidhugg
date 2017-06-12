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
  replay_point = 0;
}

TSOTraceBuilder::~TSOTraceBuilder(){
}

bool TSOTraceBuilder::schedule(int *proc, int *aux, int *alt, bool *dryrun){
  *dryrun = false;
  *alt = 0;
  this->dryrun = false;
  if(replay){
    /* Are we done with the current Event? */
    if(0 <= prefix_idx && threads[curev().iid.get_pid()].clock[curev().iid.get_pid()] <
       curev().iid.get_index() + curbranch().size - 1){
      /* Continue executing the current Event */
      IPid pid = curev().iid.get_pid();
      *proc = pid/2;
      *aux = pid % 2 - 1;
      *alt = 0;
      assert(threads[pid].available);
      ++threads[pid].clock[pid];
      return true;
    }else if(prefix_idx + 1 == int(prefix.len()) && prefix.lastnode().size() == 0){
      /* We are done replaying. Continue below... */
      assert(prefix_idx < 0 || curev().sym.size() == sym_idx);
      replay = false;
      assert(conf.dpor_algorithm != Configuration::OPTIMAL
            || std::all_of(threads.cbegin(), threads.cend(),
                           [](const Thread &t) { return !t.sleeping; }));
    }else if(prefix_idx + 1 != int(prefix.len()) &&
             dry_sleepers < int(prefix[prefix_idx+1].sleep.size())){
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
      assert(prefix_idx < 0 || curev().sym.size() == sym_idx);
      dry_sleepers = 0;
      sym_idx = 0;
      ++prefix_idx;
      IPid pid;
      if (prefix_idx < int(prefix.len())) {
        /* The event is already in prefix */
        pid = curev().iid.get_pid();
      } else {
        /* We are replaying from the wakeup tree */
        pid = prefix.first_child().pid;
        prefix.enter_first_child
          (Event(IID<IPid>(pid,threads[pid].clock[pid] + 1), {}));
      }
      *proc = pid/2;
      *aux = pid % 2 - 1;
      *alt = curbranch().alt;
      assert(threads[pid].available);
      ++threads[pid].clock[pid];
      curev().clock = threads[pid].clock;
      return true;
    }
  }

  assert(!replay);
  /* Create a new Event */

  assert(prefix_idx < 0 || !!curev().sym.size() == curev().may_conflict);

  /* Should we merge the last two events? */
  if(prefix.len() > 1 &&
     prefix[prefix.len()-1].iid.get_pid()
     == prefix[prefix.len()-2].iid.get_pid() &&
     !prefix[prefix.len()-1].may_conflict &&
     prefix[prefix.len()-1].sleep.empty()){
    assert(prefix.children_after(prefix.len()-1) == 0);
    assert(prefix[prefix.len()-1].wakeup.empty());
    prefix.delete_last();
    --prefix_idx;
    Branch b = curbranch();
    ++b.size;
    prefix.set_last_branch(std::move(b));
  }

  /* Create a new Event */
  sym_idx = 0;
  ++prefix_idx;
  assert(prefix_idx == int(prefix.len()));

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
      prefix.push(Branch(IPid(p)),
                  Event(IID<IPid>(IPid(p),threads[p].clock[p]),
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
      prefix.push(Branch(IPid(p)),
                  Event(IID<IPid>(IPid(p),threads[p].clock[p]),
                        threads[p].clock));
      *proc = p/2;
      *aux = -1;
      return true;
    }
  }

  do_race_detect();

  return false; // No available threads
}

void TSOTraceBuilder::refuse_schedule(){
  assert(prefix_idx == int(prefix.len())-1);
  assert(prefix.lastbranch().size == 1);
  assert(!prefix.last().may_conflict);
  assert(prefix.last().sleep.empty());
  assert(prefix.last().wakeup.empty());
  assert(prefix.children_after(prefix_idx) == 0);
  IPid last_pid = prefix.last().iid.get_pid();
  prefix.delete_last();
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

bool TSOTraceBuilder::is_replaying() const {
  return prefix_idx < replay_point;
}

void TSOTraceBuilder::cancel_replay(){
  if(!replay) return;
  replay = false;
  while (prefix_idx + 1 < int(prefix.len())) prefix.delete_last();
  if (prefix.node(prefix_idx).size()) {
    prefix.enter_first_child(Event(IID<IPid>(), VClock<IPid>()));
    prefix.delete_last();
  }
}

void TSOTraceBuilder::metadata(const llvm::MDNode *md){
  if(!dryrun && curev().md == 0){
    curev().md = md;
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
    assert(prefix.len());
    IID<CPid> c_iid(threads[i_iid.get_pid()].cpid,i_iid.get_index());
    errors.push_back(new RobustnessError(c_iid));
  }

  return true;
}

Trace *TSOTraceBuilder::get_trace() const{
  std::vector<IID<CPid> > cmp;
  std::vector<const llvm::MDNode*> cmp_md;
  std::vector<Error*> errs;
  for(unsigned i = 0; i < prefix.len(); ++i){
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
  for(i = int(prefix.len())-1; 0 <= i; --i){
    if(prefix.children_after(i)){
      break;
    }
  }

  if(i < 0){
    /* No more branching is possible. */
    return false;
  }
  replay_point = i;

  /* Setup the new Event at prefix[i] */
  {
    int sleep_branch_trace_count =
      prefix[i].sleep_branch_trace_count + estimate_trace_count(i+1);
    Event prev_evt = std::move(prefix[i]);
    while (ssize_t(prefix.len()) > i) prefix.delete_last();

    Branch br = prefix.first_child();

    /* Find the index of br.pid. */
    int br_idx = 1;
    for(int j = i-1; br_idx == 1 && 0 <= j; --j){
      if(prefix[j].iid.get_pid() == br.pid){
        br_idx = prefix[j].iid.get_index() + prefix.branch(j).size;
      }
    }

    Event evt(IID<IPid>(br.pid,br_idx),{});

    evt.sleep = prev_evt.sleep;
    if(br.pid != prev_evt.iid.get_pid()){
      evt.sleep.insert(prev_evt.iid.get_pid());
    }
    evt.sleep_branch_trace_count = sleep_branch_trace_count;

    prefix.enter_first_child(std::move(evt));
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
  reset_cond_branch_log();

  return true;
}

IID<CPid> TSOTraceBuilder::get_iid() const{
  IPid pid = curev().iid.get_pid();
  int idx = curev().iid.get_index();
  return IID<CPid>(threads[pid].cpid,idx);
}

static std::string rpad(std::string s, int n){
  while(int(s.size()) < n) s += " ";
  return s;
}

std::string TSOTraceBuilder::iid_string(std::size_t pos) const{
  return iid_string(prefix.branch(pos), prefix[pos].iid.get_index());
}

std::string TSOTraceBuilder::iid_string(const Branch &branch, int index) const{
  std::stringstream ss;
  ss << "(" << threads[branch.pid].cpid << "," << index;
  if(branch.size > 1){
    ss << "-" << index + branch.size - 1;
  }
  ss << ")";
  if(branch.alt != 0){
    ss << "-alt:" << branch.alt;
  }
  return ss.str();
}

std::string TSOTraceBuilder::slp_string(const VecSet<IPid> &slp) const {
  std::string res = "{";
  for(int i = 0; i < slp.size(); ++i){
    if(i != 0) res += ", ";
    res += threads[slp[i]].cpid.to_string();
  }
  res += "}";
  return res;
}

/* For debug-printing the wakeup tree; adds a node and its children to lines */
void TSOTraceBuilder::wut_string_add_node
(std::vector<std::string> &lines, std::vector<int> &iid_map,
 unsigned line, Branch branch, WakeupTreeRef<Branch> node) const{
  unsigned offset = 2 + ((lines.size() < line)?0:lines[line].size());

  std::vector<std::pair<Branch,WakeupTreeRef<Branch>>> nodes({{branch,node}});
  iid_map[branch.pid] += branch.size;
  unsigned l = line;
  WakeupTreeRef<Branch> n = node;
  Branch b = branch;
  while (n.size()) {
    b = n.begin().branch();
    n = n.begin().node();
    ++l;
    nodes.push_back({b,n});
    iid_map[b.pid] += b.size;
    if (l < lines.size()) offset = std::max(offset, unsigned(lines[l].size()));
  }
  if (lines.size() < l+1) lines.resize(l+1, "");
  /* First node needs different padding, so we do it here*/
  lines[line] += " ";
  while(lines[line].size() < offset) lines[line] += "-";

  while(nodes.size()) {
    l = line+nodes.size()-1;
    b = nodes.back().first;
    n = nodes.back().second;
    for (auto ci = n.begin(); ci != n.end(); ++ci) {
      if (ci == n.begin()) continue;
      wut_string_add_node(lines, iid_map, l+1, ci.branch(), ci.node());
    }
    iid_map[b.pid] -= b.size;
    while(lines[l].size() < offset) lines[l] += " ";
    lines[l] += " " + iid_string(b, iid_map[b.pid]);
    nodes.pop_back();
  }
}

void TSOTraceBuilder::debug_print() const {
  llvm::dbgs() << "TSOTraceBuilder (debug print):\n";
  int iid_offs = 0;
  int clock_offs = 0;
  std::vector<std::string> lines;
  VecSet<IPid> sleep_set;
  for(unsigned i = 0; i < prefix.len(); ++i){
    IPid ipid = prefix[i].iid.get_pid();
    iid_offs = std::max(iid_offs,2*ipid+int(iid_string(i).size()));
    clock_offs = std::max(clock_offs,int(prefix[i].clock.to_string().size()));
    sleep_set.insert(prefix[i].sleep);
    lines.push_back(" SLP:" + slp_string(sleep_set));
    sleep_set.erase(prefix[i].wakeup);
  }

  /* Add wakeup tree */
  std::vector<int> iid_map = iid_map_at(prefix.len());
  for(int i = prefix.len()-1; 0 <= i; --i){
    auto node = prefix.parent_at(i);
    iid_map[prefix.branch(i).pid] -= prefix.branch(i).size;
    for (auto it = node.begin(); it != node.end(); ++it) {
      Branch b = it.branch();
      if (b == prefix.branch(i)) continue; /* Only print others */
      wut_string_add_node(lines, iid_map, i, it.branch(), it.node());
    }
  }

  for(unsigned i = 0; i < prefix.len(); ++i){
    IPid ipid = prefix[i].iid.get_pid();
    llvm::dbgs() << rpad("",2+ipid*2)
                 << rpad(iid_string(i),iid_offs-ipid*2)
                 << " " << rpad(prefix[i].clock.to_string(),clock_offs)
                 << lines[i] << "\n";
  }
  for (unsigned i = prefix.len(); i < lines.size(); ++i){
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

void TSOTraceBuilder::spawn(){
  IPid parent_ipid = curev().iid.get_pid();
  CPid child_cpid = CPS.spawn(threads[parent_ipid].cpid);
  threads.push_back(Thread(child_cpid,threads[parent_ipid].clock));
  threads.push_back(Thread(CPS.new_aux(child_cpid),threads[parent_ipid].clock));
  threads.back().available = false; // Empty store buffer
}

void TSOTraceBuilder::store(const ConstMRef &ml){
  if(dryrun) return;
  IPid ipid = curev().iid.get_pid();
  threads[ipid].store_buffer.push_back(PendingStore(ml,threads[ipid].clock,last_md));
  threads[ipid+1].available = true;
}

void TSOTraceBuilder::atomic_store(const ConstMRef &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    VecSet<void const*> &A = threads[pid].sleep_accesses_w;
    for(void const *b : ml){
      A.insert(b);
    }
    return;
  }
  IPid ipid = curev().iid.get_pid();
  curev().may_conflict = true;
  record_symbolic(SymEv::Store(ml));
  bool is_update = ipid % 2;

  IPid uipid = ipid; // ID of the thread changing the memory
  IPid tipid = is_update ? ipid-1 : ipid; // ID of the (real) thread that issued the store

  if(is_update){ // Add the clock of the store instruction
    assert(threads[tipid].store_buffer.size());
    const PendingStore &pst = threads[tipid].store_buffer.front();
    curev().clock += pst.clock;
    threads[uipid].clock += pst.clock;
    curev().origin_iid = IID<IPid>(tipid,pst.clock[tipid]);
    curev().md = pst.md;
  }else{ // Add the clock of the auxiliary thread (because of fence semantics)
    assert(threads[tipid].store_buffer.empty());
    threads[tipid].clock += threads[tipid+1].clock;
    curev().clock += threads[tipid+1].clock;
  }

  VecSet<int> seen_accesses;

  /* See previous updates reads to ml */
  for(void const *b : ml){
    ByteInfo &bi = mem[b];
    int lu = bi.last_update;
    assert(lu < int(prefix.len()));
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
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    VecSet<void const*> &A = threads[pid].sleep_accesses_r;
    for(void const *b : ml){
      A.insert(b);
    }
    return;
  }
  curev().may_conflict = true;
  record_symbolic(SymEv::Load(ml));
  IPid ipid = curev().iid.get_pid();

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
        curev().clock += clk;
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
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_full_memory_conflict = true;
    return;
  }
  curev().may_conflict = true;
  record_symbolic(SymEv::Fullmem());

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
  IPid ipid = curev().iid.get_pid();
  assert(ipid % 2 == 0);
  assert(threads[ipid].store_buffer.empty());
  curev().clock += threads[ipid+1].clock;
  threads[ipid].clock += threads[ipid+1].clock;
}

void TSOTraceBuilder::join(int tgt_proc){
  if(dryrun) return;
  IPid ipid = curev().iid.get_pid();
  curev().clock += threads[tgt_proc*2].clock;
  threads[ipid].clock += threads[tgt_proc*2].clock;
  curev().clock += threads[tgt_proc*2+1].clock;
  threads[ipid].clock += threads[tgt_proc*2+1].clock;
}

void TSOTraceBuilder::mutex_lock(const ConstMRef &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
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
  curev().may_conflict = true;
  record_symbolic(SymEv::MLock(ml));
  wakeup(Access::W,ml.ref);

  Mutex &mutex = mutexes[ml.ref];
  IPid ipid = curev().iid.get_pid();

  if(mutex.last_lock < 0){
    /* No previous lock */
    see_events({mutex.last_access,last_full_memory_conflict});
  }else{
    /* Register conflict with last preceding lock */
    if(!prefix[mutex.last_lock].clock.leq(curev().clock)){
      /* Aren't these blocking too? */
      add_lock_race(mutex, mutex.last_lock);
    }
    curev().clock += prefix[mutex.last_access].clock;
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
  if(!prefix[mutex.last_lock].clock.leq(curev().clock)){
    add_lock_race(mutex, mutex.last_lock);
  }

  if(0 <= last_full_memory_conflict &&
     !prefix[last_full_memory_conflict].clock.leq(curev().clock)){
    add_lock_race(mutex, last_full_memory_conflict);
  }
}

void TSOTraceBuilder::mutex_trylock(const ConstMRef &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
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
  curev().may_conflict = true;
  record_symbolic(SymEv::MLock(ml));
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
    assert(prefix_idx+1 < int(prefix.len()));
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
  curev().may_conflict = true;
  record_symbolic(SymEv::MUnlock(ml));
  wakeup(Access::W,ml.ref);
  assert(0 <= mutex.last_access);

  see_events({mutex.last_access,last_full_memory_conflict});

  mutex.last_access = prefix_idx;
}

void TSOTraceBuilder::mutex_init(const ConstMRef &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.ref);
    return;
  }
  fence();
  assert(mutexes.count(ml.ref) == 0);
  curev().may_conflict = true;
  record_symbolic(SymEv::MInit(ml));
  mutexes[ml.ref] = Mutex(prefix_idx);
  see_events({last_full_memory_conflict});
}

void TSOTraceBuilder::mutex_destroy(const ConstMRef &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
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
  curev().may_conflict = true;
  record_symbolic(SymEv::MDelete(ml));
  wakeup(Access::W,ml.ref);

  see_events({mutex.last_access,last_full_memory_conflict});

  mutexes.erase(ml.ref);
}

bool TSOTraceBuilder::cond_init(const ConstMRef &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
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
  curev().may_conflict = true;
  record_symbolic(SymEv::CInit(ml));
  cond_vars[ml.ref] = CondVar(prefix_idx);
  see_events({last_full_memory_conflict});
  return true;
}

bool TSOTraceBuilder::cond_signal(const ConstMRef &ml){
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.ref);
    return true;
  }
  fence();
  curev().may_conflict = true;
  record_symbolic(SymEv::CSignal(ml));
  wakeup(Access::W,ml.ref);

  auto it = cond_vars.find(ml.ref);
  if(it == cond_vars.end()){
    pthreads_error("cond_signal called with uninitialized condition variable.");
    return false;
  }
  CondVar &cond_var = it->second;
  VecSet<int> seen_events = {last_full_memory_conflict};
  if(curbranch().alt < int(cond_var.waiters.size())-1){
    assert(curbranch().alt == 0);
    register_alternatives(cond_var.waiters.size());
  }
  assert(0 <= curbranch().alt);
  assert(cond_var.waiters.empty() || curbranch().alt < int(cond_var.waiters.size()));
  if(cond_var.waiters.size()){
    /* Wake up the alt:th waiter. */
    int i = cond_var.waiters[curbranch().alt];
    assert(0 <= i && i < prefix_idx);
    IPid ipid = prefix[i].iid.get_pid();
    assert(!threads[ipid].available);
    threads[ipid].available = true;
    /* The next instruction by the thread ipid should be ordered after
     * this signal.
     */
    threads[ipid].clock += curev().clock;
    seen_events.insert(i);

    /* Remove waiter from cond_var.waiters */
    for(int j = curbranch().alt; j < int(cond_var.waiters.size())-1; ++j){
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
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.ref);
    return true;
  }
  fence();
  curev().may_conflict = true;
  record_symbolic(SymEv::CBrdcst(ml));
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
    threads[ipid].clock += curev().clock;
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
    if(!dryrun && (mtx.last_lock < 0 || prefix[mtx.last_lock].iid.get_pid() != curev().iid.get_pid())){
      pthreads_error("cond_wait called with mutex which is not locked by the same thread.");
      return false;
    }
  }

  mutex_unlock(mutex_ml);
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_r.insert(cond_ml.ref);
    return true;
  }
  fence();
  curev().may_conflict = true;
  record_symbolic(SymEv::CWait(cond_ml,mutex_ml));
  wakeup(Access::R,cond_ml.ref);

  IPid pid = curev().iid.get_pid();

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
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.ref);
    return 0;
  }
  fence();

  int err = (EBUSY == 1) ? 2 : 1; // Chose an error value different from EBUSY

  curev().may_conflict = true;
  record_symbolic(SymEv::CDelete(ml));
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
  curev().may_conflict = true;
  record_symbolic(SymEv::Nondet());
  for(int i = curbranch().alt+1; i < alt_count; ++i){
    prefix.parent_at(prefix.len()-1)
      .put_child(Branch({curev().iid.get_pid(),i}));
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
    if(iclock.leq(curev().clock)) continue;
    if(std::any_of(seen_accesses.begin(),seen_accesses.end(),
                   [i,&iclock,this](int j){
                     return 0 <= j && i != j && iclock.leq(this->prefix[j].clock);
                   })) continue;
    branch.push_back(i);
  }

  /* Add clocks from seen accesses */
  IPid ipid = curev().iid.get_pid();
  for(int i : seen_accesses){
    if(i < 0) continue;
    assert(0 <= i && i <= prefix_idx);
    curev().clock += prefix[i].clock;
    threads[ipid].clock += prefix[i].clock;
  }

  for(int i : branch){
    add_noblock_race(i);
  }
}

void TSOTraceBuilder::add_noblock_race(int event){
  assert(0 <= event);
  assert(event < prefix_idx);

  reversible_races.push_back(ReversibleRace::Nonblock(event,prefix_idx));
}

void TSOTraceBuilder::add_lock_race(const Mutex &m, int event){
  assert(0 <= event);
  assert(event < prefix_idx);

  reversible_races.push_back
    (ReversibleRace::Lock(event,prefix_idx,curev().iid,&m));
}

void TSOTraceBuilder::record_symbolic(SymEv event){
  assert(!dryrun);
  if (sym_idx == curev().sym.size()) {
    // assert(!replay);
    if (replay) {
      if (conf.debug_print_on_reset)
      llvm::dbgs() << "New symbolic event " << event <<
        " after " << curev().sym.size() << " expected\n";
      // assert(false && "");
      // abort();
    }
    /* New event */
    curev().sym.push_back(event);
    sym_idx++;
  } else {
    assert(replay);
    /* Replay. SymEv::set() asserts that this is the same event as last time. */
    assert(sym_idx < curev().sym.size());
    curev().sym[sym_idx++].set(event);
  }
}

static bool do_events_conflict_(const SymEv &fst, const SymEv &snd) {
  if (fst.kind == SymEv::NONDET || snd.kind == SymEv::NONDET) return false;
  if (fst.kind == SymEv::FULLMEM || snd.kind == SymEv::FULLMEM) return true;
  if (fst.kind == SymEv::LOAD && snd.kind == SymEv::LOAD) return false;

  /* Really crude. Is it enough? */
  if (fst.has_addr()) {
    if (snd.has_addr()) {
      return fst.addr().overlaps(snd.addr());
    } else if (snd.has_apair()) {
      return fst.addr().overlaps(snd.apair1())
        || fst.addr().overlaps(snd.apair2());
    } else {
      return false;
    }
  } else if (fst.has_apair()) {
    if (snd.has_addr()) {
      return fst.apair1().overlaps(snd.addr())
        || fst.apair1().overlaps(snd.addr());
    } else if (snd.has_apair()) {
      return fst.apair1().overlaps(snd.apair1())
        || fst.apair1().overlaps(snd.apair2())
        || fst.apair2().overlaps(snd.apair1())
        || fst.apair2().overlaps(snd.apair2());
    } else {
      return false;
    }
  } else {
    return false;
  }
}

bool TSOTraceBuilder::do_events_conflict
(const Event::sym_ty &fst, const Event::sym_ty &snd) const{
  for (const SymEv &fe : fst) {
    for (const SymEv &se : snd) {
      if (do_events_conflict_(fe, se)) {
        return true;
      }
    }
  }
  return false;
}

void TSOTraceBuilder::do_race_detect() {
  /* Do race detection */
  for (const ReversibleRace &race : reversible_races) {
    race_detect(race);
  }
  reversible_races.clear();
}

void TSOTraceBuilder::race_detect(const ReversibleRace &race){
  if (conf.dpor_algorithm == Configuration::OPTIMAL) {
    race_detect_optimal(race);
    return;
  }

  int i = race.first_event;
  int j = race.second_event;

  VecSet<IPid> isleep = sleep_set_at(i);

  /* In the case that race is a failed mutex probe, there's no Event in prefix
   * for it, so we'll reconstruct it on demand.
   */
  Event mutex_probe_event({},{});

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
    const IID<IPid> &iid = k == j && race.kind == ReversibleRace::LOCK
      ? race.second_process : prefix[k].iid;
    IPid p = iid.get_pid();
    /* Did we already cover p? */
    if(p < int(candidates.size()) && 0 <= candidates[p]) continue;
    const Event *pevent;
    int psize = 1;
    if (k == j && race.kind == ReversibleRace::LOCK) {
      mutex_probe_event = reconstruct_lock_event(race);
      pevent = &mutex_probe_event;
    } else {pevent = &prefix[k]; psize = prefix.branch(k).size;}
    if (k == j) psize = 1;
    const VClock<IPid> *pclock = &pevent->clock;
    /* Is p after prefix[i]? */
    if(k != j && iclock.leq(*pclock)) continue;
    /* Is p after some other candidate? */
    bool is_after = false;
    for(int q = 0; !is_after && q < int(candidates.size()); ++q){
      if(0 <= candidates[q] && candidates[q] <= (*pclock)[q]){
        is_after = true;
      }
    }
    if(is_after) continue;
    if(int(candidates.size()) <= p){
      candidates.resize(p+1,-1);
    }
    candidates[p] = iid.get_index();
    cand.pid = p;
    cand.size = psize;
    if(prefix.parent_at(i).has_child(cand)){
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
  prefix.parent_at(i).put_child(cand);
}

void TSOTraceBuilder::race_detect_optimal(const ReversibleRace &race){
  const int i = race.first_event;
  const int j = race.second_event;

  VecSet<IPid> isleep = sleep_set_at(i);
  Event *first = &prefix[i];

  Event second({-1,0},{});
  Branch second_br(-1);
  if (race.kind == ReversibleRace::LOCK) {
    second = reconstruct_lock_event(race);
    /* XXX: Lock events don't have alternatives, right? */
    second_br = Branch(second.iid.get_pid());
  } else {
    second = prefix[j];
    second.sleep.clear();
    second.wakeup.clear();
    second_br = prefix.branch(j);
  }
  /* Only replay the racy event. */
  second_br.size = 1;

  /* v is the subsequence of events in prefix come after prefix[i],
   * but do not "happen after" (i.e. their vector clocks are not strictly
   * greater than prefix[i].clock), followed by the second event.
   *
   * It is the sequence we want to insert into the wakeup tree.
   */
  std::vector<std::pair<Branch,Event*>> v;
  for (int k = i + 1; k < int(prefix.len()); ++k){
    if (!first->clock.leq(prefix[k].clock)) {
      v.push_back({prefix.branch(k), &prefix[k]});
    }
  }
  v.push_back({second_br, &second});

  /* TODO: Build a "partial-order heap" of v. The "mins" of the heap
   * would be the weak initials.
   */

  /* iid_map is a map from processes to their next event indices, and is used to
   * construct the iid of any events we see in the wakeup tree,
   * which in turn is used to find the corresponding event in prefix.
   */
  std::vector<int> iid_map = iid_map_at(i);
  std::vector<int> siid_map = iid_map;

  isleep.insert(first->sleep);

  /* Check for redundant exploration */
  for (auto it = v.cbegin(); it != v.cend(); ++it) {
    /* Check for redundant exploration */
    if (isleep.count(it->second->iid.get_pid())) {
      /* Latter events of this process can't be weak initials either, so to
       * save us from checking, we just delete it from isleep */
      isleep.erase(it->second->iid.get_pid());

      /* Is this a weak initial of v? */
      if (!std::any_of(v.cbegin(), it,
                       [&](const std::pair<Branch,Event*> &e){
                         return e.second->clock.leq(it->second->clock);
                       })) {
        /* Then the reversal of this race has already been explored */
        return;
      }

  for (IPid p : isleep) {
    /* Find the next event of the sleeper in prefix */
    const Event *sleep_ev;
    const Branch *sleep_br;
    for (unsigned k = i;; ++k)
      if (prefix[k].iid.get_pid() == p) {
        sleep_ev = &prefix[k];
        sleep_br = &prefix.branch(k);
        break;
      }
    bool dependent = false;
    for (std::pair<Branch,Event*> ve : v) {
      if (*sleep_br == ve.first) {
        assert(false && "Already checked");
        return;
      }
      if (ve.second->iid.get_pid() == p
          || do_events_conflict(ve.second->sym, sleep_ev->sym)) {
        /* Dependent */
        dependent = true;
        break;
      }
    }
    if (!dependent) {
      return;
    }
  }

    }
  }

  /* Do insertion into the wakeup tree */
  WakeupTreeRef<Branch> node = prefix.parent_at(i);
  while(1) {
    if (!node.size()) {
      /* node is a leaf. That means that an execution that will explore the
       * reversal of this race has already been scheduled.
       */
      return;
    }

    /* */
    enum { NO, RECURSE, NEXT } skip = NO;
    for (auto child_it = node.begin(); child_it != node.end(); ++child_it) {
      /* Find this event in prefix */
#ifndef NDEBUG
      /* The clock might not be right; only use for debug printing! */
      Event *child_ev;
#endif
      Event::sym_ty child_sym;
      IID<IPid> child_iid(child_it.branch().pid,
                          iid_map[child_it.branch().pid]);
      for (unsigned k = i;; ++k) {
        assert(k < prefix.len());
        assert(prefix.branch(k).size > 0);
        if (prefix[k].iid.get_pid() == child_iid.get_pid()
            && prefix[k].iid.get_index() <= child_iid.get_index()
            && (prefix[k].iid.get_index() + prefix.branch(k).size)
               > child_iid.get_index()) {
#ifndef NDEBUG
          child_ev = &prefix[k];
#endif
          child_sym = prefix[k].sym;
          if (prefix[k].iid.get_index() != child_iid.get_index()) {
            /* If the indices of prefix[k].iid and child_iid differ, then the
             * child event must be an event without global operations.
             */
            child_sym.clear();
          }
          break;
        }
      }

      for (auto vei = v.begin(); skip == NO && vei != v.end(); ++vei) {
        std::pair<Branch,Event*> &ve = *vei;
        if (child_it.branch() == ve.first) {
          assert(child_it.branch().size >= ve.first.size
                 || !child_it.node().size());
          if (v.size() == 1 && child_it.node().size()) {
            return;
          }
          /* Drop ve from v and recurse into this node */
          v.erase(vei);
          iid_map[child_it.branch().pid] += child_it.branch().size;
          node = child_it.node();
          skip = RECURSE;
          break;
        }

        if (ve.first.pid == child_it.branch().pid
            || do_events_conflict(ve.second->sym, child_sym)) {
          /* This branch is incompatible, try the next */
          skip = NEXT;
        }
      }
      if (skip == RECURSE) break;
      if (skip == NEXT) { skip = NO; continue; }

      iid_map[child_it.branch().pid] += child_it.branch().size;
      node = child_it.node();
      skip = RECURSE;
      break;

      assert(false && "UNREACHABLE");
      abort();
    }
    if (skip == RECURSE) continue;

    /* No existing child was compatible with v. Insert v as a new sequence. */
    for (std::pair<Branch,Event*> &ve : v) node = node.put_child(ve.first);
    return;
  }
}

std::vector<int> TSOTraceBuilder::iid_map_at(int event) const{
  std::vector<int> map(threads.size(), 1);
  for (int i = 0; i < event; ++i) {
    map[prefix[i].iid.get_pid()] += prefix.branch(i).size;
  }
  return map;
}

TSOTraceBuilder::Event TSOTraceBuilder::reconstruct_lock_event
(const ReversibleRace &race) {
  assert(race.kind == ReversibleRace::LOCK);
  VClock<IPid> mutex_clock;
  /* Compute the clock of the locking process (event k in prefix is
   * something unrelated since this is a lock probe) */
  /* Find last event of p before this mutex probe */
  IPid p = race.second_process.get_pid();
  int last = race.second_event-1;
  do {
    if (prefix[last].iid.get_pid() == p) {
      mutex_clock = prefix[last].clock;
      break;
    }
  } while(last--);
  /* Recompute the clock of this mutex_lock_fail */
  ++mutex_clock[p];
  return Event(race.second_process, std::move(mutex_clock));
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
  IPid pid = curev().iid.get_pid();
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
    curev().wakeup.insert(p);
  }
}

bool TSOTraceBuilder::has_cycle(IID<IPid> *loc) const{
  int proc_count = threads.size();
  int pfx_size = prefix.len();

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
      assert(prefix[i].origin_iid.get_pid()
             == prefix[i].iid.get_pid()-1);
      stores[prefix[i].iid.get_pid() / 2].push_back
        ({prefix[i].origin_iid.get_index(),i});
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
    const Branch &branch = prefix.branch(procs[proc].pfx_index);

    if(!procs[proc].blocked){
      assert(evt.iid.get_pid() == 2*proc);
      assert(evt.iid.get_index() <= next_pc);
      assert(next_pc < evt.iid.get_index() + branch.size);
      procs[proc].block_clock = evt.clock;
      assert(procs[proc].block_clock[proc*2] <= next_pc);
      procs[proc].block_clock[proc*2] = next_pc;
      if(procs[proc].store_index < int(stores[proc].size()) &&
         stores[proc][procs[proc].store_index].store == next_pc){
        // This is a store. Also consider the update's clock.
        procs[proc].block_clock
          += prefix[stores[proc][procs[proc].store_index].update].clock;
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
      next_pc = evt.iid.get_index() + branch.size - 1;
      if(procs[proc].store_index < int(stores[proc].size()) &&
         stores[proc][procs[proc].store_index].store-1 < next_pc){
        next_pc = stores[proc][procs[proc].store_index].store-1;
      }
      assert(procs[proc].pc <= next_pc);
      procs[proc].pc = next_pc;

      if(next_pc + 1 == evt.iid.get_index() + branch.size){
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
  if(idx > int(prefix.len())) return 0;
  if(idx == int(prefix.len())) return 1;

  int count = 1;
  for(int i = int(prefix.len())-1; idx <= i; --i){
    count += prefix[i].sleep_branch_trace_count;
    count += std::max(0, int(prefix.children_after(i)))
      * (count / (1 + prefix[i].sleep.size()));
  }

  return count;
}
