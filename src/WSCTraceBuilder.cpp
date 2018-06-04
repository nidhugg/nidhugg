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

#include <sstream>
#include <stdexcept>
#include <iostream>

#define ANSIRed "\x1b[91m"
#define ANSIRst "\x1b[m"

static void clear_observed(sym_ty &syms);

WSCTraceBuilder::WSCTraceBuilder(const Configuration &conf) : TSOPSOTraceBuilder(conf) {
  threads.push_back(Thread(CPid(), -1));
  threads.push_back(Thread(CPS.new_aux(CPid()), -1));
  threads[1].available = false; // Store buffer is empty.
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
      assert(prefix_idx < 0 || curev().sym.size() == sym_idx
             || (errors.size() && errors.back()->get_location()
                 == IID<CPid>(threads[curbranch().pid].cpid,
                              curev().iid.get_index())));
      replay = false;
    }else{
      /* Go to the next event. */
      assert(prefix_idx < 0 || curev().sym.size() == sym_idx
             || (errors.size() && errors.back()->get_location()
                 == IID<CPid>(threads[curbranch().pid].cpid,
                              curev().iid.get_index())));
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
          (Event(IID<IPid>(pid,threads[pid].last_event_index() + 1),
                 /* Jump a few hoops to get the next Branch before
                  * calling enter_first_child() */
                 prefix.parent_at(prefix_idx).begin().branch().sym));
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
     !prefix[prefix.len()-1].may_conflict){
    assert(prefix.children_after(prefix.len()-1) == 0);
    assert(curev().sym.empty()); /* Would need to be copied */
    assert(curbranch().sym.empty()); /* Can't happen */
    prefix.delete_last();
    --prefix_idx;
    Branch b = curbranch();
    ++b.size;
    prefix.set_last_branch(std::move(b));
    assert(int(threads[curev().iid.get_pid()].event_indices.back()) == prefix_idx + 1);
    threads[curev().iid.get_pid()].event_indices.back() = prefix_idx;
  } else {
    /* Copy symbolic events to wakeup tree */
    if (prefix.len() > 0) {
      if (!curbranch().sym.empty()) {
#ifndef NDEBUG
        sym_ty expected = curev().sym;
        if (conf.observers) clear_observed(expected);
        assert(curbranch().sym == expected);
#endif
      } else {
        Branch b = curbranch();
        b.sym = curev().sym;
        if (conf.observers) clear_observed(b.sym);
        for (SymEv &e : b.sym) e.purge_data();
        prefix.set_last_branch(std::move(b));
      }
    }
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
    if(threads[p].available &&
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
    if(threads[p].available &&
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

  compute_prefixes();

  return false; // No available threads
}

void WSCTraceBuilder::refuse_schedule(){
  assert(prefix_idx == int(prefix.len())-1);
  assert(prefix.lastbranch().size == 1);
  assert(!prefix.last().may_conflict);
  assert(prefix.children_after(prefix_idx) == 0);
  IPid last_pid = prefix.last().iid.get_pid();
  prefix.delete_last();
  assert(int(threads[last_pid].event_indices.back()) == prefix_idx);
  threads[last_pid].event_indices.pop_back();
  --prefix_idx;
  mark_unavailable(last_pid/2,last_pid % 2 - 1);
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
  if(!replay) return;
  replay = false;
  while (prefix_idx + 1 < int(prefix.len())) prefix.delete_last();
  if (prefix.node(prefix_idx).size()) {
    prefix.enter_first_child(Event(IID<IPid>()));
    prefix.delete_last();
  }
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

bool WSCTraceBuilder::check_for_cycles(){
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

Trace *WSCTraceBuilder::get_trace() const{
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

bool WSCTraceBuilder::reset(){
  if(conf.debug_print_on_reset){
    llvm::dbgs() << " === WSCTraceBuilder reset ===\n";
    debug_print();
    llvm::dbgs() << " =============================\n";
  }

#ifndef NDEBUG
  /* The if-statement is just so we can control which test cases need to
   *  satisfy this assertion for now. Eventually, all should.
   */
  if(conf.dpor_algorithm == Configuration::OPTIMAL){
    check_symev_vclock_equiv();
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

    const Branch &br = prefix.first_child();

    /* Find the index of br.pid. */
    int br_idx = 1;
    for(int j = i-1; br_idx == 1 && 0 <= j; --j){
      if(prefix[j].iid.get_pid() == br.pid){
        br_idx = prefix[j].iid.get_index() + prefix.branch(j).size;
      }
    }

    Event evt(IID<IPid>(br.pid,br_idx));

    evt.sym = br.sym; /* For replay sanity assertions only */
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

std::string WSCTraceBuilder::iid_string(std::size_t pos) const{
  return iid_string(prefix.branch(pos), prefix[pos].iid.get_index());
}

std::string WSCTraceBuilder::iid_string(const Branch &branch, int index) const{
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

static std::string
str_join(const std::vector<std::string> &vec, const std::string &sep) {
  std::string res;
  for (auto it = vec.begin(); it != vec.end(); ++it) {
    if (it != vec.begin()) res += sep;
    res += *it;
  }
  return res;
}

/* For debug-printing the wakeup tree; adds a node and its children to lines */
void WSCTraceBuilder::wut_string_add_node
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
  /* First node needs different padding, so we do it here */
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

#ifndef NDEBUG
static std::string events_to_string(const llvm::SmallVectorImpl<SymEv> &e) {
  if (e.size() == 0) return "None()";
  std::string res;
  for (unsigned i = 0; i < e.size(); ++i) {
    res += e[i].to_string();
    if (i != e.size()-1) res += ", ";
  }
  return res;
}

void WSCTraceBuilder::check_symev_vclock_equiv() const {
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

      if (iafterj != prefix[j].clock.leq(e.clock)) {
        if (iafterj) {
          llvm::dbgs() << "SymEv thinks " << i << " happens after " << j
                       << " but vclock does not\n";
        } else {
          llvm::dbgs() << "VClock thinks " << i << " happens after " << j
                       << " but SymEv does not\n";
        }
        int ix_offs = 0;
        int iid_offs = 0;
        int clock_offs = 0;
        for(unsigned k = 0; k < prefix.len(); ++k){
          IPid ipid = prefix[k].iid.get_pid();
          ix_offs = std::max(ix_offs,int(std::to_string(k).size()));
          iid_offs = std::max(iid_offs,2*ipid+int(iid_string(k).size()));
          clock_offs = std::max(clock_offs,int(prefix[k].clock.to_string().size()));
        }

        for(unsigned k = 0; k < prefix.len(); ++k){
          IPid ipid = prefix[k].iid.get_pid();
          llvm::dbgs() << rpad("",ix_offs-int(std::to_string(k).size()))
                       << (k == i || k == j ? ANSIRed : "") << k
                       << (k == i || k == j ? ANSIRst : "")
                       << ":" << rpad("",2+ipid*2)
                       << rpad(iid_string(k),iid_offs-ipid*2)
                       << " " << rpad(prefix[k].clock.to_string(),clock_offs)
                       << " " << events_to_string(prefix[k].sym)
                       << "\n";
        }
        if(errors.size()){
          llvm::dbgs() << "Errors:\n";
          for(unsigned k = 0; k < errors.size(); ++k){
            llvm::dbgs() << "  Error #" << k+1 << ": "
                         << errors[k]->to_string() << "\n";
          }
        }
      }
      assert(iafterj == prefix[j].clock.leq(e.clock));
    }

    /* Cleanup */
    frontier.clear();
  }
}
#endif /* !defined(NDEBUG) */

void WSCTraceBuilder::debug_print() const {
  llvm::dbgs() << "WSCTraceBuilder (debug print):\n";
  int iid_offs = 0;
  int clock_offs = 0;
  std::vector<std::string> lines(prefix.len(), "");

  for(unsigned i = 0; i < prefix.len(); ++i){
    IPid ipid = prefix[i].iid.get_pid();
    iid_offs = std::max(iid_offs,2*ipid+int(iid_string(i).size()));
    clock_offs = std::max(clock_offs,int(prefix[i].clock.to_string().size()));
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

void WSCTraceBuilder::spawn(){
  IPid parent_ipid = curev().iid.get_pid();
  CPid child_cpid = CPS.spawn(threads[parent_ipid].cpid);
  /* TODO: First event of thread happens before parents spawn */
  threads.push_back(Thread(child_cpid,prefix_idx));
  threads.push_back(Thread(CPS.new_aux(child_cpid),prefix_idx));
  threads.back().available = false; // Empty store buffer
  curev().may_conflict = true;
  record_symbolic(SymEv::Spawn(threads.size() / 2 - 1));
}

void WSCTraceBuilder::store(const SymData &sd){
  curev().may_conflict = true; /* prefix_idx might become bad otherwise */
  IPid ipid = curev().iid.get_pid();
  threads[ipid].store_buffer.push_back(PendingStore(sd.get_ref(),prefix_idx,last_md));
  threads[ipid+1].available = true;
}

void WSCTraceBuilder::atomic_store(const SymData &sd){
  if (conf.observers)
    record_symbolic(SymEv::UnobsStore(sd));
  else
    record_symbolic(SymEv::Store(sd));
  do_atomic_store(sd);
}

static bool symev_is_store(const SymEv &e) {
  return e.kind == SymEv::UNOBS_STORE || e.kind == SymEv::STORE;
}

static SymAddrSize sym_get_last_write(const sym_ty &sym, SymAddr addr){
  for (auto it = sym.end(); it != sym.begin();){
    const SymEv &e = *(--it);
    if (symev_is_store(e) && e.addr().includes(addr)) return e.addr();
  }
  assert(false && "No write touching addr found");
  abort();
}

void WSCTraceBuilder::do_atomic_store(const SymData &sd){
  const SymAddrSize &ml = sd.get_ref();
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
        if(conf.observers){
          SymAddrSize lu_addr = sym_get_last_write(prefix[lu].sym, b);
          if (lu_addr != ml) {
            /* When there is "partial overlap", observers requires
             * writes to be unconditionally racing
             */
            seen_accesses.insert(bi.last_update);
            bi.unordered_updates.clear();
          }
        }else{
          seen_accesses.insert(bi.last_update);
        }
      }
    }
    for(int i : bi.last_read){
      if(0 <= i && prefix[i].iid.get_pid() != tipid) seen_accesses.insert(i);
    }

    /* Register in memory */
    if (conf.observers) {
      bi.unordered_updates.insert_geq(prefix_idx);
    }
    bi.last_update = prefix_idx;
    bi.last_update_ml = ml;
    if(is_update && threads[tipid].store_buffer.front().last_rowe >= 0){
      bi.last_read[tipid/2] = threads[tipid].store_buffer.front().last_rowe;
    }
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
    mem[b].last_read[ipid/2] = prefix_idx;
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
  const SymAddrSize &lu_ml = m.last_update_ml;
  if(0 <= lu){
    IPid lu_tipid = prefix[lu].iid.get_pid() & ~0x1;
    if(lu_tipid != ipid){
      seen_accesses.insert(lu);
    }
    if (conf.observers) {
      /* Update last_update to be an observed store */
      for (auto it = prefix[lu].sym.end();;){
        assert(it != prefix[lu].sym.begin());
        --it;
        if((it->kind == SymEv::STORE || it->kind == SymEv::CMPXHG)
           && it->addr() == lu_ml) break;
        if (it->kind == SymEv::UNOBS_STORE && it->addr() == lu_ml) {
          *it = SymEv::Store(it->data());
          break;
        }
      }
      /* Add races */
      for (int u : m.unordered_updates){
        if (prefix[lu].iid != prefix[u].iid) {
          seen_pairs.insert(std::pair<int,int>(u, lu));
        }
      }
      m.unordered_updates.clear();
    }
  }
  assert(m.unordered_updates.size() == 0);
  see_events(seen_accesses);
  see_event_pairs(seen_pairs);
}

void WSCTraceBuilder::fence(){
  IPid ipid = curev().iid.get_pid();
  assert(ipid % 2 == 0);
  assert(threads[ipid].store_buffer.empty());
  add_happens_after_thread(prefix_idx, ipid+1);
}

void WSCTraceBuilder::join(int tgt_proc){
  record_symbolic(SymEv::Join(tgt_proc));
  curev().may_conflict = true;
  assert(threads[tgt_proc*2].store_buffer.empty());
  add_happens_after_thread(prefix_idx, tgt_proc*2);
  add_happens_after_thread(prefix_idx, tgt_proc*2+1);
}

void WSCTraceBuilder::mutex_lock(const SymAddrSize &ml){
  record_symbolic(SymEv::MLock(ml));
  fence();
  if(!conf.mutex_require_init && !mutexes.count(ml.addr)){
    // Assume static initialization
    mutexes[ml.addr] = Mutex();
  }
  assert(mutexes.count(ml.addr));
  curev().may_conflict = true;

  Mutex &mutex = mutexes[ml.addr];

  if(mutex.last_lock < 0){
    /* No previous lock */
    see_events({mutex.last_access,last_full_memory_conflict});
  }else{
    add_lock_suc_race(mutex.last_lock, mutex.last_access);
    see_events({last_full_memory_conflict});
  }

  mutex.last_lock = mutex.last_access = prefix_idx;
}

void WSCTraceBuilder::mutex_lock_fail(const SymAddrSize &ml){
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

void WSCTraceBuilder::mutex_trylock(const SymAddrSize &ml){
  record_symbolic(SymEv::MLock(ml));
  fence();
  if(!conf.mutex_require_init && !mutexes.count(ml.addr)){
    // Assume static initialization
    mutexes[ml.addr] = Mutex();
  }
  assert(mutexes.count(ml.addr));
  curev().may_conflict = true;
  Mutex &mutex = mutexes[ml.addr];
  see_events({mutex.last_access,last_full_memory_conflict});

  mutex.last_access = prefix_idx;
  if(mutex.last_lock < 0){ // Mutex is free
    mutex.last_lock = prefix_idx;
  }
}

void WSCTraceBuilder::mutex_unlock(const SymAddrSize &ml){
  record_symbolic(SymEv::MUnlock(ml));
  fence();
  if(!conf.mutex_require_init && !mutexes.count(ml.addr)){
    // Assume static initialization
    mutexes[ml.addr] = Mutex();
  }
  assert(mutexes.count(ml.addr));
  Mutex &mutex = mutexes[ml.addr];
  curev().may_conflict = true;
  assert(0 <= mutex.last_access);

  see_events({mutex.last_access,last_full_memory_conflict});

  mutex.last_access = prefix_idx;
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
  if(curbranch().alt == 0) {
    for(int i = curbranch().alt+1; i < alt_count; ++i){
      curev().races.push_back(Race::Nondet(prefix_idx, i));
    }
  }
}

static bool symev_does_load(const SymEv &e) {
  return e.kind == SymEv::LOAD || e.kind == SymEv::CMPXHG
    || e.kind == SymEv::CMPXHGFAIL || e.kind == SymEv::FULLMEM;
}

static void clear_observed(SymEv &e){
  if (e.kind == SymEv::STORE){
    e.set_observed(false);
  }
}

static void clear_observed(sym_ty &syms){
  for (SymEv &e : syms){
    clear_observed(e);
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

void WSCTraceBuilder::recompute_cmpxhg_success
(sym_ty &es, const std::vector<Branch> &v, int i) const {
  for (auto ei = es.begin(); ei != es.end(); ++ei) {
    SymEv &e = *ei;
    if (e.kind == SymEv::CMPXHG || e.kind == SymEv::CMPXHGFAIL) {
      SymData data(e.addr(), e.addr().size);
      VecSet<SymAddr> needed(e.addr().begin(), e.addr().end());
      std::memset(data.get_block(), 0, e.addr().size);

      /* Scan in reverse for data */
      rev_recompute_data(data, needed, ei, es.begin());
      for (auto vi = v.end(); !needed.empty() && (vi != v.begin());){
        const Branch &vb = *(--vi);
        rev_recompute_data(data, needed, vb.sym.end(), vb.sym.begin());
      }
      for (int k = i-1; !needed.empty() && (k >= 0); --k){
        const sym_ty &ps = prefix[k].sym;
        rev_recompute_data(data, needed, ps.end(), ps.begin());
      }

      /* If needed isn't empty then the program reads uninitialised data. */
      // assert(needed.empty());

      bool will_succeed = memcmp(e.expected().get_block(), data.get_block(),
                                 e.addr().size) == 0;
      if (will_succeed) {
        e = SymEv::CmpXhg(e.data(), e.expected().get_shared_block());
      } else {
        e = SymEv::CmpXhgFail(e.data(), e.expected().get_shared_block());
      }
    }
  }
}

void WSCTraceBuilder::recompute_observed(std::vector<Branch> &v) const {
  for (Branch &b : v) {
    clear_observed(b.sym);
  }

  /* When !read_all, last_reads is the set of addresses that have been read
   * (or "are live", if comparing to a liveness analysis).
   * When read_all, last_reads is instead the set of addresses that have *not*
   * been read. All addresses that are not in last_reads are read.
   */
  VecSet<SymAddr> last_reads;
  bool read_all = false;

  for (auto vi = v.end(); vi != v.begin();){
    Branch &vb = *(--vi);
    for (auto ei = vb.sym.end(); ei != vb.sym.begin();){
      SymEv &e = *(--ei);
      switch(e.kind){
      case SymEv::LOAD:
      case SymEv::CMPXHG: /* First a load, then a store */
      case SymEv::CMPXHGFAIL:
        if (read_all)
          last_reads.erase (VecSet<SymAddr>(e.addr().begin(), e.addr().end()));
        else last_reads.insert(VecSet<SymAddr>(e.addr().begin(), e.addr().end()));
        break;
      case SymEv::STORE:
        assert(false); abort();
      case SymEv::UNOBS_STORE:
        if (read_all ^ last_reads.intersects
            (VecSet<SymAddr>(e.addr().begin(), e.addr().end()))){
          e = SymEv::Store(e.data());
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
    } else {
      prefix[i].clock = {};
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
        /* A virtual event does not contribute to the vclock and cannot
         * subsume races. */
        if (s.kind == Race::LOCK_FAIL) return false;
        /* Also filter out observed races with nonfirst witness */
        if (f.kind == Race::OBSERVED && s.kind == Race::OBSERVED
            && f.first_event == s.first_event
            && f.second_event == s.second_event){
          /* N.B. We want the _first_ observer as the witness; thus
           * the reversal of f and s.
           */
          return s.witness_event <= f.witness_event;
        }
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
    if (symev_has_pid(fe) && fe.num() == (snd_pid / 2)) return true;
    for (const SymEv &se : snd) {
      if (do_symevs_conflict(fst_pid, fe, snd_pid, se)) {
        return true;
      }
    }
  }
  for (const SymEv &se : snd) {
    if (symev_has_pid(se) && se.num() == (fst_pid / 2)) return true;
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

void WSCTraceBuilder::compute_prefixes() {
  std::cerr << "Computing prefixes" << std::endl;
  auto is_load = [&](unsigned i) {
    return std::any_of(prefix[i].sym.begin(), prefix[i].sym.end(),
                       [](const SymEv &e) { return e.kind == SymEv::LOAD; });
  };
  auto get_addr = [&](unsigned i) {
    for (SymEv &e : prefix[i].sym) {
      if (e.has_addr()) return e.addr();
    }
    abort();
  };
  auto is_store = [&](unsigned i) {
    return std::any_of(prefix[i].sym.begin(), prefix[i].sym.end(),
                       [](const SymEv &e) { return e.kind == SymEv::STORE; });
  };

  auto pretty_index = [&] (int i) -> std::string {
    if (i==-1) return "init event";
    return  iid_string(i) + events_to_string(prefix[i].sym);
  }; 

  std::map<SymAddr,std::vector<int>> writes_by_address;
  for (unsigned j = 0; j < prefix.len(); ++j) {
    if (is_store(j)) {
      writes_by_address[get_addr(j).addr].push_back(j);
    }
  }
  // for (std::pair<SymAddr,std::vector<int>> &pair : writes_by_address) {
  //   pair.second.push_back(-1);
  // }

  for (unsigned i = 0; i < prefix.len(); ++i) {
    if (!prefix.branch(i).pinned && is_load(i)) {
      auto addr = get_addr(i);
      const std::vector<int> &possible_writes = writes_by_address[addr.addr];
      int original_read_from = *prefix[i].read_from;
      assert(std::any_of(possible_writes.begin(), possible_writes.end(),
             [=](int i) { return i == original_read_from; })
           || original_read_from == -1);      
      // assert(std::any_of(possible_writes.begin(), possible_writes.end(),
      //        [](int i) { return i == -1; })
      //      || original_read_from == -1);


      auto try_read_from = [&](int j) {
        if (j == original_read_from) return;
        prefix[i].read_from = j;
        prefix.branch(i).pinned = true;
	      std::cerr << "Trying to make " << pretty_index(i)
                  << " read from " << pretty_index(j) << 
                  " instead of " << pretty_index(original_read_from);
        try_sat(writes_by_address);
      };

      for (int j : possible_writes) try_read_from(j);
      try_read_from(-1); 

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

  auto pretty_index = [&] (int i) -> std::string {
    if (i==-1) return "init event";
    return  iid_string(i) + events_to_string(prefix[i].sym);
  }; 

  unsigned no_keep = 0;
  std::vector<unsigned> var;
  for (unsigned i = 0; i < prefix.len(); ++i) {
    var.push_back(no_keep);
    if (keep[i]) no_keep++;
  }

  sat.alloc_variables(no_keep);
  /* PO */
  for (unsigned i = 0; i < prefix.len(); ++i) {
    if (!keep[i]) continue;
    if (Option<unsigned> pred = po_predecessor(i)) {
      assert(*pred != i);
      sat.add_edge(var[*pred], var[i]);
    }
  }

  /* Read-from and SC consistency */
  for (unsigned r = 0; r < prefix.len(); ++r) {
    if (!keep[r] || !prefix[r].read_from) continue;
    int w = *prefix[r].read_from;
    assert(int(r) != w);
    if (w == -1) {
      for (int j : writes_by_address[get_addr(r).addr]) {
        if (!keep[j]) continue;
        sat.add_edge(var[r], var[j]);
      }
    } else {
      sat.add_edge(var[w], var[r]);
      for (int j : writes_by_address[get_addr(r).addr]) {
        if (j == w || !keep[j]) continue;
        sat.add_edge_disj(var[j], var[w],
                          var[r], var[j]);
      }
    }
  }

  /* Other happens-after edges (such as thread spawn and join) */
  for (unsigned i = 0; i < prefix.len(); ++i) {
    if (!keep[i]) continue;
    for (unsigned j : prefix[i].happens_after) {
      sat.add_edge(var[j], var[i]);
    }
  }
}

void WSCTraceBuilder::try_sat
(std::map<SymAddr,std::vector<int>> &writes_by_address){
  std::vector<bool> keep = causal_past();
  std::unique_ptr<SatSolver> sat = conf.get_sat_solver();

  output_formula(*sat, writes_by_address, keep);
  //output_formula(std::cerr, writes_by_address, keep);

  if (!sat->check_sat()) {
    std::cerr << ": UNSAT\n";
    return;
  }
  std::cerr << ": SAT\n";

  std::vector<unsigned> model = sat->get_model();

  unsigned no_keep = 0;
  for (unsigned i = 0; i < prefix.len(); ++i) {
    if (keep[i]) no_keep++;
  }
  std::vector<Branch> new_prefix(no_keep, Branch(-1));
  for (unsigned i = 0, var = 0; i < prefix.len(); ++i) {
    if (keep[i]) {
      unsigned pos = model[var++];
      new_prefix[pos] = prefix.branch(i);
    }
  }

  WakeupTreeRef<Branch> root = prefix.parent_at(0);
  for (Branch &b : new_prefix) {
    auto existing = root.begin();
    while (existing != root.end() && !(existing.branch() == b)) ++existing;
    if (existing != root.end()) {
      root = existing.node();
    } else {
      root = root.put_child(b);
    }
  }
}

std::vector<bool> WSCTraceBuilder::causal_past() const {
  std::vector<bool> acc(prefix.len());
  for (unsigned i = 0; i < prefix.len(); ++i) {
    if (prefix.branch(i).pinned) {
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
    iid_map_step(map, prefix.branch(i));
  }
  return map;
}

void WSCTraceBuilder::iid_map_step(std::vector<int> &iid_map, const Branch &event) const{
  if (iid_map.size() <= unsigned(event.pid)) iid_map.resize(event.pid+1, 1);
  iid_map[event.pid] += event.size;
}

void WSCTraceBuilder::iid_map_step_rev(std::vector<int> &iid_map, const Branch &event) const{
  iid_map[event.pid] -= event.size;
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

inline std::pair<bool,unsigned> WSCTraceBuilder::
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

inline unsigned WSCTraceBuilder::find_process_event(IPid pid, int index) const{
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

bool WSCTraceBuilder::has_pending_store(IPid pid, SymAddr ml) const {
  const std::vector<PendingStore> &sb = threads[pid].store_buffer;
  for(unsigned i = 0; i < sb.size(); ++i){
    if(sb[i].ml.includes(ml)){
      return true;
    }
  }
  return false;
}

bool WSCTraceBuilder::has_cycle(IID<IPid> *loc) const{
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

int WSCTraceBuilder::estimate_trace_count() const{
  return estimate_trace_count(0);
}

int WSCTraceBuilder::estimate_trace_count(int idx) const{
  if(idx > int(prefix.len())) return 0;
  if(idx == int(prefix.len())) return 1;

  int count = 42;
  for(int i = int(prefix.len())-1; idx <= i; --i){
    count += prefix[i].sleep_branch_trace_count;
    // count += std::max(0, int(prefix.children_after(i)))
    //   * (count / (1 + prefix[i].sleep.size()));
  }

  return count;
}
