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
#include "TraceUtil.h"

#include <sstream>
#include <stdexcept>

#define ANSIRed "\x1b[91m"
#define ANSIRst "\x1b[m"

static void clear_observed(sym_ty &syms);

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
  assert(!has_vclocks && "Can't add more events after analysing the trace");

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
      assert(prefix_idx < 0 || curev().sym.size() == sym_idx
             || (errors.size() && errors.back()->get_location()
                 == IID<CPid>(threads[curbranch().pid].cpid,
                              curev().iid.get_index())));
      replay = false;
      assert(conf.dpor_algorithm == Configuration::SOURCE
             || (errors.size() && errors.back()->get_location()
                 == IID<CPid>(threads[curev().iid.get_pid()].cpid,
                              curev().iid.get_index()))
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
      assert(prefix_idx < 0 || curev().sym.size() == sym_idx
             || (errors.size() && errors.back()->get_location()
                 == IID<CPid>(threads[curbranch().pid].cpid,
                              curev().iid.get_index())));
      dry_sleepers = 0;
      sym_idx = 0;
      ++prefix_idx;
      IPid pid;
      if (prefix_idx < int(prefix.len())) {
        /* The event is already in prefix */
        pid = curev().iid.get_pid();
        curev().happens_after.clear();
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
      assert(!threads[pid].sleeping);
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
     prefix[prefix.len()-1].sleep.empty() &&
     prefix.children_after(prefix.len()-1) == 0){
    assert(prefix[prefix.len()-1].wakeup.empty());
    assert(curev().sym.empty()); /* Would need to be copied */
    assert(curbranch().sym.empty()); /* Can't happen */
    unsigned size = curbranch().size;
    prefix.delete_last();
    --prefix_idx;
    Branch b = curbranch();
    b.size += size;
    prefix.set_last_branch(std::move(b));
    assert(int(threads[curev().iid.get_pid()].event_indices.back()) == prefix_idx + 1);
    threads[curev().iid.get_pid()].event_indices.back() = prefix_idx;
  } else {
    /* Copy symbolic events to wakeup tree */
    if (prefix.len() > 0) {
      if (!curbranch().sym.empty()) {
#ifndef NDEBUG
        sym_ty expected = curev().sym;
        if (conf.dpor_algorithm == Configuration::OBSERVERS)
          clear_observed(expected);
        assert(curbranch().sym == expected);
#endif
      } else {
        Branch b = curbranch();
        b.sym = curev().sym;
        if (conf.dpor_algorithm == Configuration::OBSERVERS)
          clear_observed(b.sym);
        for (SymEv &e : b.sym) e.purge_data();
        prefix.set_last_branch(std::move(b));
      }
    }
  }

  /* Create a new Event */
  sym_idx = 0;

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
      threads[p].event_indices.push_back(++prefix_idx);
      assert(prefix_idx == int(prefix.len()));
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
      threads[p].event_indices.push_back(++prefix_idx);
      assert(prefix_idx == int(prefix.len()));
      prefix.push(Branch(IPid(p)),
                  Event(IID<IPid>(IPid(p),threads[p].last_event_index())));
      *proc = p/2;
      *aux = -1;
      return true;
    }
  }

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
  assert(int(threads[last_pid].event_indices.back()) == prefix_idx);
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
  /* We need vector clocks for this check */
  compute_vclocks();

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
  SrcLocVectorBuilder cmp_md;
  std::vector<Error*> errs;
  for(unsigned i = 0; i < prefix.len(); ++i){
    cmp.push_back(IID<CPid>(threads[prefix[i].iid.get_pid()].cpid,prefix[i].iid.get_index()));
    cmp_md.push_from(prefix[i].md);
  };
  for(unsigned i = 0; i < errors.size(); ++i){
    errs.push_back(errors[i]->clone());
  }
  Trace *t = new IIDSeqTrace(cmp,cmp_md.build(),errs);
  t->set_blocked(!sleepset_is_empty());
  return t;
}

