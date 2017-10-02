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
  threads.push_back(Thread(CPid(), -1));
  threads.push_back(Thread(CPS.new_aux(CPid()), -1));
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
    if(0 <= prefix_idx && threads[curev().iid.get_pid()].last_event_index() <
       curev().iid.get_index() + curbranch().size - 1){
      /* Continue executing the current Event */
      IPid pid = curev().iid.get_pid();
      *proc = pid/2;
      *aux = pid % 2 - 1;
      *alt = 0;
      assert(threads[pid].available);
      threads[pid].event_indices.push_back(prefix_idx);
      return true;
    }else if(prefix_idx + 1 == int(prefix.len()) && prefix.lastnode().size() == 0){
      /* We are done replaying. Continue below... */
      assert(prefix_idx < 0 || curev().sym.size() == sym_idx);
      replay = false;
      assert(conf.dpor_algorithm != Configuration::OPTIMAL
            || (errors.size() && errors.back()->get_location()
                == IID<CPid>(threads[curev().iid.get_pid()].cpid, curev().iid.get_index()))
            || std::all_of(threads.cbegin(), threads.cend(),
                           [](const Thread &t) { return !t.sleeping; }));
    }else if(prefix_idx + 1 != int(prefix.len()) &&
             dry_sleepers < int(prefix[prefix_idx+1].sleep.size())){
      /* Before going to the next event, dry run the threads that are
       * being put to sleep.
       */
      IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers];
      prefix[prefix_idx+1].sleep_evs.resize
        (prefix[prefix_idx+1].sleep.size());
      threads[pid].sleep_sym = &prefix[prefix_idx+1].sleep_evs[dry_sleepers];
      threads[pid].sleep_sym->clear();
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
          (Event(IID<IPid>(pid,threads[pid].last_event_index() + 1)));
      }
      *proc = pid/2;
      *aux = pid % 2 - 1;
      *alt = curbranch().alt;
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
    threads[curev().iid.get_pid()].event_indices.back() = prefix_idx;
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
       (conf.max_search_depth < 0 || threads[p].last_event_index() < conf.max_search_depth)){
      threads[p].event_indices.push_back(prefix_idx);
      prefix.push(Branch(IPid(p)),
                  Event(IID<IPid>(IPid(p),threads[p].last_event_index())));
      *proc = p/2;
      *aux = 0;
      return true;
    }
  }

  for(p = 0; p < sz; p += 2){ // Loop through real threads
    if(threads[p].available && !threads[p].sleeping &&
       (conf.max_search_depth < 0 || threads[p].last_event_index() < conf.max_search_depth)){
      threads[p].event_indices.push_back(prefix_idx);
      prefix.push(Branch(IPid(p)),
                  Event(IID<IPid>(IPid(p),threads[p].last_event_index())));
      *proc = p/2;
      *aux = -1;
      return true;
    }
  }

  compute_vclocks();

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
  threads[last_pid].event_indices.pop_back();
  --prefix_idx;
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
    prefix.enter_first_child(Event(IID<IPid>()));
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

#ifndef NDEBUG
  /* The if-statement is just so we can control which test cases need to
   *  satisfy this assertion for now. Eventually, all should.
   */
  if (conf.dpor_algorithm == Configuration::OPTIMAL)
  {
    /* Check for SymEv<->VClock equivalence
     * SymEv considers that event i happens after event j iff there is a
     * subsequence s of i..j including i and j s.t.
     *   forall 0 <= k < len(s)-1. do_events_conflict(s[k], s[k+1])
     * As checking all possible subseqences is exponential, we instead rely on
     * the fact that we've already verified the SymEv<->VClock equivalence of
     * any pair in 0..i-1. Thus, it is sufficient to check whether i has a
     * conflict with some event k which has a vector clock strictly greater than
     * j.
     */
    /* The frontier is the set of events such that e is the first event of pid
     * that happen after the event.
     */
    std::vector<unsigned> frontier;
    for (unsigned i = 0; i < prefix.len(); ++i) {
      const Event &e = prefix[i];
      const IPid pid = e.iid.get_pid();
      const Event *prev
        = (e.iid.get_index() == 1 ? nullptr
           : &prefix[find_process_event(pid, e.iid.get_index()-1)]);
      if (i == prefix.len() - 1 && errors.size() &&
          errors.back()->get_location()
          == IID<CPid>(threads[pid].cpid, e.iid.get_index())) {
        /* Ignore dependency differences with the errored event in aborted
         * executions
         */
        break;
      }
      for (unsigned j = i-1; j != unsigned(-1); --j) {
        bool iafterj = false;
        if (prefix[j].iid.get_pid() == pid
            || do_events_conflict(e, prefix[j])) {
          iafterj = true;
          if (!prev || !prefix[j].clock.leq(prev->clock)) {
            frontier.push_back(j);
          }
        } else if (prev && prefix[j].clock.leq(prev->clock)) {
          iafterj = true;
        } else {
          for (unsigned k : frontier) {
            if (prefix[j].clock.leq(prefix[k].clock)) {
              iafterj = true;
              break;
            }
          }
        }

        assert(iafterj == prefix[j].clock.leq(e.clock));
      }

      /* Cleanup */
      frontier.clear();
    }
  }
#endif

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

    Event evt(IID<IPid>(br.pid,br_idx));

    evt.sleep = prev_evt.sleep;
    if(br.pid != prev_evt.iid.get_pid()){
      evt.sleep.insert(prev_evt.iid.get_pid());
    }
    evt.sleep_branch_trace_count = sleep_branch_trace_count;

    prefix.enter_first_child(std::move(evt));
  }

  CPS = CPidSystem();
  threads.clear();
  threads.push_back(Thread(CPid(),-1));
  threads.push_back(Thread(CPS.new_aux(CPid()),-1));
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
  return slp.to_string_one_line([this](IPid p) { return threads[p].cpid.to_string(); });
}