bool TSOTraceBuilder::reset(){
  compute_vclocks();

  do_race_detect();

  if(conf.debug_print_on_reset){
    llvm::dbgs() << " === TSOTraceBuilder reset ===\n";
    debug_print();
    llvm::dbgs() << " =============================\n";
  }

#ifndef NDEBUG
  /* The if-statement is just so we can control which test cases need to
   *  satisfy this assertion for now. Eventually, all should.
   */
  if(conf.dpor_algorithm != Configuration::SOURCE){
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
    uint64_t sleep_branch_trace_count =
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
  blocked_awaits.clear();
  last_full_memory_conflict = -1;
  prefix_idx = -1;
  dryrun = false;
  replay = true;
  dry_sleepers = 0;
  last_md = 0;
  reset_cond_branch_log();
  has_vclocks = false;

  return true;
}

IID<CPid> TSOTraceBuilder::get_iid() const{
  return get_iid(prefix_idx);
}
IID<CPid> TSOTraceBuilder::get_iid(unsigned i) const{
  IPid pid = prefix[i].iid.get_pid();
  int idx = prefix[i].iid.get_index();
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

static std::string
str_join(const std::vector<std::string> &vec, const std::string &sep) {
  std::string res;
  for (auto it = vec.begin(); it != vec.end(); ++it) {
    if (it != vec.begin()) res += sep;
    res += *it;
  }
  return res;
}

std::string TSOTraceBuilder::oslp_string(const struct obs_sleep &os) const {
  std::vector<std::string> elems;
  auto pid_str = [this](IPid p) { return threads[p].cpid.to_string(); };
  for (const auto &sleeper : os.sleep) {
    elems.push_back(threads[sleeper.pid].cpid.to_string());
    if (sleeper.not_if_read) {
      elems.back() += "/" + sleeper.not_if_read->to_string(pid_str);
    }
  }
  for (const SymAddrSize &sas : os.must_read) {
    elems.push_back(sas.to_string(pid_str));
  }
  return "{" + str_join(elems, ",") + "}";
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

static std::string events_to_string(const llvm::SmallVectorImpl<SymEv> &e) {
  if (e.size() == 0) return "None()";
  std::string res;
  for (unsigned i = 0; i < e.size(); ++i) {
    res += e[i].to_string();
    if (i != e.size()-1) res += ", ";
  }
  return res;
}

#ifndef NDEBUG
void TSOTraceBuilder::check_symev_vclock_equiv() const {
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

void TSOTraceBuilder::debug_print() const {
  llvm::dbgs() << "TSOTraceBuilder (debug print):\n";
  int iid_offs = 0;
  int symev_offs = 0;
  std::vector<std::string> lines;
  struct obs_sleep sleep_set;

  for(unsigned i = 0; i < prefix.len(); ++i){
    IPid ipid = prefix[i].iid.get_pid();
    iid_offs = std::max(iid_offs,2*ipid+int(iid_string(i).size()));
    symev_offs = std::max(symev_offs,
                          int(events_to_string(prefix[i].sym).size()));
    obs_sleep_add(sleep_set, prefix[i]);
    lines.push_back(" SLP:" + oslp_string(sleep_set));
    obs_sleep_wake(sleep_set, ipid, prefix[i].sym);
  }
  for(const auto &ab : blocked_awaits) {
    for (const auto &pa : ab.second) {
      IPid ipid = pa.first;
      const auto &a = pa.second;
      iid_offs = std::max(iid_offs,2*ipid+int(iid_string(Branch(ipid),a.index).size()));
      symev_offs = std::max(symev_offs,
                            int(a.ev.to_string().size()));
      lines.push_back("");
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

  unsigned i = 0;
  for(; i < prefix.len(); ++i){
    IPid ipid = prefix[i].iid.get_pid();
    llvm::dbgs() << rpad("",2+ipid*2)
                 << rpad(iid_string(i),iid_offs-ipid*2)
                 << " " << rpad(events_to_string(prefix[i].sym),symev_offs)
                 << lines[i] << "\n";
  }
  for(const auto &ab : blocked_awaits) {
    for (const auto &pb : ab.second) {
      IPid ipid = pb.first;
      const auto &b = pb.second;
      llvm::dbgs() << " b"
                   << rpad("",ipid*2)
                   << rpad(iid_string(Branch(ipid),b.index),iid_offs-ipid*2)
                   << " " << rpad(b.ev.to_string(),symev_offs)
                   << lines[i++] << "\n";
    }
  }
  for (; i < lines.size(); ++i){
    llvm::dbgs() << std::string(2+iid_offs + 1+symev_offs, ' ') << lines[i] << "\n";
  }
  if(errors.size()){
    llvm::dbgs() << "Errors:\n";
    for(unsigned i = 0; i < errors.size(); ++i){
      llvm::dbgs() << "  Error #" << i+1 << ": "
                   << errors[i]->to_string() << "\n";
    }
  }
}

bool TSOTraceBuilder::spawn(){
  IPid parent_ipid = curev().iid.get_pid();
  CPid child_cpid = CPS.spawn(threads[parent_ipid].cpid);
  /* TODO: First event of thread happens before parents spawn */
  threads.push_back(Thread(child_cpid,prefix_idx));
  threads.push_back(Thread(CPS.new_aux(child_cpid),prefix_idx));
  threads.back().available = false; // Empty store buffer
  curev().may_conflict = true;
  return record_symbolic(SymEv::Spawn(threads.size() / 2 - 1));
}

bool TSOTraceBuilder::store(const SymData &sd){
  if(dryrun) return true;
  curev().may_conflict = true; /* prefix_idx might become bad otherwise */
  IPid ipid = curev().iid.get_pid();
  threads[ipid].store_buffer.push_back(PendingStore(sd.get_ref(),prefix_idx,last_md));
  threads[ipid+1].available = true;
  return true;
}

bool TSOTraceBuilder::atomic_store(const SymData &sd){
  if (conf.dpor_algorithm == Configuration::OBSERVERS) {
    if (!record_symbolic(SymEv::UnobsStore(sd))) return false;
  } else {
    if (!record_symbolic(SymEv::Store(sd))) return false;
  }
  do_atomic_store(sd);
  return true;
}

static bool symev_is_store(const SymEv &e) {
  return e.kind == SymEv::UNOBS_STORE || e.kind == SymEv::STORE;
}

static bool symev_does_store(const SymEv &e) {
  return e.kind == SymEv::UNOBS_STORE || e.kind == SymEv::STORE
    || e.kind == SymEv::CMPXHG || e.kind == SymEv::RMW
    || e.kind == SymEv::XCHG_AWAIT;
}

static VecSet<int> to_vecset_and_clear
(boost::container::flat_map<int,unsigned> &set) {
  VecSet<int> ret;
  for (const auto &el : std::move(set)) {
    ret.insert(el.second);
  }
  set.clear();
  return ret;
}

void TSOTraceBuilder::do_atomic_store(const SymData &sd){
  const SymAddrSize &ml = sd.get_ref();
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
  VecSet<std::pair<int,int>> seen_pairs;

  /* See previous updates reads to ml */
  for(SymAddr b : ml){
    ByteInfo &bi = mem[b];
    int lu = bi.last_update;
    assert(lu < int(prefix.len()));

    bool lu_before_read = false;

    for(int i : bi.last_read){
      if (i < 0) continue;
      const IPid last_read_pid = prefix[i].iid.get_pid();
      if(last_read_pid != tipid) seen_accesses.insert(i);
      if (lu >= 0 && i > lu && prefix[lu].iid.get_pid() != last_read_pid+1)
        lu_before_read = true;
    }

    if (lu_before_read) {
      bi.unordered_updates.clear();
      bi.before_unordered.clear(); // No need to add loads (?)
    } else if(0 <= lu) {
      sym_ty &lu_sym = prefix[lu].sym;
      if (lu_sym.size() != 1
          || lu_sym[0].kind != SymEv::UNOBS_STORE
          || lu_sym[0].addr() != ml) {
        observe_memory(b, bi, seen_accesses, seen_pairs, true);
      } else {
        seen_accesses.insert(bi.before_unordered);
      }
    }
    /* Register in memory */
    if (conf.dpor_algorithm == Configuration::OBSERVERS) {
      bi.unordered_updates[ipid] = prefix_idx;
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

  seen_accesses.insert(last_full_memory_conflict);

  see_events(seen_accesses);
  see_event_pairs(seen_pairs);
}

/* This predicate has to be transitive. */
static bool rmwaction_commutes(const Configuration &conf,
                               RmwAction::Kind lhs, bool lhs_used,
                               RmwAction::Kind rhs, bool rhs_used) {
  if (!conf.commute_rmws) return false;
  if (lhs_used || rhs_used) return false;
  using Kind = RmwAction::Kind;
  switch(lhs) {
  case Kind::ADD: case Kind::SUB:
    return (rhs == Kind::ADD || rhs == Kind::SUB);
  case Kind::XCHG:
    return false;
  default:
    /* All kinds except for XCHG commutes with themselves */
    return rhs == lhs;
  }
}

bool TSOTraceBuilder::atomic_rmw(const SymData &sd, RmwAction action) {
  if (!record_symbolic(SymEv::Rmw(sd, action))) return false;
  const SymAddrSize &ml = sd.get_ref();
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    VecSet<SymAddr> &R = threads[pid].sleep_accesses_r;
    VecSet<SymAddr> &W = threads[pid].sleep_accesses_w;
    for(SymAddr b : ml){
      R.insert(b);
      W.insert(b);
    }
    return true;
  }
  curev().may_conflict = true;
  IPid ipid = curev().iid.get_pid();
  assert(ipid % 2 == 0);

  { // Add the clock of the auxiliary thread (because of fence semantics)
    assert(threads[ipid].store_buffer.empty());
    add_happens_after_thread(prefix_idx, ipid+1);
  }

  VecSet<int> seen_accesses;
  VecSet<std::pair<int,int>> seen_pairs;

  /* See previous updates & reads to ml */
  for(SymAddr b : ml){
    ByteInfo &bi = mem[b];
    int lu = bi.last_update;
    const SymAddrSize &lu_ml = bi.last_update_ml;
    bool conflicts_with_lu = false, lu_before_read = false;
    assert(lu < int(prefix.len()));

    for(int i : bi.last_read){
      if (i < 0) continue;
      const IPid last_read_pid = prefix[i].iid.get_pid();
      if(last_read_pid != ipid) seen_accesses.insert(i);
      if (lu >= 0 && i > lu && prefix[lu].iid.get_pid() != last_read_pid+1)
        lu_before_read = true;
    }

    if (lu_before_read) {
      bi.unordered_updates.clear();
      bi.before_unordered.clear();
    } else if(0 <= lu) {
      IPid lu_tipid = prefix[lu].iid.get_pid() & ~0x1;
      if(lu_tipid == ipid && ml != lu_ml && lu != prefix_idx){
        add_happens_after(prefix_idx, lu);
      }

      sym_ty &lu_sym = prefix[lu].sym;
      if (lu_sym.size() != 1
          || lu_sym[0].kind != SymEv::RMW
          || !rmwaction_commutes(conf, lu_sym[0].rmw_kind(),
                                 lu_sym[0].rmw_result_used(),
                                 action.kind, action.result_used)
          || lu_sym[0].addr() != ml) {
        conflicts_with_lu = true;
        observe_memory(b, bi, seen_accesses, seen_pairs, true);
      } else {
        seen_accesses.insert(bi.before_unordered);
      }
    }
    /* Register access in memory */
    assert(!conflicts_with_lu || bi.unordered_updates.empty());
    bi.unordered_updates[ipid] = prefix_idx;
    bi.last_update = prefix_idx;
    bi.last_update_ml = ml;
    wakeup(Access::W,b);
  }

  seen_accesses.insert(last_full_memory_conflict);

  see_events(seen_accesses);
  see_event_pairs(seen_pairs);
  return true;
}

bool TSOTraceBuilder::xchg_await(const SymData &sd, AwaitCond cond) {
  if (!record_symbolic(SymEv::XchgAwait(sd, cond))) return false;
  const SymAddrSize &ml = sd.get_ref();
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    VecSet<SymAddr> &R = threads[pid].sleep_accesses_r;
    VecSet<SymAddr> &W = threads[pid].sleep_accesses_w;
    for(SymAddr b : ml){
      R.insert(b);
      W.insert(b);
    }
    return true;
  }
  curev().may_conflict = true;
  IPid ipid = curev().iid.get_pid();
  assert(ipid % 2 == 0);

  { // Add the clock of the auxiliary thread (because of fence semantics)
    assert(threads[ipid].store_buffer.empty());
    add_happens_after_thread(prefix_idx, ipid+1);
  }

  VecSet<int> seen_accesses;
  VecSet<int> &happens_after_later = curev().happens_after_later;
  VecSet<std::pair<int,int>> seen_pairs;

  /* Clear it from blocked_awaits, if it is present. */
  auto it = blocked_awaits.find(ml.addr);
  if (it != blocked_awaits.end()) {
    it->second.erase(curev().iid.get_pid());
    if (it->second.empty()) blocked_awaits.erase(it);
  }

  /* See previous updates & reads to ml */
  for(SymAddr b : ml){
    ByteInfo &bi = mem[b];
    int lu = bi.last_update;
    const SymAddrSize &lu_ml = bi.last_update_ml;
    bool lu_before_read = false;
    assert(lu < int(prefix.len()));

    for(int i : bi.last_read){
      if(0 <= i && prefix[i].iid.get_pid() != ipid) {
        /* A lot of these will be redundant. We should consider how to
         * make this more efficient. */
        if (awaitcond_satisfied_before(i, ml, cond)) {
          seen_accesses.insert(i);
        } else {
          happens_after_later.insert(i);
        }
      }
      if (i > lu) lu_before_read = true;
    }

    if (lu_before_read) {
      bi.unordered_updates.clear();
      bi.before_unordered.clear();
    } else if(0 <= lu) {
      IPid lu_tipid = prefix[lu].iid.get_pid() & ~0x1;
      if(lu_tipid == ipid && ml != lu_ml && lu != prefix_idx){
        add_happens_after(prefix_idx, lu);
      }

      observe_memory(b, bi, happens_after_later, seen_pairs, true);
    }
    /* Register access in memory */
    assert(bi.unordered_updates.empty());
    bi.unordered_updates[ipid] = prefix_idx;
    bi.last_update = prefix_idx;
    bi.last_update_ml = ml;
    wakeup(Access::W,b);
  }

  happens_after_later.insert(last_full_memory_conflict);

  see_events(seen_accesses);
  see_event_pairs(seen_pairs);
  return true;
}

bool TSOTraceBuilder::xchg_await_fail(const SymData &sd, AwaitCond cond) {
  if (conf.memory_model != Configuration::SC) {
    invalid_input_error("Exchange-Await is only implemented for SC right now");
    return false;
  }
  assert(!dryrun);

  auto iid = curev().iid;
  SymEv e = SymEv::XchgAwait(sd, std::move(cond));
  const SymAddrSize &ml = sd.get_ref();
  auto ret = blocked_awaits[ml.addr].try_emplace
    (iid.get_pid(), iid.get_index(), std::move(e));
#ifndef NDEBUG
  if(!ret.second) {
    BlockedAwait &aw = ret.first->second;
    assert(aw.index == curev().iid.get_index());
    const AwaitCond &one = e.cond();
    const AwaitCond &other = aw.ev.cond();
    assert(other.op == one.op);
    assert(memcmp(other.operand.get(), one.operand.get(), ml.size) == 0);
  }
#endif

  /* We postpone race detection of failed awaits until the end, in case
   * they get unblocked. */

  return true;
}

bool TSOTraceBuilder::load(const SymAddrSize &ml){
  if (!record_symbolic(SymEv::Load(ml))) return false;
  do_load(ml);
  return true;
}

void TSOTraceBuilder::do_load(const SymAddrSize &ml){
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
      IPid lu_tipid = prefix[lu].iid.get_pid() & ~0x1;
      if(lu_tipid == ipid && ml != lu_ml && lu != prefix_idx){
        add_happens_after(prefix_idx, lu);
      }
    }
    observe_memory(b, mem[b], seen_accesses, seen_pairs, false);

    /* Register load in memory */
    mem[b].last_read[ipid/2] = prefix_idx;
    wakeup(Access::R,b);
  }

  seen_accesses.insert(last_full_memory_conflict);

  see_events(seen_accesses);
  see_event_pairs(seen_pairs);
}

bool TSOTraceBuilder::compare_exchange
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

bool TSOTraceBuilder::load_await(const SymAddrSize &ml, AwaitCond cond) {
  if (conf.memory_model != Configuration::SC) {
    invalid_input_error("Load-Await is only implemented for SC right now");
    return false;
  }
  if (!record_symbolic(SymEv::LoadAwait(ml, cond))) return false;

  if (dryrun) {
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    VecSet<SymAddr> &A = threads[pid].sleep_accesses_r;
    for(SymAddr b : ml){
      A.insert(b);
    }
    return true;
  }
  curev().may_conflict = true;
  IPid ipid = curev().iid.get_pid();
  assert(threads[ipid].store_buffer.empty());

  VecSet<int> &happens_after_later = curev().happens_after_later;
  VecSet<std::pair<int,int>> seen_pairs;

  /* Clear it from blocked_awaits, if it is present. */
  auto it = blocked_awaits.find(ml.addr);
  if (it != blocked_awaits.end()) {
    it->second.erase(curev().iid.get_pid());
    if (it->second.empty()) blocked_awaits.erase(it);
  }

  for (SymAddr b : ml) {
    ByteInfo &bi = mem[b];
    observe_memory(b, bi, happens_after_later, seen_pairs, false);

    /* Register load in memory */
    mem[b].last_read[ipid/2] = prefix_idx;
    wakeup(Access::R,b);
  }

  see_event_pairs(seen_pairs);
  return true;
}

bool TSOTraceBuilder::load_await_fail(const SymAddrSize &ml, AwaitCond cond) {
  if (conf.memory_model != Configuration::SC) {
    invalid_input_error("Load-Await is only implemented for SC right now");
    return false;
  }
  assert(!dryrun);

  auto iid = curev().iid;
  SymEv ev(SymEv::LoadAwait(ml, std::move(cond)));
  auto ret = blocked_awaits[ml.addr].try_emplace
    (iid.get_pid(), iid.get_index(), std::move(ev));
#ifndef NDEBUG
  if(!ret.second) {
    BlockedAwait &aw = ret.first->second;
    assert(aw.index == curev().iid.get_index());
    const AwaitCond &one = ev.cond();
    const AwaitCond &other = aw.ev.cond();
    assert(other.op == one.op);
    assert(memcmp(other.operand.get(), one.operand.get(), ml.size) == 0);
  }
#endif

  /* We postpone race detection of failed awaits until the end, in case
   * they get unblocked. */

  return true;
}

static bool shadows_all_of(const sym_ty &sym, const SymAddrSize &ml) {
  VecSet<SymAddr> addrs(ml.begin(), ml.end());
  for (const SymEv &e : sym) {
    /* Only some event kinds shadows */
    if (e.kind == SymEv::FULLMEM) return true; // Probably unreachable
    if (e.kind != SymEv::STORE && e.kind != SymEv::FULLMEM
        && e.kind != SymEv::XCHG_AWAIT) continue;
    for (SymAddr a : e.addr()) addrs.erase(a);
    if (addrs.empty()) return true;
  }

  assert(!addrs.empty());
  return false;
}

bool TSOTraceBuilder::do_await(unsigned j, const IID<IPid> &iid, const SymEv &e,
                               const VClock<IPid> &above_clock,
                               std::vector<Race> &races) {
  bool can_stop = false;
  const SymAddrSize &ml = e.addr();
  const AwaitCond &cond = e.cond();
  std::vector<unsigned> unordered_accesses;
  for (unsigned i = j;;) {
    if (i-- == 0) break;
    const Event &ie = prefix[i];
    if (std::any_of(ie.sym.begin(), ie.sym.end(),
                    [](const SymEv &e) { return e.kind == SymEv::FULLMEM; })) {
      invalid_input_error
        ("Full memory conflicts are not compatible with awaits", get_iid(i));
      return false;
    }
    if (!std::any_of(ie.sym.begin(), ie.sym.end(),
                     [&ml](const SymEv &e) { return symev_does_store(e)
                         && e.addr().overlaps(ml); })) {
      continue;
    }
    can_stop = can_stop || shadows_all_of(ie.sym, ml);
    if (conf.commute_rmws) {
      // Optimisation idea: Don't include a set of k's that shadow i
      if (unordered_accesses.size() >= 20)
        Debug::warn("TSOTracebuilder::do_await:exponential")
          << "WARNING: Scaling exponentially on a large number of independent writes\n";
      const auto &not_excluded = [this](const std::vector<unsigned> &exclude, unsigned e) {
        const auto &eclock = prefix[e].clock;
        for (unsigned x : exclude) {
          if (eclock.includes(prefix[x].iid)) return false;
        }
        return true;
      };
      /* TODO: Can this be done non-recursively? (maybe by using include
       * and exclude as the state somehow?) */
      std::vector<unsigned> include; include.reserve(unordered_accesses.size());
      std::vector<unsigned> exclude; exclude.reserve(unordered_accesses.size());
      bool did_insert = false;
      const std::function<void(unsigned)> try_subsequences = [&](const unsigned k) {
        if (k == 0) {
          // Do the do
          assert(std::is_sorted(include.begin(), include.end()));
          if (awaitcond_satisfied_by(i, include, ml, cond)) {
            if (conf.debug_print_on_reset) {
              llvm::dbgs() << "Yes, I can reverse " << i << iid_string(i) << " with "
                           << j << (j != prefix.len() ? iid_string(j) : "b")
                           << "\ninc: [";
              for (unsigned i : include) llvm::dbgs() << i << iid_string(i) << ", ";
              llvm::dbgs() << "] exc: [";
              for (unsigned i : exclude) llvm::dbgs() << i << iid_string(i) << ", ";
              llvm::dbgs() << "]\n";
            }
            races.push_back(Race::Sequence(i, j, iid, e, exclude)); // XXX
            did_insert = true;
          }
          return;
        } else {
          const unsigned ke = unordered_accesses[k-1];
          if (prefix[ke].clock.includes(ie.iid)) { //  ie.clock.includes(prefix[ke].iid))
            /* ke is not in notdep(i) */
            try_subsequences(k-1);
            return;
          }
          if (!(above_clock.includes(prefix[ke].iid))) {
            exclude.push_back(ke);
            try_subsequences(k-1);
            assert(exclude.back() == ke);
            exclude.pop_back();
          }
          if (not_excluded(exclude, ke)) {
            include.push_back(ke);
            try_subsequences(k-1);
            assert(include.back() == ke);
            include.pop_back();
          }
        }
      };
      if (!above_clock.includes(ie.iid)) {
        try_subsequences(unordered_accesses.size());
      }
      unordered_accesses.push_back(i);
      if (can_stop && did_insert) break;
    } else if (above_clock.includes(ie.iid)) {
      continue;
    } else if (awaitcond_satisfied_before(i, ml, cond)) {
      if (conf.debug_print_on_reset) {
        llvm::dbgs() << "Yes, I can reverse " << i << iid_string(i) << " with " << j << (j != prefix.len() ? iid_string(j) : "b") << "\n";
      }
      races.push_back(Race::Sequence(i, j, iid, e, {}));
      if (can_stop) break;
    }
  }
  return true;
}

bool TSOTraceBuilder::full_memory_conflict(){
  if (!record_symbolic(SymEv::Fullmem())) return false;
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_full_memory_conflict = true;
    return true;
  }
  curev().may_conflict = true;

  /* See all pervious memory accesses */
  VecSet<int> seen_accesses;
  VecSet<std::pair<int,int>> seen_pairs;
  for(auto it = mem.begin(); it != mem.end(); ++it){
    observe_memory(it->first, it->second, seen_accesses, seen_pairs, true);
    it->second.before_unordered = {prefix_idx}; // Not needed?
    for(int i : it->second.last_read){
      seen_accesses.insert(i);
    }
  }
  for(auto it = mutexes.begin(); it != mutexes.end(); ++it){
    seen_accesses.insert(it->second.last_access);
  }
  seen_accesses.insert(last_full_memory_conflict);
  last_full_memory_conflict = prefix_idx;

  wakeup(Access::W_ALL_MEMORY,{SymMBlock::Global(0),0});

  /* No later access can have a conflict with any earlier access */
  mem.clear();

  see_events(seen_accesses);
  see_event_pairs(seen_pairs);
  return true;
}

static bool observe_store(sym_ty &syms, const SymAddr &ml) {
  for (auto it = syms.end();;){
    assert(it != syms.begin());
    --it;
    if((it->kind == SymEv::STORE || it->kind == SymEv::RMW
        || it->kind == SymEv::CMPXHG || it->kind == SymEv::XCHG_AWAIT)
       && it->addr().includes(ml)) break;
    if (it->kind == SymEv::UNOBS_STORE && it->addr().includes(ml)) {
      *it = SymEv::Store(it->data());
      return true;
    }
    assert(!symev_does_store(*it) || !it->addr().includes(ml));
  }
  return false;
}

static void add_to_seen
(boost::container::flat_map<int, unsigned> &unordered_updates,
 VecSet<int> &seen_accesses) {
  std::vector<int> es;
  es.reserve(unordered_updates.size());
  for (auto &u : unordered_updates) {
    es.push_back(u.second);
  }
  std::sort(es.begin(), es.end());
  seen_accesses.insert(VecSet<int>(std::move(es)));
}

void TSOTraceBuilder::observe_memory(SymAddr ml, ByteInfo &m,
                                     VecSet<int> &seen_accesses,
                                     VecSet<std::pair<int,int>> &seen_pairs,
                                     bool is_update){
  IPid ipid = curev().iid.get_pid();
  int lu = m.last_update;
  if(0 <= lu){
    IPid lu_tipid = prefix[lu].iid.get_pid() & ~0x1;
    if(lu_tipid != ipid){
      seen_accesses.insert(lu);
    }
    bool did_observe_store = false;
    if (conf.dpor_algorithm == Configuration::OBSERVERS) {
      /* Update last_update to be an observed store */
      did_observe_store = observe_store(prefix[lu].sym, ml);
      if (did_observe_store) {
        /* Add races */
        for (const std::pair<IPid,unsigned> &u : m.unordered_updates) {
          if (u.second != unsigned(lu)) {
            seen_pairs.insert(std::pair<int,int>(u.second, lu));
          }
        }
        m.unordered_updates.clear();
        m.before_unordered = {lu};
      }
    }
    if (!did_observe_store) {
      if (is_update) {
        m.before_unordered = to_vecset_and_clear(m.unordered_updates);
        if (m.before_unordered.empty() && lu >= 0) {
          m.before_unordered = {lu};
        }
        seen_accesses.insert(m.before_unordered);
      } else {
        add_to_seen(m.unordered_updates, seen_accesses);
      }
    } else {
      assert(m.unordered_updates.size() == 0);
    }
  }
  assert(!is_update || m.unordered_updates.size() == 0);
}

bool TSOTraceBuilder::fence(){
  if(dryrun) return true;
  IPid ipid = curev().iid.get_pid();
  assert(ipid % 2 == 0);
  assert(threads[ipid].store_buffer.empty());
  add_happens_after_thread(prefix_idx, ipid+1);
  return true;
}

bool TSOTraceBuilder::join(int tgt_proc){
  if (!record_symbolic(SymEv::Join(tgt_proc))) return false;
  if(dryrun) return true;
  curev().may_conflict = true;
  assert(threads[tgt_proc*2].store_buffer.empty());
  add_happens_after_thread(prefix_idx, tgt_proc*2);
  add_happens_after_thread(prefix_idx, tgt_proc*2+1);
  return true;
}

bool TSOTraceBuilder::mutex_lock(const SymAddrSize &ml){
  if (!record_symbolic(SymEv::MLock(ml))) return false;
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.addr);
    return true;
  }
  if (!fence()) return false;
  if(!conf.mutex_require_init && !mutexes.count(ml.addr)){
    // Assume static initialization
    mutexes[ml.addr] = Mutex();
  }
  curev().may_conflict = true;
  wakeup(Access::W,ml.addr);

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
  return true;
}

bool TSOTraceBuilder::mutex_lock_fail(const SymAddrSize &ml){
  assert(!dryrun);
  if(!conf.mutex_require_init && !mutexes.count(ml.addr)){
    // Assume static initialization
    mutexes[ml.addr] = Mutex();
  }
  assert(mutexes.count(ml.addr));
  Mutex &mutex = mutexes[ml.addr];
  assert(0 <= mutex.last_lock);
  add_lock_fail_race(mutex.last_lock);

  if(0 <= last_full_memory_conflict){
    add_lock_fail_race(last_full_memory_conflict);
  }
  return true;
}

bool TSOTraceBuilder::mutex_trylock(const SymAddrSize &ml){
  if (!record_symbolic(SymEv::MLock(ml))) return false;
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.addr);
    return true;
  }
  if (!fence()) return false;
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
  if(!mutex.locked){ // Mutex is free
    mutex.last_lock = prefix_idx;
    mutex.locked = true;
  }
  return true;
}

bool TSOTraceBuilder::mutex_unlock(const SymAddrSize &ml){
  if (!record_symbolic(SymEv::MUnlock(ml))) return false;
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.addr);
    return true;
  }
  if (!fence()) return false;
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
  mutex.locked = false;
  return true;
}

bool TSOTraceBuilder::mutex_init(const SymAddrSize &ml){
  if (!record_symbolic(SymEv::MInit(ml))) return false;
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.addr);
    return true;
  }
  if (!fence()) return false;
  curev().may_conflict = true;
  Mutex &mutex = mutexes[ml.addr];
  see_events({mutex.last_access, last_full_memory_conflict});

  mutex.last_access = prefix_idx;
  return true;
}

bool TSOTraceBuilder::mutex_destroy(const SymAddrSize &ml){
  if (!record_symbolic(SymEv::MDelete(ml))) return false;
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.addr);
    return true;
  }
  if (!fence()) return false;
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
  return true;
}

bool TSOTraceBuilder::cond_init(const SymAddrSize &ml){
  if (!record_symbolic(SymEv::CInit(ml))) return false;
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.addr);
    return true;
  }
  if (!fence()) return false;
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
  if (!record_symbolic(SymEv::CSignal(ml))) return false;
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.addr);
    return true;
  }
  if (!fence()) return false;
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
    if (!register_alternatives(cond_var.waiters.size())) return false;
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
  if (!record_symbolic(SymEv::CBrdcst(ml))) return false;
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.addr);
    return true;
  }
  if (!fence()) return false;
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

  if (!mutex_unlock(mutex_ml)) return false;
  if (!record_symbolic(SymEv::CWait(cond_ml))) return false;
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_r.insert(cond_ml.addr);
    return true;
  }
  if (!fence()) return false;
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

  if (!mutex_lock(mutex_ml)) return false;
  if (!record_symbolic(SymEv::CAwake(cond_ml))) return false;
  if(dryrun){
    return true;
  }
  curev().may_conflict = true;

  return true;
}