/* For debug-printing the wakeup tree; adds a node and its children to lines */
void TSOTraceBuilder::wut_string_add_node
(std::vector<std::string> &lines, std::vector<int> &iid_map,
 unsigned line, Branch branch, WakeupTreeRef<Branch> node) const{
  unsigned offset = 2 + ((lines.size() < line)?0:lines[line].size());

  std::vector<std::pair<Branch,WakeupTreeRef<Branch>>> nodes({{branch,node}});
  iid_map_step(iid_map, branch);
  unsigned l = line;
  WakeupTreeRef<Branch> n = node;
  Branch b = branch;
  while (n.size()) {
    b = n.begin().branch();
    n = n.begin().node();
    ++l;
    nodes.push_back({b,n});
    iid_map_step(iid_map, b);
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
    iid_map_step_rev(iid_map, b);
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

  std::vector<VecSet<IPid>> wakeup_sets;
  if (conf.observers) wakeup_sets = compute_observers_wakeup_sets();

  for(unsigned i = 0; i < prefix.len(); ++i){
    IPid ipid = prefix[i].iid.get_pid();
    iid_offs = std::max(iid_offs,2*ipid+int(iid_string(i).size()));
    clock_offs = std::max(clock_offs,int(prefix[i].clock.to_string().size()));
    sleep_set.insert(prefix[i].sleep);
    lines.push_back(" SLP:" + slp_string(sleep_set));
    if (conf.observers){
      sleep_set.erase(wakeup_sets[i]);
    }else{
      sleep_set.erase(prefix[i].wakeup);
    }
  }

  /* Add wakeup tree */
  std::vector<int> iid_map = iid_map_at(prefix.len());
  for(int i = prefix.len()-1; 0 <= i; --i){
    auto node = prefix.parent_at(i);
    iid_map_step_rev(iid_map, prefix.branch(i));
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
  /* TODO: First event of thread happens before parents spawn */
  threads.push_back(Thread(child_cpid,prefix_idx));
  threads.push_back(Thread(CPS.new_aux(child_cpid),prefix_idx));
  threads.back().available = false; // Empty store buffer
  curev().may_conflict = true;
  record_symbolic(SymEv::Spawn(threads.size() / 2 - 1));
}

void TSOTraceBuilder::store(const SymAddrSize &ml){
  if(dryrun) return;
  curev().may_conflict = true; /* prefix_idx might become bad otherwise */
  IPid ipid = curev().iid.get_pid();
  threads[ipid].store_buffer.push_back(PendingStore(ml,prefix_idx,last_md));
  threads[ipid+1].available = true;
}

void TSOTraceBuilder::atomic_store(const SymAddrSize &ml){
  if (conf.observers)
    record_symbolic(SymEv::UnobsStore(ml));
  else
    record_symbolic(SymEv::Store(ml));
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    VecSet<SymAddr> &A = threads[pid].sleep_accesses_w;
    for(SymAddr b : ml){
      A.insert(b);
    }
    return;
  }
  IPid ipid = curev().iid.get_pid();
  curev().may_conflict = true;
  bool is_update = ipid % 2;

  IPid uipid = ipid; // ID of the thread changing the memory
  IPid tipid = is_update ? ipid-1 : ipid; // ID of the (real) thread that issued the store

  if(is_update){ // Add the clock of the store instruction
    assert(threads[tipid].store_buffer.size());
    const PendingStore &pst = threads[tipid].store_buffer.front();
    assert(pst.store_event != (unsigned)prefix_idx);
    add_happens_after(prefix_idx, pst.store_event);
    curev().origin_iid = prefix[pst.store_event].iid;
    curev().md = pst.md;
  }else{ // Add the clock of the auxiliary thread (because of fence semantics)
    assert(threads[tipid].store_buffer.empty());
    add_happens_after_thread(prefix_idx, tipid+1);
  }

  VecSet<int> seen_accesses;

  /* See previous updates reads to ml */
  for(SymAddr b : ml){
    ByteInfo &bi = mem[b];
    int lu = bi.last_update;
    assert(lu < int(prefix.len()));
    if(0 <= lu){
      IPid lu_tipid = 2*(prefix[lu].iid.get_pid() / 2);
      if(lu_tipid != tipid){
        if (!conf.observers){
          seen_accesses.insert(bi.last_update);
        }
      }
    }
    for(int i : bi.last_read){
      if(0 <= i && prefix[i].iid.get_pid() != tipid) seen_accesses.insert(i);
    }
  }

  seen_accesses.insert(last_full_memory_conflict);

  see_events(seen_accesses);

  /* Register in memory */
  for(SymAddr b : ml){
    ByteInfo &bi = mem[b];
    if (conf.observers) {
      bi.unordered_updates.insert_geq(prefix_idx);
    }
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

void TSOTraceBuilder::load(const SymAddrSize &ml){
  record_symbolic(SymEv::Load(ml));
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    VecSet<SymAddr> &A = threads[pid].sleep_accesses_r;
    for(SymAddr b : ml){
      A.insert(b);
    }
    return;
  }
  curev().may_conflict = true;
  IPid ipid = curev().iid.get_pid();

  /* Check if this is a ROWE */
  for(int i = int(threads[ipid].store_buffer.size())-1; 0 <= i; --i){
    if(threads[ipid].store_buffer[i].ml.addr == ml.addr){
      /* ROWE */
      threads[ipid].store_buffer[i].last_rowe = prefix_idx;
      return;
    }
  }

  /* Load from memory */

  VecSet<int> seen_accesses;
  VecSet<std::pair<int,int>> seen_pairs;

  /* See all updates to the read bytes. */
  for(SymAddr b : ml){
    int lu = mem[b].last_update;
    const SymAddrSize &lu_ml = mem[b].last_update_ml;
    if(0 <= lu){
      IPid lu_tipid = 2*(prefix[lu].iid.get_pid() / 2);
      if(lu_tipid != ipid){
        seen_accesses.insert(lu);
      }else if(ml != lu_ml){
        if (lu != prefix_idx)
          add_happens_after(prefix_idx, lu);
      }
      if (conf.observers) {
        /* Update last_update to be an observed store */
        for (auto it = prefix[lu].sym.end();;){
          assert(it != prefix[lu].sym.begin());
          --it;
          if(it->kind == SymEv::STORE && it->addr() == lu_ml) break;
          if (it->kind == SymEv::UNOBS_STORE && it->addr() == lu_ml) {
            *it = SymEv::Store(lu_ml);
            break;
          }
        }
        /* Add races */
        for (int u : mem[b].unordered_updates){
          if (prefix[lu].iid != prefix[u].iid) {
            seen_pairs.insert(std::pair<int,int>(u, lu));
          }
        }
        mem[b].unordered_updates.clear();
      }
    }
    assert(mem[b].unordered_updates.size() == 0);
  }

  seen_accesses.insert(last_full_memory_conflict);

  see_events(seen_accesses);
  see_event_pairs(seen_pairs);

  /* Register load in memory */
  for(SymAddr b : ml){
    mem[b].last_read[ipid/2] = prefix_idx;
    wakeup(Access::R,b);
  }
}

void TSOTraceBuilder::full_memory_conflict(){
  record_symbolic(SymEv::Fullmem());
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_full_memory_conflict = true;
    return;
  }
  curev().may_conflict = true;
  assert(!conf.observers);

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

void TSOTraceBuilder::fence(){
  if(dryrun) return;
  IPid ipid = curev().iid.get_pid();
  assert(ipid % 2 == 0);
  assert(threads[ipid].store_buffer.empty());
  add_happens_after_thread(prefix_idx, ipid+1);
}

void TSOTraceBuilder::join(int tgt_proc){
  record_symbolic(SymEv::Join(tgt_proc));
  if(dryrun) return;
  curev().may_conflict = true;
  IPid ipid = curev().iid.get_pid();
  assert(threads[tgt_proc*2].store_buffer.empty());
  add_happens_after_thread(prefix_idx, tgt_proc*2);
  add_happens_after_thread(prefix_idx, tgt_proc*2+1);
}

void TSOTraceBuilder::mutex_lock(const SymAddrSize &ml){
  record_symbolic(SymEv::MLock(ml));
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.addr);
    return;
  }
  fence();
  if(!conf.mutex_require_init && !mutexes.count(ml.addr)){
    // Assume static initialization
    mutexes[ml.addr] = Mutex();
  }
  assert(mutexes.count(ml.addr));
  curev().may_conflict = true;
  wakeup(Access::W,ml.addr);

  Mutex &mutex = mutexes[ml.addr];
  IPid ipid = curev().iid.get_pid();

  if(mutex.last_lock < 0){
    /* No previous lock */
    see_events({mutex.last_access,last_full_memory_conflict});
  }else{
    add_lock_suc_race(mutex.last_lock, mutex.last_access);
    see_events({last_full_memory_conflict});
  }

  mutex.last_lock = mutex.last_access = prefix_idx;
}