int TSOTraceBuilder::cond_destroy(const SymAddrSize &ml){
  const int err = (EBUSY == 1) ? 2 : 1; // Chose an error value different from EBUSY

  if (!record_symbolic(SymEv::CDelete(ml))) return err;
  if(dryrun){
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    threads[pid].sleep_accesses_w.insert(ml.addr);
    return 0;
  }
  if (!fence()) return err;

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

bool TSOTraceBuilder::register_alternatives(int alt_count){
  curev().may_conflict = true;
  if (!record_symbolic(SymEv::Nondet(alt_count))) return false;
  if(curbranch().alt == 0) {
    for(int i = curbranch().alt+1; i < alt_count; ++i){
      curev().races.push_back(Race::Nondet(prefix_idx, i));
    }
  }
  return true;
}

struct TSOTraceBuilder::obs_sleep
TSOTraceBuilder::obs_sleep_at(int i) const{
  assert(i >= 0);
  std::vector<int> iid_map = iid_map_at(0);
  struct obs_sleep sleep;
  for(int j = 0;; ++j){
    obs_sleep_add(sleep, prefix[j]);
    if (j == i) break;
    obs_sleep_wake(sleep, prefix[j]);
    iid_map_step(iid_map, prefix.branch(j));
  }

  return sleep;
}

bool TSOTraceBuilder::obs_sleep::count(IPid p) const {
  return std::any_of(sleep.begin(), sleep.end(),
                     [p](const struct sleeper &s) {
                       return s.pid == p;
                     });
}

void TSOTraceBuilder::obs_sleep_add(struct obs_sleep &sleep,
                                    const Event &e) const{
  for (int k = 0; k < e.sleep.size(); ++k){
    sleep.sleep.push_back({e.sleep[k], &e.sleep_evs[k], nullptr});
  }
}

/* Efficient unordered set delete */
template <typename T> static inline
void unordered_vector_delete(std::vector<T> &vec, std::size_t pos) {
  assert(pos < vec.size());
  if (pos+1 != vec.size())
    vec[pos] = std::move(vec.back());
  vec.pop_back();
}

void
TSOTraceBuilder::obs_sleep_wake(struct obs_sleep &sleep, const Event &e) const{
  if (conf.memory_model == Configuration::TSO) {
    assert(!conf.commute_rmws);
    if (e.wakeup.size()) {
      for (unsigned i = 0; i < sleep.sleep.size();) {
        if (e.wakeup.count(sleep.sleep[i].pid)) {
          unordered_vector_delete(sleep.sleep, i);
        } else {
          ++i;
        }
      }
    }
  } else {
    sym_ty sym = e.sym;
    if (conf.dpor_algorithm == Configuration::OBSERVERS) {
      /* A tricky part to this is that we must clear observers from the events
       * we use to wake */
      clear_observed(sym);
    }
#ifndef NDEBUG
    obs_wake_res res =
#endif
      obs_sleep_wake(sleep, e.iid.get_pid(), sym);
    assert(res != obs_wake_res::BLOCK);
  }
}

static bool symev_does_load(const SymEv &e) {
  return e.kind == SymEv::LOAD || e.kind == SymEv::RMW
    || e.kind == SymEv::CMPXHG || e.kind == SymEv::CMPXHGFAIL
    || e.kind == SymEv::LOAD_AWAIT || e.kind == SymEv::XCHG_AWAIT
    || e.kind == SymEv::FULLMEM;
}

TSOTraceBuilder::obs_wake_res
TSOTraceBuilder::obs_sleep_wake(struct obs_sleep &sleep,
                                IPid p, const sym_ty &sym) const{

  if (conf.dpor_algorithm == Configuration::OBSERVERS) {
    for (const SymEv &e : sym) {
      /* Now check for readers */
      if (e.kind == SymEv::FULLMEM) {
        /* Reads all; observes all */
        sleep.must_read.clear();
      } else if (symev_does_load(e)) {
        const SymAddrSize &esas = e.addr();
        for (int i = 0; i < int(sleep.must_read.size());) {
          if (sleep.must_read[i].overlaps(esas)) {
            unordered_vector_delete(sleep.must_read, i);
          } else {
            ++i;
          }
        }
      }
      if (symev_is_store(e)) {
        /* Now check for shadowing of needed observations */
        const SymAddrSize &esas = e.addr();
        if (std::any_of(sleep.must_read.begin(), sleep.must_read.end(),
                        [&esas](const SymAddrSize &ssas) {
                          return ssas.subsetof(esas);
                        })) {
          return obs_wake_res::BLOCK;
        }
        /* Now handle write-write races by moving the sleepers to sleep_if */
        for (auto it = sleep.sleep.begin(); it != sleep.sleep.end(); ++it) {
          if (std::any_of(it->sym->begin(), it->sym->end(),
                          [&esas](const SymEv &f) {
                            return symev_is_store(f) && f.addr() == esas;
                          })) {
            assert(!it->not_if_read || *it->not_if_read == esas);
            it->not_if_read = esas;
          }
        }
      }
    }
  }

  for (unsigned i = 0; i < sleep.sleep.size();) {
    const auto &s = sleep.sleep[i];
    if (s.pid == p) {
      if (s.not_if_read) {
        sleep.must_read.push_back(*s.not_if_read);
        unordered_vector_delete(sleep.sleep, i);
      } else {
        return obs_wake_res::BLOCK;
      }
    } else if (do_events_conflict(p, sym, s.pid, *s.sym)){
      unordered_vector_delete(sleep.sleep, i);
    } else {
      ++i;
    }
  }

  /* Check if the sleep set became empty */
  if (sleep.sleep.empty() && sleep.must_read.empty()) {
    return obs_wake_res::CLEAR;
  } else {
    return obs_wake_res::CONTINUE;
  }
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

bool TSOTraceBuilder::
sequence_clears_sleep(const std::vector<Branch> &seq,
                      const struct obs_sleep &sleep_const) const{
  /* We need a writable copy */
  struct obs_sleep isleep = sleep_const;
  obs_wake_res state = obs_wake_res::CONTINUE;
  for (auto it = seq.cbegin(); state == obs_wake_res::CONTINUE
         && it != seq.cend(); ++it) {
    state = obs_sleep_wake(isleep, it->pid, it->sym);
  }
  /* Redundant */
  return (state == obs_wake_res::CLEAR);
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
    case SymEv::XCHG_AWAIT:
      assert(symev_does_store(p));
      if (data.get_ref().overlaps(p.addr())) {
        for (SymAddr a : p.addr()) {
          if (needed.erase(a)) {
            data[a] = p.data()[a];
          }
        }
      }
      break;
    default:
      assert(!symev_does_store(p));
      break;
    }
  }
}