void TSOTraceBuilder::mutex_lock_fail(const SymAddrSize &ml){
  assert(!dryrun);
  if(!conf.mutex_require_init && !mutexes.count(ml.addr)){
    // Assume static initialization
    mutexes[ml.addr] = Mutex();
  }
  assert(mutexes.count(ml.addr));
  Mutex &mutex = mutexes[ml.addr];
  assert(0 <= mutex.last_lock);
  add_lock_fail_race(mutex, mutex.last_lock);

  if(0 <= last_full_memory_conflict){
    add_lock_fail_race(mutex, last_full_memory_conflict);
  }
}

void TSOTraceBuilder::mutex_trylock(const SymAddrSize &ml){
  record_symbolic(SymEv::MLock(ml));
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.addr);
    return;
  }
  fence();
  if(!conf.mutex_require_init && !mutexes.count(ml.addr)){
    // Assume static initialization
    mutexes[ml.addr] = Mutex();
  }
  assert(mutexes.count(ml.addr));
  curev().may_conflict = true;
  wakeup(Access::W,ml.addr);
  Mutex &mutex = mutexes[ml.addr];
  see_events({mutex.last_access,last_full_memory_conflict});

  mutex.last_access = prefix_idx;
  if(mutex.last_lock < 0){ // Mutex is free
    mutex.last_lock = prefix_idx;
  }
}

void TSOTraceBuilder::mutex_unlock(const SymAddrSize &ml){
  record_symbolic(SymEv::MUnlock(ml));
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.addr);
    return;
  }
  fence();
  if(!conf.mutex_require_init && !mutexes.count(ml.addr)){
    // Assume static initialization
    mutexes[ml.addr] = Mutex();
  }
  assert(mutexes.count(ml.addr));
  Mutex &mutex = mutexes[ml.addr];
  curev().may_conflict = true;
  wakeup(Access::W,ml.addr);
  assert(0 <= mutex.last_access);

  see_events({mutex.last_access,last_full_memory_conflict});

  mutex.last_access = prefix_idx;
}

void TSOTraceBuilder::mutex_init(const SymAddrSize &ml){
  record_symbolic(SymEv::MInit(ml));
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.addr);
    return;
  }
  fence();
  assert(mutexes.count(ml.addr) == 0);
  curev().may_conflict = true;
  mutexes[ml.addr] = Mutex(prefix_idx);
  see_events({last_full_memory_conflict});
}

void TSOTraceBuilder::mutex_destroy(const SymAddrSize &ml){
  record_symbolic(SymEv::MDelete(ml));
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.addr);
    return;
  }
  fence();
  if(!conf.mutex_require_init && !mutexes.count(ml.addr)){
    // Assume static initialization
    mutexes[ml.addr] = Mutex();
  }
  assert(mutexes.count(ml.addr));
  Mutex &mutex = mutexes[ml.addr];
  curev().may_conflict = true;
  wakeup(Access::W,ml.addr);

  see_events({mutex.last_access,last_full_memory_conflict});

  mutexes.erase(ml.addr);
}

bool TSOTraceBuilder::cond_init(const SymAddrSize &ml){
  record_symbolic(SymEv::CInit(ml));
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
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
  curev().may_conflict = true;
  cond_vars[ml.addr] = CondVar(prefix_idx);
  see_events({last_full_memory_conflict});
  return true;
}

bool TSOTraceBuilder::cond_signal(const SymAddrSize &ml){
  record_symbolic(SymEv::CSignal(ml));
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.addr);
    return true;
  }
  fence();
  curev().may_conflict = true;
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
  assert(0 <= curbranch().alt);
  assert(cond_var.waiters.empty() || curbranch().alt < int(cond_var.waiters.size()));
  if(cond_var.waiters.size()){
    /* Wake up the alt:th waiter. */
    int i = cond_var.waiters[curbranch().alt];
    assert(0 <= i && i < prefix_idx);
    IPid ipid = prefix[i].iid.get_pid();
    assert(!threads[ipid].available);
    threads[ipid].available = true;
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

bool TSOTraceBuilder::cond_broadcast(const SymAddrSize &ml){
  record_symbolic(SymEv::CBrdcst(ml));
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.addr);
    return true;
  }
  fence();
  curev().may_conflict = true;
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
    threads[ipid].available = true;
    seen_events.insert(i);
  }
  cond_var.waiters.clear();
  cond_var.last_signal = prefix_idx;

  see_events(seen_events);

  return true;
}

bool TSOTraceBuilder::cond_wait(const SymAddrSize &cond_ml, const SymAddrSize &mutex_ml){
  {
    auto it = mutexes.find(mutex_ml.addr);
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
  record_symbolic(SymEv::CWait(cond_ml));
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_r.insert(cond_ml.addr);
    return true;
  }
  fence();
  curev().may_conflict = true;
  wakeup(Access::R,cond_ml.addr);

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

bool TSOTraceBuilder::cond_awake(const SymAddrSize &cond_ml, const SymAddrSize &mutex_ml){
  if (!dryrun){
    assert(cond_vars.count(cond_ml.addr));
    CondVar &cond_var = cond_vars[cond_ml.addr];
    add_happens_after(prefix_idx, cond_var.last_signal);
  }

  mutex_lock(mutex_ml);
  record_symbolic(SymEv::CAwake(cond_ml));
  if(dryrun){
    return true;
  }
  curev().may_conflict = true;

  return true;
}

int TSOTraceBuilder::cond_destroy(const SymAddrSize &ml){
  record_symbolic(SymEv::CDelete(ml));
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.addr);
    return 0;
  }
  fence();

  int err = (EBUSY == 1) ? 2 : 1; // Chose an error value different from EBUSY

  curev().may_conflict = true;
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

void TSOTraceBuilder::register_alternatives(int alt_count){
  curev().may_conflict = true;
  record_symbolic(SymEv::Nondet(alt_count));
  if(curbranch().alt == 0) {
    for(int i = curbranch().alt+1; i < alt_count; ++i){
      curev().races.push_back(Race::Nondet(prefix_idx, i));
    }
  }
}

VecSet<TSOTraceBuilder::IPid> TSOTraceBuilder::sleep_set_at(int i) const{
  VecSet<IPid> sleep;
  for(int j = 0; j < i; ++j){
    sleep.insert(prefix[j].sleep);
    sleep.erase(prefix[j].wakeup);
  }
  sleep.insert(prefix[i].sleep);
  return sleep;
}

std::vector<VecSet<TSOTraceBuilder::IPid>> TSOTraceBuilder::
compute_observers_wakeup_sets() const{
  std::vector<VecSet<IPid>> wakeup_sets(prefix.len());

  std::vector<int> iid_map = iid_map_at(0);
  for (unsigned i = 0; i < prefix.len(); ++i){
    const Event &e = prefix[i];
    for (int j = 0; j < e.sleep.size(); ++j){
      /* In order to wake processes at the right point when observer effects are
       * possible, in case the sleeping event does appear in the current
       * execution sequence, we must use the symbolic event with the observer
       * flags from where the event was executed, and additionally check for the
       * first conflict in reverse from there. Otherwise we will compute
       * incorrect sleep sets for the case when we have a trio of writes from
       * different processes before a read of the same variable. In particular,
       * after executing w1-w2-w3-r we will schedule w2-w3-w1-r. During this
       * trace, w1 will be in the sleep set before the first event. The place
       * where w1 should leave the sleep set is after w3, as w1 and w3 conflicts
       * in this trace. However, if doing wakeup top-down, we would either do it
       * with the observer flag set, which would have it conflict with w2 and
       * leave the sleepset prematurely (note that w2-w1-w3-r is equivalent to
       * w1-w2-w3-r), or we would do it without the observer flag set, which
       * would not have it leave the sleepset at all until it is executed.
       */
      bool found; unsigned p = e.sleep[j], k;
      std::tie(found, k) = try_find_process_event(p, iid_map[p]);
      if (found){
        const sym_ty &sym = prefix[k].sym;
        for (unsigned l = k-1;; --l){
          if (do_events_conflict(p,                    sym,
                                 prefix.branch(l).pid, prefix[l].sym)){
            wakeup_sets[l].insert(p);
            break;
          }

          assert(l != i && "An event was executed while in the sleepset");
        }
      }else{
        const sym_ty &sym = e.sleep_evs[j];
        for (unsigned l = i; l < prefix.len(); ++l){
          if (do_events_conflict(p,                    sym,
                                 prefix.branch(l).pid, prefix[l].sym)){
            wakeup_sets[l].insert(p);
            break;
          }
        }
      }
    }

    iid_map_step(iid_map, prefix.branch(i));
  }

  return wakeup_sets;
}

std::map<TSOTraceBuilder::IPid,const sym_ty*>
TSOTraceBuilder::sym_sleep_set_at(int i) const{
  assert(i >= 0);
  std::vector<int> iid_map = iid_map_at(0);
  std::map<IPid,const sym_ty*> sleep;
  for(int j = 0;; ++j){
    const Event &e = prefix[j];
    for (int k = 0; k < e.sleep.size(); ++k){
      /* If the sleeping event occurs in prefix before i, then we know that it
       * will also be awoken before that. Thus, as an optimisation, we kan skip
       * putting it in sleep to begin with.
       */
      bool found; unsigned l;
      std::tie(found, l) = try_find_process_event
        (e.sleep[k], iid_map[e.sleep[k]]);
      if (!found || int(l) >= i) {
        sleep.emplace(e.sleep[k], &e.sleep_evs[k]);
      }
    }
    if (j == i) break;
    sym_sleep_set_wake(sleep, prefix[j].iid.get_pid(), prefix[j].sym);
    iid_map_step(iid_map, prefix.branch(j));
  }

  return sleep;
}

void TSOTraceBuilder::sym_sleep_set_add(std::map<IPid,const sym_ty*> &sleep,
                                          const Event &e) const{
  for (int k = 0; k < e.sleep.size(); ++k){
    sleep.emplace(e.sleep[k], &e.sleep_evs[k]);
  }
}