template <class Iter>
static void recompute_scan_rev
(const SymAddrSize &desired, VecSet<SymAddr> &needed, std::vector<const SymEv*> &stack,
 Iter end, Iter begin){
  for (auto pi = end; !needed.empty() && (pi != begin);){
    const SymEv &p = *(--pi);
    switch(p.kind){
    case SymEv::STORE:
    case SymEv::UNOBS_STORE:
    case SymEv::CMPXHG:
    case SymEv::XCHG_AWAIT: {
      assert(symev_does_store(p));
      const SymAddrSize &sas = p.addr();
      if (desired.overlaps(sas)) {
        bool any = false;
        for (SymAddr a : sas) {
          if (needed.erase(a)) {
            any = true;
          }
        }
        if (any) stack.push_back(&p);
      }
    } break;
    case SymEv::RMW: {
      const SymAddrSize &sas = p.addr();
      if (desired.overlaps(sas)
          && std::any_of(sas.begin(), sas.end(),
                         [&](const SymAddr a) { return needed.count(a); })) {
        assert(sas.subsetof(desired));
        needed.insert(VecSet<SymAddr>(sas.begin(), sas.end()));
        stack.push_back(&p);
      }
    } break;
    default:
      assert(!symev_does_store(p));
      break;
    }
  }
}