static void clear_observed(SymEv &e){
  if (e.kind == SymEv::STORE){
    e = SymEv::UnobsStore(e.addr());
  }
}

static void clear_observed(sym_ty &syms){
  for (SymEv &e : syms){
    clear_observed(e);
  }
}

void TSOTraceBuilder::sym_sleep_set_wake(std::map<IPid,const sym_ty*> &sleep,
                                           IPid p, const sym_ty &sym) const{
  for (auto it = sleep.begin(); it != sleep.end();) {
    if (do_events_conflict(p, sym, it->first, *it->second)){
      it = sleep.erase(it);
    }else{
      ++it;
    }
  }
}

void TSOTraceBuilder::sym_sleep_set_wake(std::map<IPid,const sym_ty*> &sleep,
                                           const Event &e) const{
  auto si = sleep.begin();
  auto wi = e.wakeup.begin();
  while (si != sleep.end() && wi != e.wakeup.end()) {
    if (si->first < *wi) {
      ++si;
    } else if (si->first == *wi){
      si = sleep.erase(si);
      ++wi;
    } else {
      ++wi;
    }
  }
}

void TSOTraceBuilder::see_events(const VecSet<int> &seen_accesses){
  for(int i : seen_accesses){
    if(i < 0) continue;
    if (i == prefix_idx) continue;
    add_noblock_race(i);
  }
}

void TSOTraceBuilder::see_event_pairs
(const VecSet<std::pair<int,int>> &seen_accesses){
  for (std::pair<int,int> p : seen_accesses){
    add_observed_race(p.first, p.second);
  }
}

void TSOTraceBuilder::add_noblock_race(int event){
  assert(0 <= event);
  assert(event < prefix_idx);

  curev().races.push_back(Race::Nonblock(event,prefix_idx));
}

void TSOTraceBuilder::add_lock_suc_race(int lock, int unlock){
  assert(0 <= lock);
  assert(lock < unlock);
  assert(unlock < prefix_idx);

  curev().races.push_back(Race::LockSuc(lock,prefix_idx,unlock));
}

void TSOTraceBuilder::add_lock_fail_race(const Mutex &m, int event){
  assert(0 <= event);
  assert(event < prefix_idx);

  lock_fail_races.push_back(Race::LockFail(event,prefix_idx,curev().iid,&m));
}

void TSOTraceBuilder::add_observed_race(int first, int second){
  assert(0 <= first);
  assert(first < second);
  assert(second < prefix_idx);

  prefix[second].races.push_back(Race::Observed(first,second,prefix_idx));
}

void TSOTraceBuilder::add_happens_after(unsigned second, unsigned first){
  assert(first != ~0u);
  assert(second != ~0u);
  assert(first != second);
  assert(first < second);
  assert((long long)second <= prefix_idx);

  prefix[second].happens_after.push_back(first);
}

void TSOTraceBuilder::add_happens_after_thread(unsigned second, IPid thread){
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

void TSOTraceBuilder::compute_vclocks(){
  /* The first event of a thread happens after the spawn event that
   * created it.
   */
  for (const Thread &t : threads) {
    if (t.spawn_event >= 0 && t.event_indices.size() > 0){
      add_happens_after(t.event_indices[0], t.spawn_event);
    }
  }

  /* Move LockFail races into the right event */
  std::vector<Race> final_lock_fail_races;
  for (Race &r : lock_fail_races){
    if (r.second_event < int(prefix.len())) {
      prefix[r.second_event].races.emplace_back(std::move(r));
    } else {
      assert(r.second_event == int(prefix.len()));
      final_lock_fail_races.emplace_back(std::move(r));
    }
  }
  lock_fail_races = std::move(final_lock_fail_races);

  for (unsigned i = 0; i < prefix.len(); i++){
    IPid ipid = prefix[i].iid.get_pid();
    if (prefix[i].iid.get_index() > 1) {
      unsigned last = find_process_event(prefix[i].iid.get_pid(), prefix[i].iid.get_index()-1);
      prefix[i].clock = prefix[last].clock;
    }
    prefix[i].clock[ipid] = prefix[i].iid.get_index();

    /* First add the non-reversible edges */
    for (unsigned j : prefix[i].happens_after){
      assert(j < i);
      prefix[i].clock += prefix[j].clock;
    }

    /* Now we want add the possibly reversible edges, but first we must
     * check for reversibility, since this information is lost (more
     * accurately less easy to compute) once we add them to the clock.
     */
    std::vector<Race> &races = prefix[i].races;

    /* First move all races that are not pairs (and thus cannot be
     * subsumed by other events) to the front.
     */
    auto first_pair = partition(races.begin(), races.end(),
                                [](const Race &r){
                                  return r.kind == Race::NONDET;
                                });

    auto end = races.end();
    bool changed;
    do {
      auto oldend = end;
      changed = false;
      end = partition
        (first_pair, end,
         [this,i](const Race &r){
          return !prefix[r.first_event].clock.leq(prefix[i].clock);
         });
      for (auto it = end; it != oldend; ++it){
        if (it->kind == Race::LOCK_SUC){
          prefix[i].clock += prefix[it->unlock_event].clock;
          changed = true;
        }
      }
    } while (changed);
    /* Then filter out subsumed */
    auto fill = frontier_filter
      (first_pair, end,
       [this](const Race &f, const Race &s){
        int se = s.kind == Race::LOCK_SUC ? s.unlock_event : s.first_event;
        return prefix[f.first_event].clock.leq(prefix[se].clock);
       });
    /* Add clocks of remaining (reversible) races */
    for (auto it = first_pair; it != fill; ++it){
      if (it->kind == Race::LOCK_SUC){
        assert(prefix[it->first_event].clock.leq
               (prefix[it->unlock_event].clock));
        prefix[i].clock += prefix[it->unlock_event].clock;
      }else if (it->kind != Race::LOCK_FAIL){
        prefix[i].clock += prefix[it->first_event].clock;
      }
    }

    /* Now delete the subsumed races. We delayed doing this to avoid
     * iterator invalidation. */
    races.resize(fill - races.begin(), races[0]);
  }
}

void TSOTraceBuilder::record_symbolic(SymEv event){
  if(dryrun) {
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    assert(threads[pid].sleep_sym);
    threads[pid].sleep_sym->push_back(event);
    return;
  }
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

bool TSOTraceBuilder::do_events_conflict
(const Event &fst, const Event &snd) const{
  return do_events_conflict(fst.iid.get_pid(), fst.sym,
                            snd.iid.get_pid(), snd.sym);
}

bool TSOTraceBuilder::do_symevs_conflict
(IPid fst_pid, const SymEv &fst,
 IPid snd_pid, const SymEv &snd) const {
  if (fst.kind == SymEv::NONDET || snd.kind == SymEv::NONDET) return false;
  if (fst.kind == SymEv::FULLMEM || snd.kind == SymEv::FULLMEM) return true;
  if (fst.kind == SymEv::LOAD && snd.kind == SymEv::LOAD) return false;
  if (fst.kind == SymEv::UNOBS_STORE && snd.kind == SymEv::UNOBS_STORE)
    return false;

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

bool TSOTraceBuilder::do_events_conflict
(IPid fst_pid, const sym_ty &fst,
 IPid snd_pid, const sym_ty &snd) const{
  if (fst_pid == snd_pid) return true;
  for (const SymEv &fe : fst) {
    if (fe.has_num() && fe.num() == (snd_pid / 2)) return true;
    for (const SymEv &se : snd) {
      if (do_symevs_conflict(fst_pid, fe, snd_pid, se)) {
        return true;
      }
    }
  }
  for (const SymEv &se : snd) {
    if (se.has_num() && se.num() == (fst_pid / 2)) return true;
  }
  return false;
}

bool TSOTraceBuilder::is_observed_conflict
(const Event &fst, const Event &snd, const Event &thd) const{
  return is_observed_conflict(fst.iid.get_pid(), fst.sym,
                              snd.iid.get_pid(), snd.sym,
                              thd.iid.get_pid(), thd.sym);
}

bool TSOTraceBuilder::is_observed_conflict
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

bool TSOTraceBuilder::is_observed_conflict
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

void TSOTraceBuilder::do_race_detect() {
  /* Bucket sort races by first_event index */
  std::vector<std::vector<const Race*>> races(prefix.len());
  for (const Race &r : lock_fail_races) races[r.first_event].push_back(&r);
  for (unsigned i = 0; i < prefix.len(); ++i){
    for (const Race &r : prefix[i].races) races[r.first_event].push_back(&r);
  }

  /* Do race detection */
  std::map<IPid,const sym_ty*> sleep;
  for (unsigned i = 0; i < races.size(); ++i){
    sym_sleep_set_add(sleep, prefix[i]);
    for (const Race *race : races[i]) {
      assert(race->first_event == int(i));
      race_detect(*race, (const std::map<IPid,const sym_ty*>&)sleep);
    }
    sym_sleep_set_wake(sleep, prefix[i]);
  }

  for (unsigned i = 0; i < prefix.len(); ++i) prefix[i].races.clear();
  lock_fail_races.clear();
}

void TSOTraceBuilder::race_detect
(const Race &race, const std::map<TSOTraceBuilder::IPid,const sym_ty*> &isleep){
  if (conf.dpor_algorithm == Configuration::OPTIMAL) {
    race_detect_optimal(race, isleep);
    return;
  }

  int i = race.first_event;
  int j = race.second_event;

  if (race.kind == Race::NONDET){
    assert(race.alternative > prefix.branch(i).alt);
    prefix.parent_at(i)
      .put_child(Branch({prefix[i].iid.get_pid(),race.alternative}));
    return;
  }

  /* In the case that race is a failed mutex probe, there's no Event in prefix
   * for it, so we'll reconstruct it on demand.
   */
  Event mutex_probe_event({-1,0});

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
    const IID<IPid> &iid = k == j && race.kind == Race::LOCK_FAIL
      ? race.second_process : prefix[k].iid;
    IPid p = iid.get_pid();
    /* Did we already cover p? */
    if(p < int(candidates.size()) && 0 <= candidates[p]) continue;
    const Event *pevent;
    int psize = 1;
    if (k == j && race.kind == Race::LOCK_FAIL) {
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

void TSOTraceBuilder::race_detect_optimal
(const Race &race, const std::map<TSOTraceBuilder::IPid,const sym_ty*> &isleep_const){
  const int i = race.first_event;
  const int j = race.second_event;

  /* We need a writable copy */
  std::map<IPid,const sym_ty*> isleep;
  if (!conf.observers) isleep = isleep_const;

  const Event &first = prefix[i];
  Event second({-1,0});
  Branch second_br(-1);
  if (race.kind == Race::LOCK_FAIL) {
    second = reconstruct_lock_event(race);
    /* XXX: Lock events don't have alternatives, right? */
    second_br = Branch(second.iid.get_pid());
  } else if (race.kind == Race::NONDET) {
    second = first;
    second_br = prefix.branch(i);
    second_br.alt = race.alternative;
  } else {
    second = prefix[j];
    second.sleep.clear();
    second.wakeup.clear();
    second_br = prefix.branch(j);
  }
  if (race.kind == Race::OBSERVED) {
  } else {
    /* Only replay the racy event. */
    second_br.size = 1;
  }

  /* v is the subsequence of events in prefix come after prefix[i],
   * but do not "happen after" (i.e. their vector clocks are not strictly
   * greater than prefix[i].clock), followed by the second event.
   *
   * It is the sequence we want to insert into the wakeup tree.
   */
  std::vector<std::pair<Branch,sym_ty>> v;
  std::vector<const Event*> observers;
  std::vector<std::pair<Branch,const sym_ty&>> notobs;

  if (conf.observers) {
    /* Start v with the entire prefix before e, for the
     * observers-compatible redundancy check.
     */
    for (int k = 0; k < i; ++k) {
      v.emplace_back(prefix.branch(k), prefix[k].sym);
    }
  }

  for (int k = i + 1; k < int(prefix.len()); ++k){
    if (!first.clock.leq(prefix[k].clock)) {
      v.emplace_back(prefix.branch(k), prefix[k].sym);
    } else if (race.kind == Race::OBSERVED && k != j) {
      if (!std::any_of(observers.begin(), observers.end(),
                       [this,k](const Event* o){
                         return o->clock.leq(prefix[k].clock); })){
        if (is_observed_conflict(first, second, prefix[k])){
          observers.push_back(&prefix[k]);
        } else {
          notobs.emplace_back(prefix.branch(k), prefix[k].sym);
        }
      }
    }
  }
  v.emplace_back(second_br, std::move(second.sym));
  if (race.kind == Race::OBSERVED) {
    int k = race.witness_event;
    const Branch &first_br = prefix.branch(i);
    const Event &witness = prefix[k];
    Branch witness_br = prefix.branch(k);
    /* Only replay the racy event. */
    witness_br.size = 1;

    v.emplace_back(first_br, first.sym);
    v.insert(v.end(), notobs.begin(), notobs.end());
    v.emplace_back(witness_br, witness.sym);
  }

  if (conf.observers) {
    /* Recompute observed states on events in v */
    for (std::pair<Branch,sym_ty> &pair : v) {
      clear_observed(pair.second);
    }

    /* When !read_all, last_reads is the set of addresses that have been read
     * (or "are live", if comparing to a liveness analysis).
     * When read_all, last_reads is instead the set of addresses that have *not*
     * been read. All addresses that are not in last_reads are read.
     */
    VecSet<SymAddr> last_reads;
    bool read_all = false;

    for (auto vi = v.end(); vi != v.begin();){
      std::pair<Branch,sym_ty> &vp = *(--vi);
      for (auto ei = vp.second.end(); ei != vp.second.begin();){
        SymEv &e = *(--ei);
        switch(e.kind){
        case SymEv::LOAD:
          if (read_all)
               last_reads.erase (VecSet<SymAddr>(e.addr().begin(), e.addr().end()));
          else last_reads.insert(VecSet<SymAddr>(e.addr().begin(), e.addr().end()));
          break;
        case SymEv::STORE:
          assert(false); abort();
        case SymEv::UNOBS_STORE:
          if (read_all ^ last_reads.intersects
              (VecSet<SymAddr>(e.addr().begin(), e.addr().end()))){
            e = SymEv::Store(e.addr());
          }
          if (read_all)
               last_reads.insert(VecSet<SymAddr>(e.addr().begin(), e.addr().end()));
          else last_reads.erase (VecSet<SymAddr>(e.addr().begin(), e.addr().end()));
          break;
        case SymEv::FULLMEM:
          last_reads.clear();
          read_all = true;
          break;
        default:
          break;
        }
      }
    }
  }

  /* TODO: Build a "partial-order heap" of v. The "mins" of the heap
   * would be the initials.
   */

  /* iid_map is a map from processes to their next event indices, and is used to
   * construct the iid of any events we see in the wakeup tree,
   * which in turn is used to find the corresponding event in prefix.
   */
  std::vector<int> iid_map;
  if (conf.observers) iid_map = iid_map_at(0);
  else iid_map = iid_map_at(i);

  /* Check for redundant exploration */
  if (!conf.observers) {
  for (auto it = v.cbegin(); it != v.cend(); ++it) {
    /* Check for redundant exploration */
    if (isleep.count(it->first.pid)) {
      /* Latter events of this process can't be weak initials either, so to
       * save us from checking, we just delete it from isleep */
      isleep.erase(it->first.pid);

      /* Is this a weak initial of v? */
      bool initial = true;
      for (auto prev = it; prev != v.cbegin();){
        --prev;
        if (do_events_conflict(prev->first.pid, prev->second,
                               it  ->first.pid, it  ->second)){
          initial = false;
          break;
        }
      }
      if (initial){
        /* Then the reversal of this race has already been explored */
        return;
      }
    }
  }
  } else {
    /* Observer-compatible redundancy check. Note the similarity to
     * wakeup tree insertion. */
    /* Conceptually, we want to pop_front() on v after each iteration;
     * but since that's expensive on vectors, we instead keep a pointer
     * vf to where v "should" start, and then erase() all the elements
     * in one batch afterwards.
     */
    auto vf = v.begin();
    for (int l = 0;; l++, ++vf) {
      for (IPid p : prefix[l].sleep) {
        /* Find this event in prefix */
        IID<IPid> sleep_iid(p, iid_map[p]);
        unsigned k = find_process_event(p, sleep_iid.get_index());
        Branch sleep_br = prefix.branch(k);
        sym_ty sleep_sym = prefix[k].sym;
        clear_observed(sleep_sym);
        assert(prefix[k].iid.get_index() == sleep_iid.get_index()
               && "Sleepers cannot be truncated");
        /* As an optimisation, when l < i, we could go directly to the event in
         * v */
        bool skip = false;
        for (auto vei = vf; !skip && vei != v.end(); ++vei) {
          std::pair<Branch,sym_ty> &ve = *vei;
          if (sleep_br == ve.first) {
            if (sleep_sym != ve.second) {
              /* This can happen due to observer effects. We must now make sure
               * ve.second->sym does not have any conflicts with any previous
               * event in v; i.e. wether it actually is a weak initial of v. */
              for (auto pei = vf; !skip && pei != vei; ++pei) {
                const std::pair<Branch,sym_ty> &pe = *pei;
                if (do_events_conflict(ve.first.pid, ve.second,
                                       pe.first.pid, pe.second)) {
                  /* We're not redundant with p, try next sleeper */
                  skip = true;
                }
              }
              if (skip) break;
            }

            /* p is an initial of v; this insertion is redundant */
            return;
          }
          if (ve.first.pid == p
              || do_events_conflict(ve.first.pid, ve.second,
                                    p, sleep_sym)) {
            /* We're not redundant with p, try next sleeper */
            skip = true;
          }
        }
        if (!skip) {
          /* p is an initial of v; this insertion is redundant */
          return;
        }
      }
      if (l >= i) break;
      iid_map_step(iid_map, vf->first);
    }
    v.erase(v.begin(), vf);
  }

  if (!conf.observers){
  for (std::pair<IPid,const sym_ty*> pair : isleep) {
    const Branch &sleep_br = prefix.branch(find_process_event
                                           (pair.first, iid_map[pair.first]));
    bool dependent = false;
    std::vector<int> dbgp_iid_map = iid_map;
    for (const std::pair<Branch,sym_ty> &ve : v) {
      if (sleep_br == ve.first) {
        assert(false && "Already checked");
        return;
      }
      if (do_events_conflict(ve.first.pid, ve.second,
                             pair.first, *pair.second)) {
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
      sym_ty child_sym;
      IID<IPid> child_iid(child_it.branch().pid,
                          iid_map[child_it.branch().pid]);
      unsigned k = find_process_event(child_iid.get_pid(), child_iid.get_index());
      child_sym = prefix[k].sym;
      clear_observed(child_sym);
      if (prefix[k].iid.get_index() != child_iid.get_index()) {
        /* If the indices of prefix[k].iid and child_iid differ, then the
         * child event must be an event without global operations.
         */
        child_sym.clear();
      }

      for (auto vei = v.begin(); skip == NO && vei != v.end(); ++vei) {
        std::pair<Branch,sym_ty> &ve = *vei;
        if (child_it.branch() == ve.first) {
          if (child_sym != ve.second) {
            /* This can happen due to observer effects. We must now make sure
             * ve.second->sym does not have any conflicts with any previous
             * event in v; i.e. wether it actually is a weak initial of v. */
            for (auto pei = v.begin(); skip == NO && pei != vei; ++pei){
              const std::pair<Branch,sym_ty> &pe = *pei;
              if (do_events_conflict(ve.first.pid, ve.second,
                                     pe.first.pid, pe.second)){
                skip = NEXT;
              }
            }
            if (skip == NEXT) break;
          }

          if (v.size() == 1 && child_it.node().size()) {
            return;
          }
          /* Drop ve from v and recurse into this node */
          v.erase(vei);
          iid_map_step(iid_map, child_it.branch());
          node = child_it.node();
          skip = RECURSE;
          break;
        }

        if (do_events_conflict(ve.first.pid, ve.second,
                               child_it.branch().pid, child_sym)) {
          /* This branch is incompatible, try the next */
          skip = NEXT;
        }
      }
      if (skip == RECURSE) break;
      if (skip == NEXT) { skip = NO; continue; }

      iid_map_step(iid_map, child_it.branch());
      node = child_it.node();
      skip = RECURSE;
      break;

      assert(false && "UNREACHABLE");
      abort();
    }
    if (skip == RECURSE) continue;

    /* No existing child was compatible with v. Insert v as a new sequence. */
    for (const std::pair<Branch,sym_ty> &ve : v) node = node.put_child(ve.first);
    return;
  }
}

std::vector<int> TSOTraceBuilder::iid_map_at(int event) const{
  std::vector<int> map(threads.size(), 1);
  for (int i = 0; i < event; ++i) {
    iid_map_step(map, prefix.branch(i));
  }
  return map;
}

void TSOTraceBuilder::iid_map_step(std::vector<int> &iid_map, const Branch &event) const{
  iid_map[event.pid] += event.size;
}

void TSOTraceBuilder::iid_map_step_rev(std::vector<int> &iid_map, const Branch &event) const{
  iid_map[event.pid] -= event.size;
}

TSOTraceBuilder::Event TSOTraceBuilder::reconstruct_lock_event
(const Race &race) {
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
                     [](SymEv &e){ return e.kind == SymEv::M_LOCK; }));
  ret.sym = prefix[race.first_event].sym;
  return ret;
}

inline std::pair<bool,unsigned> TSOTraceBuilder::
try_find_process_event(IPid pid, int index) const{
  assert(pid >= 0 && pid < int(threads.size()));
  assert(index >= 1);
  if (index > int(threads[pid].event_indices.size())){
    return {false, ~0};
  }

  unsigned k = threads[pid].event_indices[index-1];
  assert(k < prefix.len());
  assert(prefix.branch(k).size > 0);
  assert(prefix[k].iid.get_pid() == pid
         && prefix[k].iid.get_index() <= index
         && (prefix[k].iid.get_index() + prefix.branch(k).size) > index);

  return {true, k};
}

inline unsigned TSOTraceBuilder::find_process_event(IPid pid, int index) const{
  assert(pid >= 0 && pid < int(threads.size()));
  assert(index >= 1 && index <= int(threads[pid].event_indices.size()));
  unsigned k = threads[pid].event_indices[index-1];
  assert(k < prefix.len());
  assert(prefix.branch(k).size > 0);
  assert(prefix[k].iid.get_pid() == pid
         && prefix[k].iid.get_index() <= index
         && (prefix[k].iid.get_index() + prefix.branch(k).size) > index);

  return k;
}

bool TSOTraceBuilder::has_pending_store(IPid pid, SymAddr ml) const {
  const std::vector<PendingStore> &sb = threads[pid].store_buffer;
  for(unsigned i = 0; i < sb.size(); ++i){
    if(sb[i].ml.includes(ml)){
      return true;
    }
  }
  return false;
}

void TSOTraceBuilder::wakeup(Access::Type type, SymAddr ml){
  IPid pid = curev().iid.get_pid();
  sym_ty ev;
  std::vector<IPid> wakeup; // Wakeup these
  switch(type){
  case Access::W_ALL_MEMORY:
    {
      ev.push_back(SymEv::Fullmem());
      for(unsigned p = 0; p < threads.size(); ++p){
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
      ev.push_back(SymEv::Load(SymAddrSize(ml,1)));
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
      ev.push_back(SymEv::Store(SymAddrSize(ml,1)));
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

#ifndef NDEBUG
  if (conf.dpor_algorithm == Configuration::OPTIMAL) {
    VecSet<IPid> wakeup_set(wakeup);
    for (unsigned p = 0; p < threads.size(); ++p){
      if (!threads[p].sleeping) continue;
      assert(threads[p].sleep_sym);
      assert(bool(wakeup_set.count(p))
             == do_events_conflict(pid, ev, p, *threads[p].sleep_sym));
    }
  }
#endif

  for(IPid p : wakeup){
    assert(threads[p].sleeping);
    threads[p].sleep_accesses_r.clear();
    threads[p].sleep_accesses_w.clear();
    threads[p].sleep_full_memory_conflict = false;
    threads[p].sleep_sym = nullptr;
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