static void recompute_replay_fwd
(SymData &data, std::vector<const SymEv*> &stack){
  while(!stack.empty()) {
    const SymEv &p = *stack.back();
    stack.pop_back();
    const SymAddrSize &sas = p.addr();
    if (p.kind == SymEv::RMW) {
      assert(sas.subsetof(data.get_ref()));
      void *data_at_sas = (char*)data.get_block() + (sas.addr - data.get_ref().addr);
      p.rmwaction().apply_to(data_at_sas, sas.size, data_at_sas);
    } else {
      assert(symev_does_store(p));
      assert(data.get_ref().overlaps(sas));
      for (SymAddr a : sas) {
        data[a] = p.data()[a];
      }
    }
  }
}

bool TSOTraceBuilder::awaitcond_satisfied_before
(unsigned i, const SymAddrSize &ml, const AwaitCond &cond) const {
  /* Recompute what's written */
  SymData data(ml, ml.size);
  VecSet<SymAddr> needed(ml.begin(), ml.end());
  std::memset(data.get_block(), 0, ml.size);

  for (unsigned j = i; !needed.empty();) {
    if (j-- == 0) break;
    const sym_ty &js = prefix[j].sym;
    rev_recompute_data(data, needed, js.end(), js.begin());
  }

  return cond.satisfied_by(data);
}

bool TSOTraceBuilder::awaitcond_satisfied_by
(unsigned i, const std::vector<unsigned> &seq, const SymAddrSize &ml,
 const AwaitCond &cond) const {
  /* Recompute what's written */
  SymData data(ml, ml.size);
  VecSet<SymAddr> needed(ml.begin(), ml.end());
  std::memset(data.get_block(), 0, ml.size);
  std::vector<const SymEv*> stack;

  /* Last comes seq */
  for (unsigned j = seq.size(); !needed.empty();) {
    if (j-- == 0) break;
    const sym_ty &js = prefix[seq[j]].sym;
    recompute_scan_rev(ml, needed, stack, js.end(), js.begin());
  }

  /* Then comes then prefix[:i] */
  for (unsigned j = i; !needed.empty();) {
    if (j-- == 0) break;
    const sym_ty &js = prefix[j].sym;
    recompute_scan_rev(ml, needed, stack, js.end(), js.begin());
  }

  recompute_replay_fwd(data, stack);

  return cond.satisfied_by(data);
}

void TSOTraceBuilder::recompute_cmpxhg_success
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

void TSOTraceBuilder::recompute_observed(std::vector<Branch> &v) const {
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
      case SymEv::RMW:
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

void TSOTraceBuilder::add_lock_suc_race(int lock, int unlock){
  assert(0 <= lock);
  assert(lock < unlock);
  assert(unlock < prefix_idx);

  curev().races.push_back(Race::LockSuc(lock,prefix_idx,unlock));
}

void TSOTraceBuilder::add_lock_fail_race(int event){
  assert(0 <= event);
  assert(event < prefix_idx);

  lock_fail_races.push_back(Race::LockFail(event,prefix_idx,curev().iid));
}

void TSOTraceBuilder::add_observed_race(int first, int second){
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

void TSOTraceBuilder::add_happens_after(unsigned second, unsigned first){
  assert(first != ~0u);
  assert(second != ~0u);
  assert(first != second);
  assert(first < second);
  assert((long long)second <= prefix_idx);

  std::vector<unsigned> &vec = prefix[second].happens_after;
  if (vec.size() && vec.back() == first) return;

  vec.push_back(first);
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
  /* Be idempotent */
  if (has_vclocks) return;

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
      prefix[i].clock = VClock<IPid>();
    }
    prefix[i].clock[ipid] = prefix[i].iid.get_index();

    /* First add the non-reversible edges */
    for (unsigned j : prefix[i].happens_after){
      assert(j < i);
      prefix[i].clock += prefix[j].clock;
    }

    /* Generate await races (with stores, races with loads are handled eagerly) */
    std::vector<Race> &races = prefix[i].races;
    if (std::any_of(prefix[i].sym.begin(), prefix[i].sym.end(),
                    [](const SymEv &e) { return e.has_cond(); })) {
      const SymEv &aw = prefix[i].sym[0];
      assert(prefix[i].sym.size() == 1);
      do_await(i, prefix[i].iid, aw, prefix[i].clock, races);
    }

    /* Now we want add the possibly reversible edges, but first we must
     * check for reversibility, since this information is lost (more
     * accurately less easy to compute) once we add them to the clock.
     */

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
         /* return: Does s subsume f? */
        /* A virtual event does not contribute to the vclock and cannot
         * subsume races. */
        if (s.kind == Race::LOCK_FAIL) return false;
        /* Sequence races can subsume each other, but that is */
        // XXX: Not implemented
        if (s.kind == Race::SEQUENCE) {
          assert(f.kind == Race::SEQUENCE || f.kind == Race::NONBLOCK);
          int fe = f.first_event, se = s.first_event;
          const std::vector<unsigned> &fx = f.exclude, &sx = s.exclude;
          assert(std::is_sorted(fx.begin(), fx.end()));
          assert(std::is_sorted(sx.begin(), sx.end()));
          if (!prefix[se].clock.includes(prefix[fe].iid)) return false;
          /* fe h-b se */

          for (auto si = sx.begin(), fi = fx.begin(); si != sx.end(); ++si) {
            while (fi != fx.end() && *fi < *si) { ++fi; }
            if (fi != fx.end() && *fi == *si) { ++fi; continue; }
            if (prefix[*si].clock.includes(prefix[fe].iid)) continue;
            return false;
          }
          return true;
        }
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
        return prefix[se].clock.includes(prefix[f.first_event].iid);
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
    assert(prefix[i].happens_after_later.empty()
           || (prefix[i].sym.size() == 1
               && prefix[i].sym[0].has_cond()));
    for (int b : prefix[i].happens_after_later) {
      if (b != -1 && !prefix[i].clock.includes(prefix[b].iid)) {
        prefix[i].clock += prefix[b].clock;
      }
    }
    prefix[i].happens_after_later.clear();

    /* Now delete the subsumed races. We delayed doing this to avoid
     * iterator invalidation. */
    races.resize(fill - races.begin(), races[0]);
  }

  has_vclocks = true;
}

bool TSOTraceBuilder::record_symbolic(SymEv event){
  if(dryrun) {
    assert(prefix_idx+1 < int(prefix.len()));
    assert(dry_sleepers <= prefix[prefix_idx+1].sleep.size());
    IPid pid = prefix[prefix_idx+1].sleep[dry_sleepers-1];
    assert(threads[pid].sleep_sym);
    threads[pid].sleep_sym->push_back(event);
    return true;
  }
  if (!replay) {
    assert(sym_idx == curev().sym.size());
    /* New event */
    curev().sym.push_back(event);
    sym_idx++;
  } else {
    /* Replay. SymEv::set() asserts that this is the same event as last time. */
    assert(sym_idx < curev().sym.size());
    SymEv &last = curev().sym[sym_idx++];
    if (!last.is_compatible_with(event)) {
      auto pid_str = [this](IPid p) { return threads[p].cpid.to_string(); };
      nondeterminism_error("Event with effect " + last.to_string(pid_str)
                           + " became " + event.to_string(pid_str)
                           + " when replayed");
      return false;
    }
    last = event;
  }
  return true;
}

bool TSOTraceBuilder::do_events_conflict(int i, int j) const{
  return do_events_conflict(prefix[i], prefix[j]);
}

bool TSOTraceBuilder::do_events_conflict
(const Event &fst, const Event &snd) const{
  return do_events_conflict(fst.iid.get_pid(), fst.sym,
                            snd.iid.get_pid(), snd.sym);
}

static bool symev_has_pid(const SymEv &e) {
  return e.kind == SymEv::SPAWN || e.kind == SymEv::JOIN;
}

static bool symev_is_load(const SymEv &e) {
  return e.kind == SymEv::LOAD || e.kind == SymEv::CMPXHGFAIL
    || e.kind == SymEv::LOAD_AWAIT;
}

static bool symev_is_unobs_store(const SymEv &e) {
  return e.kind == SymEv::UNOBS_STORE;
}

bool TSOTraceBuilder::do_symevs_conflict
(IPid fst_pid, const SymEv &fst,
 IPid snd_pid, const SymEv &snd) const {
  if (fst.kind == SymEv::NONDET || snd.kind == SymEv::NONDET) return false;
  if (fst.kind == SymEv::FULLMEM || snd.kind == SymEv::FULLMEM) return true;
  if (symev_is_load(fst) && symev_is_load(snd)) return false;
  if (symev_is_unobs_store(fst) && symev_is_unobs_store(snd)
      && fst.addr() == snd.addr()) return false;
  if (fst.kind == SymEv::RMW && snd.kind == SymEv::RMW
      && fst.addr() == snd.addr()
      && rmwaction_commutes(conf, fst.rmw_kind(), fst.rmw_result_used(),
                            snd.rmw_kind(), snd.rmw_result_used())) {
    return false;
  }

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
  return symev_does_load(thd) && thd.addr().overlaps(snd.addr());
}

void TSOTraceBuilder::do_race_detect() {
  assert(has_vclocks);

  /* Compute all races of blocked awaits. */
  for (const auto &ab : blocked_awaits) {
    for (const auto &pb : ab.second) {
      IID<IPid> iid(pb.first, pb.second.index);
      std::vector<Race> races;
      const SymEv &ev = pb.second.ev;
      assert(!try_find_process_event(pb.first, iid.get_index()).first);
      VClock<IPid> clock = reconstruct_blocked_clock(iid);
      do_await(prefix.len(), iid, ev, clock, races);
      for (Race &r : races) {
        assert(r.first_event >= 0 && r.first_event < ssize_t(prefix.len()));
        /* Can we optimise do_await to not include these guys in the
         * first place? */
        // XXX: What about other events in include?
        assert (!clock.includes(prefix[r.first_event].iid));
        lock_fail_races.push_back(std::move(r));
      }
    }
  }

  /* Bucket sort races by first_event index */
  std::vector<std::vector<const Race*>> races(prefix.len());
  for (const Race &r : lock_fail_races) races[r.first_event].push_back(&r);
  for (unsigned i = 0; i < prefix.len(); ++i){
    for (const Race &r : prefix[i].races) races[r.first_event].push_back(&r);
  }

  /* Do race detection */
  struct obs_sleep sleep;
  for (unsigned i = 0; i < races.size(); ++i){
    obs_sleep_add(sleep, prefix[i]);
    for (const Race *race : races[i]) {
      assert(race->first_event == int(i));
      race_detect(*race, (const struct obs_sleep&)sleep);
    }
    obs_sleep_wake(sleep, prefix[i]);
  }

  for (unsigned i = 0; i < prefix.len(); ++i) prefix[i].races.clear();
  lock_fail_races.clear();
}

void TSOTraceBuilder::race_detect
(const Race &race, const struct obs_sleep &isleep){
  if (conf.dpor_algorithm != Configuration::SOURCE) {
    race_detect_optimal(race, isleep);
    return;
  }

  int i = race.first_event;
  int j = race.second_event;

  if (race.kind == Race::NONDET){
    assert(race.alternative > prefix.branch(i).alt);
    prefix.parent_at(i)
      .put_child(Branch({prefix[i].iid.get_pid(),race.alternative,
                         prefix.branch(i).sym}));
    return;
  }
  if (race.kind == Race::SEQUENCE) {
    llvm::dbgs() << "ERROR: No support in Source-DPOR for awaits+commuting RMWs yet\n";
    abort();
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
  const sym_ty *cand_sym = nullptr;
  const VClock<IPid> &iclock = prefix[i].clock;
  for(int k = i+1; k <= j; ++k){
    const IID<IPid> &iid = k == j && race.is_fail_kind()
      ? race.second_process : prefix[k].iid;
    IPid p = iid.get_pid();
    /* Did we already cover p? */
    if(p < int(candidates.size()) && 0 <= candidates[p]) continue;
    const Event *pevent;
    int psize = 1;
    if (k == j && race.is_fail_kind()) {
      mutex_probe_event = reconstruct_blocked_event(race);
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
    cand_sym = &pevent->sym;
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
  assert(cand_sym);
  cand.sym = *cand_sym;
  if (race.kind == Race::NONBLOCK) {
    /* Needed for assertions */
    recompute_cmpxhg_success(cand.sym, {}, i);
  }
  prefix.parent_at(i).put_child(cand);
}

void TSOTraceBuilder::race_detect_optimal
(const Race &race, const struct obs_sleep &isleep){
  const int i = race.first_event;
  std::vector<Branch> v = wakeup_sequence(race);

  /* Check if we have previously explored everything startable with v */
  if (!sequence_clears_sleep(v, isleep)) return;

  /* Do insertion into the wakeup tree */
  WakeupTreeRef<Branch> node = prefix.parent_at(i);
  while(1) {
    if (!node.size()) {
      /* node is a leaf. That means that an execution that will explore the
       * reversal of this race has already been scheduled.
       */
      return;
    }

    /* skip is used to break out of the loops if we find a child to
     * recurse into (RECURSE), or if the child is incompatible and we
     * need to check the next (NEXT)
     */
    enum { NO, RECURSE, NEXT } skip = NO;
    for (auto child_it = node.begin(); child_it != node.end(); ++child_it) {
      const sym_ty &child_sym = child_it.branch().sym;
      bool found_match = false;

      for (auto vei = v.begin(); skip == NO && vei != v.end(); ++vei) {
        const Branch &ve = *vei;
        if (child_it.branch() == ve) {
          found_match = true;
          if (child_sym != ve.sym) {
            /* This can happen due to observer effects. We must now make sure
             * ve.second->sym does not have any conflicts with any previous
             * event in v; i.e. wether it actually is a weak initial of v.
             */
            for (auto pei = v.begin(); skip == NO && pei != vei; ++pei){
              const Branch &pe = *pei;
              if (do_events_conflict(ve.pid, ve.sym,
                                     pe.pid, pe.sym)){
                skip = NEXT;
              }
            }
            if (skip == NEXT) break;
          }

          if (v.size() == 1) {
            /* v is about to run out, which means that we had already
             * scheduled an execution that was startable with v.
             * From this point, the recursive insertion is a no-op, so
             * exit early.
             */
            return;
          }

          /* We will recurse into this node. To do that we first need to
           * drop all events in child_it.branch() from v.
           */
          if (ve.size < child_it.branch().size) {
            /* child_it.branch() contains more events than just ve.
             * We need to scan v to find all of them.
             */
            int missing = child_it.branch().size - ve.size;
            IPid pid = ve.pid;
            for (auto vri = v.erase(vei); missing != 0 && vri != v.end();) {
              if (vri->pid == pid) {
                assert(vri->sym.empty());
                if (vri->size > missing) {
                  vri->size = missing;
                  missing = 0;
                  break;
                } else {
                  missing -= vri->size;
                  vri =  v.erase(vri);
                }
              } else {
                ++vri;
              }
            }
          } else if (ve.size > child_it.branch().size) {
            /* ve is larger than child_it.branch(). Delete the common
             * prefix from ve.
             */
            vei->size -= child_it.branch().size;
            vei->sym.clear();
            vei->alt = 0;
          } else {
            /* Drop ve from v. */
            v.erase(vei);
          }
          break;
        }

        if (do_events_conflict(ve.pid, ve.sym,
                               child_it.branch().pid, child_sym)) {
          /* This branch is incompatible, try the next */
          skip = NEXT;
        }
      }
      if (skip == NEXT) { skip = NO; continue; }

      if (!found_match) {
        /* Insertion is not necessary in this case */
        return;
      }

      /* The child is compatible with v, recurse into it. */
      node = child_it.node();
      skip = RECURSE;
      break;

      assert(false && "UNREACHABLE");
      abort();
    }
    if (skip == RECURSE) continue;

    /* No existing child was compatible with v. Insert v as a new sequence. */
    for (Branch &ve : v) {
      if (conf.dpor_algorithm == Configuration::OBSERVERS)
        clear_observed(ve.sym);
      for (SymEv &e : ve.sym) e.purge_data();
      node = node.put_child(std::move(ve));
    }
    return;
  }
}

std::vector<TSOTraceBuilder::Branch> TSOTraceBuilder::
wakeup_sequence(const Race &race) const{
  const int i = race.first_event;
  const int j = race.second_event;

  const Event &first = prefix[i];
  Event second({-1,0});
  Branch second_br(-1);
  std::vector<unsigned>::const_iterator exclude{};
  std::vector<unsigned>::const_iterator exclude_end = exclude;
  if (race.is_fail_kind()) {
    second = reconstruct_blocked_event(race);
    /* XXX: Lock events don't have alternatives, right? */
    second_br = Branch(second.iid.get_pid());
    second_br.sym = std::move(second.sym);
  } else if (race.kind == Race::NONDET) {
    second = first;
    second_br = branch_with_symbolic_data(i);
    second_br.alt = race.alternative;
  } else {
    second = prefix[j];
    second.sleep.clear();
    second.wakeup.clear();
    second_br = branch_with_symbolic_data(j);
  }
  if (race.kind != Race::OBSERVED) {
    /* Only replay the racy event. */
    second_br.size = 1;
  }
  if (race.kind == Race::SEQUENCE) {
    exclude = race.exclude.begin();
    exclude_end = race.exclude.end();
    if (conf.debug_print_on_reset)
      llvm::dbgs() << "SEQUENCE race with " << exclude_end - exclude << " exclusions\n";
  }

  /* v is the subsequence of events in prefix come after prefix[i],
   * but do not "happen after" (i.e. their vector clocks are not strictly
   * greater than prefix[i].clock), followed by the second event.
   *
   * It is the sequence we want to insert into the wakeup tree.
   */
  std::vector<Branch> v;
  std::vector<const Event*> observers;
  std::vector<Branch> notobs;

  /* A below-clock including all excluded events */
  VClock<IPid> exclude_clock
    (std::vector<int>(threads.size(), std::numeric_limits<int>::max()));

  for (int k = i + 1; k < int(prefix.len()); ++k){
    assert(exclude == exclude_end || long(*exclude) >= k);
      const IID<IPid> &kiid = prefix[k].iid;
    if (exclude != exclude_end && *exclude == unsigned(k)) {
      /* XXX: We could just build the exclude clock in advance, and rely
       * on the second branch
       */
      exclude_clock[kiid.get_pid()]
        = std::min(exclude_clock[kiid.get_pid()], kiid.get_index());
      ++exclude;
    } else if (prefix[k].clock.intersects_below(exclude_clock)) {
      /* continue */
    } else if (!first.clock.leq(prefix[k].clock)) {
      v.emplace_back(branch_with_symbolic_data(k));
    } else if (race.kind == Race::OBSERVED && k != j) {
      if (!std::any_of(observers.begin(), observers.end(),
                       [this,k](const Event* o){
                         return o->clock.leq(prefix[k].clock); })){
        if (is_observed_conflict(first, second, prefix[k])){
          assert(!observers.empty() || k == race.witness_event);
          observers.push_back(&prefix[k]);
        } else if (race.kind == Race::OBSERVED) {
          notobs.emplace_back(branch_with_symbolic_data(k));
        }
      }
    }
  }
  if (race.kind == Race::NONBLOCK) {
    recompute_cmpxhg_success(second_br.sym, v, i);
  }
  v.push_back(std::move(second_br));
  if (race.kind == Race::OBSERVED) {
    int k = race.witness_event;
    Branch first_br = branch_with_symbolic_data(i);
    Branch witness_br = branch_with_symbolic_data(k);
    /* Only replay the racy event. */
    witness_br.size = 1;

    v.emplace_back(std::move(first_br));
    v.insert(v.end(), std::make_move_iterator(notobs.begin()),
             std::make_move_iterator(notobs.end()));
    notobs.clear(); /* Since their states are undefined after std::move */
    v.emplace_back(std::move(witness_br));
  }

  if (conf.dpor_algorithm == Configuration::OBSERVERS) {
    /* Recompute observed states on events in v */
    recompute_observed(v);
  }

  return v;
}

std::vector<int> TSOTraceBuilder::iid_map_at(int event) const{
  std::vector<int> map(threads.size(), 1);
  for (int i = 0; i < event; ++i) {
    iid_map_step(map, prefix.branch(i));
  }
  return map;
}

void TSOTraceBuilder::iid_map_step(std::vector<int> &iid_map, const Branch &event) const{
  if (iid_map.size() <= unsigned(event.pid)) iid_map.resize(event.pid+1, 1);
  iid_map[event.pid] += event.size;
}

void TSOTraceBuilder::iid_map_step_rev(std::vector<int> &iid_map, const Branch &event) const{
  iid_map[event.pid] -= event.size;
}

VClock<int> TSOTraceBuilder::reconstruct_blocked_clock(IID<IPid> event) const {
  VClock<IPid> ret;
  /* Compute the clock of the blocking process (event k in prefix is
   * something unrelated since this is a blocked event) */
  /* Find last event of p before this event */
  IPid p = event.get_pid();
  if (event.get_index() != 1) {
    int last = find_process_event(p, event.get_index()-1);
    ret = prefix[last].clock;
  }
  /* Recompute the clock of this mutex_lock_fail */
  ++ret[p];

  return ret;
}

TSOTraceBuilder::Event TSOTraceBuilder::
reconstruct_blocked_event(const Race &race) const {
  assert(race.is_fail_kind());
  Event ret(race.second_process);
  ret.clock = reconstruct_blocked_clock(race.second_process);

  if (race.kind == Race::LOCK_FAIL) {
    assert(std::any_of(prefix[race.first_event].sym.begin(),
                       prefix[race.first_event].sym.end(),
                       [](const SymEv &e){ return e.kind == SymEv::M_LOCK
                           || e.kind == SymEv::FULLMEM; }));
    ret.sym = prefix[race.first_event].sym;
  } else {
    assert(race.kind == Race::SEQUENCE);
    ret.sym = {race.ev};
  }
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

#ifndef NDEBUG
#  define IFDEBUG(X) X
#else
#  define IFDEBUG(X) ((void)0)
#endif

void TSOTraceBuilder::wakeup(Access::Type type, SymAddr ml){
  IPid pid = curev().iid.get_pid();
  IFDEBUG(sym_ty ev);
  std::vector<IPid> wakeup; // Wakeup these
  switch(type){
  case Access::W_ALL_MEMORY:
    {
      IFDEBUG(ev.push_back(SymEv::Fullmem()));
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
      IFDEBUG(ev.push_back(SymEv::Load(SymAddrSize(ml,1))));
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
      /* We don't pick the right value, but it should not matter */
      IFDEBUG(ev.push_back(SymEv::Store({SymAddrSize(ml,1), 1})));
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
  if (conf.dpor_algorithm != Configuration::SOURCE) {
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

long double TSOTraceBuilder::estimate_trace_count() const{
  return estimate_trace_count(0);
}

long double TSOTraceBuilder::estimate_trace_count(int idx) const{
  if(idx > int(prefix.len())) return 0;
  if(idx == int(prefix.len())) return 1;

  long double count = 1;
  for(int i = int(prefix.len())-1; idx <= i; --i){
    count += prefix[i].sleep_branch_trace_count;
    count += std::max(0, int(prefix.children_after(i)))
      * (count / (1 + prefix[i].sleep.size()));
  }

  return count;
}
