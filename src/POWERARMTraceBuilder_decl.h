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

#ifndef __POWERARM_TRACE_BUILDER_H__
#define __POWERARM_TRACE_BUILDER_H__

namespace PATB_impl{

  /* An event may perform memory accesses. Each access is a store or a
   * load to some memory location. This class defines a memory
   * accesses in the same sense as how the Interpreter considers
   * memory accesses (in contrast to ByteAccess below).
   */
  class Access{
  public:
    enum Type {
      LOAD,
      STORE
    };
    /* Create an access of type tp. Address and data are unknown. */
    Access(Type tp)
      : type(tp), addr(0,1), data(addr,1),
        addr_known(false), data_known(false),
        load_req(0) {};
    /* Is this a store or a load? */
    Type type;
    /* The memory location accessed. */
    MRef addr;
    /* If this is a store, then data is the data that is being
     * written. Otherwise, data is undefined.
     */
    MBlock data;
    /* Is the address known yet (has it been registered)? */
    bool addr_known;
    /* Is the data known yet? (Always false for loads.) */
    bool data_known;
    /* Null if there are no particular requirements on the source of
     * the load. Otherwise, a source for the load access is only
     * considered acceptable if *load_req is satisfied by the loaded
     * value.
     */
    POWERARMTraceBuilder::ldreqfun_t *load_req;
    /* Is address and data (if a store) known? */
    bool satisfied() const { return addr_known && (type != STORE || data_known); };
  };

  /* An Access corresponds to a number of byte-sized accesses. These
   * are represented as ByteAccess objects. Most of what is done by
   * the POWERARMTraceBuilder is done on the level of ByteAccess,
   * rather than on the level of Access.
   */
  class ByteAccess{
  public:
    enum Type {
      LOAD,
      STORE
    };
    ByteAccess(Type tp, void *addr) // Load or store
      : type(tp), addr(addr), data(0), data_known(false) {};
    ByteAccess(void *addr, char data) // Store
      : type(STORE), addr(addr), data(data), data_known(true) {};
    /* Is this a store or a load? */
    Type type;
    /* The address of the accessed byte. Guaranteed to be known. */
    void *addr;
    /* If a store, then data is the data that is being
     * written. Otherwise, data is undefined. */
    char data;
    /* Is the data known? (Always false for loads.) */
    bool data_known;
    /* Is address and data (if a store) known? */
    bool satisfied() const { return type != STORE || data_known; };
  };

  /* Rel objects are used to encode relations over events (such as co,
   * rf, com, hb, ...). For each such relation, each event will be
   * equipped with a Rel object, which contains pointers forward and
   * backward to other events which are related to that
   * event. Transitive pointers may be missing.
   */
  class Rel{
  public:
    /* A set of identifiers of events which are related after this
     * event. If this event is e, then for all events e' in fwd it
     * holds that e->e'. Furthermore, fwd contains at least all events
     * e' such that e->e' and there is no e'' such that e->e''->e'.
     */
    VecSet<IID<int> > fwd;
    /* A set of identifiers of events which are related before this
     * event. If this event is e, then for all events e' in fwd it
     * holds that e'->e. Furthermore, fwd contains at least all events
     * e' such that e'->e and there is no e'' such that e'->e''->e.
     */
    VecSet<IID<int> > bwd;
  };

  /* A Relations object collects the relations of one event. */
  class Relations{
  public:
    Relations() = default;
    Relations(Relations&&) = default;
    Relations(const Relations&) = default;
    Relations &operator=(Relations&&) = default;
    Relations &operator=(const Relations&) = default;
    /* Refer to [Alglave,Maranget,Tautschnig,TOPLAS 2014] for the
     * meaning of these relations. Here they are extended to
     * byte-level accesses.
     */
    Rel rf;
    Rel co;
    Rel com;
    Rel poloc;
    Rel ii0;
    Rel ci0;
    Rel cc0;
    Rel hb; // Actually the transitive, irreflexive closure hb+
    Rel prop;
  };

  /* An ExtraHB object describes a set of extra hb edges which are
   * not entered into the event structure.
   *
   * The new edges are (I*after_I) U (C*after_C).
   *
   * Invariant: I is a subset of C.
   * Invariant: after_I and after_C are disjoint.
   */
  struct ExtraHB{
    VecSet<IID<int> > I;
    VecSet<IID<int> > C;
    VecSet<IID<int> > after_I;
    VecSet<IID<int> > after_C;
  };
  /* An ExtraRel object describes a set of extra edges in hb and
   * prop, which are not entered into the event structure.
   *
   * The new edges are the hb edges in EHB and the prop edges (e,e')
   * for all events e,e' such that e' in prop[e].
   */
  struct ExtraRel{
    ExtraHB EHB;
    std::map<IID<int>,VecSet<IID<int> > > prop;
  };

  /* A Param represents one set of choices for one event: The parameter
   * decides a coherence position for each byte-store, and a source
   * for each byte-load of the event.
   */
  class Param{
  public:
    /* A Choice represents either a coherence position for a store or
     * a source for a load. The Choice represents a coherence position
     * iff 0<=co_pos.
     */
    class Choice{
    public:
      Choice(int cp) : co_pos(cp) { assert(0 <= cp); }; // coherence choice
      Choice(IID<int> src) : co_pos(-1), src(src) {}; // source choice
      /* co_pos represents a position in the coherence order at the
       * time that this event is committed. The represented position
       * is the position such that the event is preceded by precisely
       * co_pos other store events (initial store not counted, since
       * it is not an actual event) in co-order. Note that other
       * stores may be inserted into the co order later (after this
       * event is committed), so a store will not necessarily remain
       * at the coherence position indicated by co_pos.
       */
      int co_pos;
      /* The store event from which this event gets its read
       * value. Null if this event reads the initial value.
       */
      IID<int> src;
      bool is_coherence_choice() const { return 0 <= co_pos; };
      bool is_source_choice() const { return !is_coherence_choice(); };
      bool operator<(const Choice &C) const{
        if(is_coherence_choice() != C.is_coherence_choice()){
          return is_coherence_choice();
        }
        if(is_coherence_choice()){
          return co_pos < C.co_pos;
        }else{
          return src < C.src;
        }
      };
      bool operator==(const Choice &C) const{
        if(is_coherence_choice()){
          return co_pos == C.co_pos; // Note: this also implies C.is_coherence_choice()
        }else{
          return !C.is_coherence_choice() && src == C.src;
        }
      };
    };
    /* A vector containing the choices for each byte-access. The
     * choice choices[i] corresponds to the byte-access baccesses[i]
     * for the event. choices[i] is a coherence position if the
     * corresponding byte-access is a store. Otherwise, choices[i] is
     * a source.
     */
    std::vector<Choice> choices;
    /* The relations which are implied by these choices.
     *
     * rel only includes relations with events which were committed
     * before this event.
     *
     * Note that even when this object is related to another event
     * evt' according to rel, the same relation is not necessarily
     * reflected in evt'. This happens in particular when this event
     * is not committed, or committed with a different parameter.
     */
    Relations rel;
    ExtraRel ER;

    bool operator<(const Param &B) const { return choices < B.choices; };
    bool operator==(const Param &B) const { return choices == B.choices; };
  };

  class Branch{
  public:
    struct PEvent{
      IID<int> iid;
      Param param;
      bool operator<(const PEvent &e) const { return iid < e.iid || (iid == e.iid && param < e.param); };
      bool operator==(const PEvent &e) const { return iid == e.iid && param == e.param; };
    };
    std::vector<PEvent> branch;
    /* param_type describes how e.recalc_params and
     * e.recalc_params_load_src should be set up for events that are
     * in this locked branch.
     */
    enum ParamType {
      /* Assign RECALC_PARAM_NO for all events except the last
       * two. Assign RECALC_PARAM_STAR to the second to last
       * event. For the last event, assign RECALC_PARAM_LOAD_SRC to
       * recalc_params, and the IID of the second to last event to
       * recalc_params_load_src.
       */
      PARAM_WR,
      /* Assign RECALC_PARAM_NO for all events except the last
       * one. Assign RECALC_PARAM_STAR to the last event.
       */
      PARAM_STAR
    };
    ParamType param_type;
    bool operator<(const Branch &B) const{
      if(param_type != B.param_type) return param_type < B.param_type;
      /* Shorter branches are considered less than longer
       * branches. Branches of equal length are compared
       * lexicographically.
       */
      if(branch.size() != B.branch.size()) return branch.size() < B.branch.size();
      for(unsigned i = 0; i < branch.size(); ++i){
        if(!(branch[i] == B.branch[i])) return branch[i] < B.branch[i];
      }
      return false;
    };
    bool operator==(const Branch &B) const{
      if(param_type != B.param_type) return false;
      if(branch.size() != B.branch.size()) return false;
      for(unsigned i = 0; i < branch.size(); ++i){
        if(!(branch[i] == B.branch[i])) return false;
      }
      return true;
    };
  };

  /* An event is some atomic operation performing memory accesses. */
  class PAEvent{
  public:
    PAEvent(int load_count, int store_count,
            const VecSet<int> addr_deps, const VecSet<int> &data_deps,
            int eieio_idx, int lwsync_idx, int sync_idx,
            int clk_idx)
      : committed(false), filled_status(STATUS_COMPLETE), load_count(load_count),
        has_load(0 < load_count), has_store(0 < store_count),
        unknown_addr_count(load_count+store_count),
        unknown_data_count(store_count),
        addr_deps(addr_deps), data_deps(data_deps),
        eieio_idx_before(eieio_idx), lwsync_idx_before(lwsync_idx), sync_idx_before(sync_idx),
        eieio_idx_after(-1), lwsync_idx_after(-1), sync_idx_after(-1),
        branch_start(-1), branch_end(-1), clock_index(clk_idx), poloc_bwd_computed(false) {
      accesses.reserve(load_count+store_count);
      accesses.resize(load_count,Access(Access::LOAD));
      accesses.resize(load_count+store_count,Access(Access::STORE));
      cb_bwd.set(clock_index);
      recalc_params = RECALC_NO;
    };
    /* The identifier of this event. */
    IID<int> iid;
    /* Is the event committed? */
    bool committed;
    /* filled_status indicates which parts of this object are defined.
     */
    enum EventFilledStatus {
      /* All parts of this object are undefined except
       * filled_status. This is the case when the object is only kept
       * as a space that will later be filled in with an event.
       */
      STATUS_EMPTY,
      /* The only defined attributes of this object are:
       * - filled_status
       * - iid
       * - cur_param.choices (Relations are not defined)
       * - branch_start
       * - branch_end
       * - cur_branch
       * - new_branches
       * - new_params
       * - recalc_params
       * - recalc_params_load_src
       *
       * This is the case when the object has been prepared for being
       * fetched as a part of a locked branch.
       */
      STATUS_PARAMETER,
      /* All parts of this object are defined. It is a complete event.
       */
      STATUS_COMPLETE
    };
    EventFilledStatus filled_status;
    /* All accesses of this event. accesses[i] is the access with
     * Accid i. (It follows that all loads come before all stores in
     * accesses.)
     */
    std::vector<Access> accesses;
    /* The number of loads in accesses. */
    int load_count;
    /* All byte accesses of this event corresponding to accesses in
     * the vector accesses where the address is known.
     */
    std::vector<ByteAccess> baccesses;
    /* The addresses of the bytes accessed by this event. */
    VecSet<void*> access_tgts;
    /* True iff there is at least one load in accesses. */
    bool has_load;
    /* True iff there is at least one store in accesses. */
    bool has_store;
    /* The number of addresses which are not yet known for this event. */
    int unknown_addr_count;
    /* The number of data values which are not yet known for this event. */
    int unknown_data_count;
    bool all_addr_and_data_known() const {
      return unknown_addr_count == 0 && unknown_data_count == 0;
    };
    /* The indices of the events which are address dependencies of
     * this event. I.e. i is in addr_deps iff IID(proc,i) is an
     * address dependency of this event, where proc ==
     * this->iid.get_pid().
     */
    VecSet<int> addr_deps;
    /* The indices of the events which are data dependencies of this
     * event.
     */
    VecSet<int> data_deps;
    /* Indices of the events which are ctrl+isync dependencies for
     * this event.
     */
    VecSet<int> ctrl_isync_deps;
    /* The indices of the events which are ctrl dependencies of this
     * event, but which are not ctrl+isync dependencies of this event.
     */
    VecSet<int> ctrl_deps;
    /* -1 if this event is not po-preceded (resp. po-succeeded) by any
     * eieio (resp. lwsync, sync) fence. Otherwise, the number of
     * events which po-precede (resp. po-succeed) the last eieio
     * (resp. lwsync, sync) fence which itself po-precedes
     * (resp. po-succeedes) this event.
     */
    int eieio_idx_before;
    int lwsync_idx_before;
    int sync_idx_before;
    int eieio_idx_after;
    int lwsync_idx_after;
    int sync_idx_after;

    /* If no choices have been made for coherence and read-from for
     * this event, then cur_param is empty. Otherwise, cur_param
     * contains the current choices for this event.
     */
    Param cur_param;
    /* The set of parameters that have been identified and checked for
     * compliance with the memory model, but which have not yet been
     * explored.
     */
    std::vector<Param> new_params;
    /* For events which remain in the prefix when an execution is
     * reset (in particular those that are in locked branches),
     * recalc_params and recalc_params_load_src determine whether new
     * parameters should be calculated with a call to
     * populate_branches when the event is committed again.
     *
     * - RECALC_NO: Don't recompute. Use the existing parameters.
     *
     * - RECALC_STAR: Recompute completely, in the same way as when an
     *   event is committed during free exploration.
     *
     * - RECALC_LOAD: Recompute as for RECALC_STAR. Then filter the
     *   parameter alternatives to remove all parameters except those
     *   where this events has an incoming read from-edge from the
     *   event indicated by recalc_params_load_src.
     *
     * - RECALC_PARAM: Recompute as for RECALC_STAR. Then remove all
     *   parameters except for the one which matches cur_param. This
     *   is done in order to recompute the relations associated with
     *   the parameter in cur_param.
     *
     * Intended usage of recalc_params and recalc_params_load_src:
     *
     * All locked branches have the following shape:
     *
     * e0[p0].e1[p1]. ... .en[pn].w0[pw].r0[w0]
     *
     * where w0 is a store, and r0 is a load that should read from
     * w0. When such a branch is locked in the prefix in a call to
     * reset(), recalc_params and recalc_params_load_src are set up as
     * follows:
     *
     * e0.recalc_params = RECALC_PARAM
     * e1.recalc_params = RECALC_PARAM
     * ...
     * en.recalc_params = RECALC_PARAM
     * w0.recalc_params = RECALC_STAR
     * r0.recalc_params = RECALC_LOAD
     * r0.recalc_params_load_src = w0
     *
     * The purpose of this is to recalculate relations for all ei
     * events without changing the parameter, to allow w0 to take any
     * allowed parameter, and to allow r0 to take any allowed
     * parameter provided that r0 loads from w0.
     *
     * When the parameters have been recalculated once, recalc_params
     * is assigned RECALC_NO, so that recalculation is only done the
     * first time a particular locked branch appears in the prefix.
     */
    enum RecalcParams {
      RECALC_NO,
      RECALC_STAR,
      RECALC_LOAD,
      RECALC_PARAM
    };
    RecalcParams recalc_params;
    IID<int> recalc_params_load_src;
    /* The locked branches that should be explored in order to reverse
     * R->W races from this (load) event.
     */
    VecSet<Branch> new_branches;
    /* If this event is part of a locked branch, then branch_start is
     * the index of the first event on that branch and branch_end is
     * the index of the last event on that branch. Otherwise,
     * branch_start and branch_end are -1.
     */
    int branch_start, branch_end;
    bool in_locked_branch() const { return 0 <= branch_start; };
    /* If this event is part of a locked branch, and branch_start is
     * the index in prefix of this event, then cur_branch is that
     * locked branch. Otherwise, cur_branch is undefined.
     */
    Branch cur_branch;

    Relations rel;
    BVClock cb_bwd;
    /* The index of this event in BVClocks (e.g. cb_bwd). */
    int clock_index;
    /* Set if rel.poloc.bwd has been populated. */
    bool poloc_bwd_computed;

    /* Temporary value used for colouring during graph
     * searches. Always clear this value before starting a graph
     * colouring algorithm.
     */
    int colour;

    /* A one-line string representation of the parameter B for this
     * event.
     */
    std::string to_string(const Param &B) const{
      std::stringstream ss;
      for(unsigned i = 0; i < B.choices.size(); ++i){
        if(i != 0) ss << " ";
        const Param::Choice &C = B.choices[i];
        if(C.is_coherence_choice()){
          ss << "ST@" << long(baccesses[i].addr) << ":#" << C.co_pos;
        }else{
          ss << "LD@" << long(baccesses[i].addr) << ":"
             << (C.src.is_null() ? std::string("init") : C.src.to_string());
        }
      }
      return ss.str();
    };

    /* Returns true iff any access of this event has a load_req */
    bool has_load_req() const {
      for(int i = 0; i < load_count; ++i){
        if(accesses[i].load_req){
          return true;
        }
      }
      return false;
    };
  };

  /* POWER specific version of PAEvent. */
  class POWEREvent : public PAEvent{
  public:
    POWEREvent(int load_count, int store_count,
               const VecSet<int> addr_deps, const VecSet<int> &data_deps,
               int eieio_idx, int lwsync_idx, int sync_idx, int clk_idx)
      : PAEvent(load_count,store_count,addr_deps,data_deps,
                eieio_idx,lwsync_idx,sync_idx,clk_idx) {};
  };

  /* ARM specific version of PAEvent. */
  class ARMEvent : public PAEvent{
  public:
    ARMEvent(int load_count, int store_count,
             const VecSet<int> addr_deps, const VecSet<int> &data_deps,
             int eieio_idx, int lwsync_idx, int sync_idx,
             int clk_idx)
      : PAEvent(load_count,store_count,addr_deps,data_deps,
                eieio_idx,lwsync_idx,sync_idx,clk_idx) {};
  };

  /* A Mem object contains information about the accesses to one
   * particular byte in memory. (See TB::mem.)
   */
  template<MemoryModel MemMod,CB_T CB,class Event>
  class Mem{
  public:
    Mem() : loads(1) {};
    /* The coherence vector contains all committed events that store
     * to this byte, in co-order.
     */
    std::vector<IID<int> > coherence;
    /* loads contains all committed events that load from this
     * byte. loads[0] contains all loads that load the initial value
     * of this byte. loads[i] for 0<i<loads.size() contains all loads
     * that load from coherence[i-1].
     */
    std::vector<VecSet<IID<int> > > loads;
    /* Update this Mem object to reflect that the event evt is
     * committed, with choice C made for its access A, where A.addr is
     * the address of the byte tracked by this Mem object.
     */
    void commit(Event &evt, const ByteAccess &A, const Param::Choice &C);
  };

  /* A TraceRecorder helps the TraceBuilder to record a human-readable
   * string representation of a trace.
   */
  class TraceRecorder : public Trace{
  public:
    TraceRecorder(const std::vector<CPid> *cpids) : Trace({}), cpids(cpids), active(false) {};
    /* Get the recorded trace representation. */
    std::string to_string(int ind = 0) const;
    /* Called when an event is committed by the TraceBuilder. */
    void trace_commit(const IID<int> &iid, const Param &param, const std::vector<Access> &accesses,
                      const std::vector<ByteAccess> &baccesses, std::vector<MBlock> values);
    /* Called when the ExecutionEngine committed an instruction */
    void trace_register_metadata(int proc, const llvm::MDNode *md);
    /* Called when the ExecutionEngine committed a call to an external function. */
    void trace_register_external_function_call(int proc, const std::string &fname, const llvm::MDNode *md);
    /* Called when the ExecutionEngine entered a function.*/
    void trace_register_function_entry(int proc, const std::string &fname, const llvm::MDNode *md);
    /* Called when the ExecutionEngine returned from a function.*/
    void trace_register_function_exit(int proc);
    /* Called when the ExecutionEngine detects an error. */
    void trace_register_error(int proc, const std::string &err_msg);
    /* Clear the recorded trace */
    void clear() { lines.clear(); last_committed.consumed = true; fun_call_stack.clear(); };
    void activate() { active = true; };
    void deactivate() { active = false; };
    bool is_active() const { return active; };
  private:
    const std::vector<CPid> *cpids;
    bool active;
    struct Committed{
      Committed() : consumed(true) {};
      bool consumed;
      IID<int> iid;
      Param param;
      std::vector<Access> accesses;
      std::vector<ByteAccess> baccesses;
      std::vector<MBlock> values;
    };
    Committed last_committed;
    /* For each thread p, fun_call_stack[p] is a vector containing the
     * names of the functions on its call stack. Older calls are
     * earlier in the vector.
     */
    std::vector<std::vector<std::string> > fun_call_stack;
    /* An Str object is some human-readable string (line) that is part
     * of a description of a recorded trace. The line is associated
     * with a particular thread. The thread may influence how much the
     * line should be indented.
     */
    struct Line{
      int proc;
      std::string ln;
    };
    std::vector<Line> lines;

    /* Attempt to find the source code line associated with md.
     *
     * On success, true is returned. On success, *srcln is assigned a
     * string on the form "(ID): FILE:LN SRC" where ID is either an
     * IID<CPid> or just a CPid corresponding to proc, FILE is the
     * base name of the source code file, LN is the line number in the
     * source code, and SRC is the actual source code line. On
     * success, *id_len is assigned the length of the string "(ID): ".
     *
     * On failure, false is returned. On failure, *srcln is assigned a
     * string on the form "(ID): " where ID is either an IID<CPid> or
     * just a CPid corresponding to proc. On failure, *id_len is
     * assigned the length of the string "(ID): ".
     */
    bool source_line(int proc, const llvm::MDNode *md, std::string *srcln, int *id_len);
  };

  template<MemoryModel MemMod,CB_T CB,class Event>
  class TB : public POWERARMTraceBuilder {
  public:
    TB(const Configuration &conf = Configuration::default_conf);
    virtual ~TB();

    virtual IID<int> fetch(int proc,
                           int load_count,
                           int store_count,
                           const VecSet<int> &addr_deps,
                           const VecSet<int> &data_deps);
    virtual void register_addr(const IID<int> &iid, Accid a, const MRef &addr);
    virtual void register_data(const IID<int> &iid, Staccid a, const MBlock &data);
    virtual void register_load_requirement(const IID<int> &iid, Ldaccid a, ldreqfun_t *f);
    virtual void fetch_ctrl(int proc, const VecSet<int> &deps);
    virtual void fetch_fence(int proc, FenceType type);

    virtual bool schedule(IID<int> *iid, std::vector<MBlock> *values);

    virtual bool check_for_cycles(){
      throw std::logic_error("POWERARMTraceBuilder::check_for_cycles: Not supported.");
    };
    virtual int spawn(int proc);
    virtual void abort();
    virtual Trace *get_trace() const;
    virtual bool reset();
    virtual void replay();
    virtual IID<CPid> get_iid() const;
    virtual bool sleepset_is_empty() const;
    virtual void debug_print() const;
    virtual void trace_register_metadata(int proc, const llvm::MDNode *md);
    virtual void trace_register_external_function_call(int proc, const std::string &fname, const llvm::MDNode *md);
    virtual void trace_register_function_entry(int proc, const std::string &fname, const llvm::MDNode *md);
    virtual void trace_register_function_exit(int proc);
    virtual void trace_register_error(int proc, const std::string &err_msg);
  private:
    /* prefix specifies an ordered sequence events as they occur in a
     * computation. The first sched_count events in prefix make up the
     * current (committed) computation. Any events after the first
     * sched_count events specify how this computation should
     * continue.
     *
     * The details about each event are kept in the vector fch below.
     */
    std::vector<IID<int> > prefix;
    /* The number of events that have been committed in the current
     * computation.
     */
    int sched_count;
    /* The number of events that have been fetched in the current
     * computation.
     */
    int fetch_count;
    /* The number of events that are fetched but not committed, and
     * which do not have load_req on any of its accesses.
     */
    int uncommitted_nonblocking_count;
    /* fch contains all currently fetched (and possibly committed)
     * events. fch also contains some yet unfetched events, and some
     * "empty" events (PAEvent::is_empty).
     *
     * fch[proc] contains the events belonging to thread proc. For
     * each i such that 0 <= i < fch[proc].size(), it holds that
     * either
     *
     * 1) fch[proc][i] is empty (i.e. fch[proc][i].is_empty). This
     *    means that the data in fch[proc][i] should be considered
     *    undefined. fch[proc][i] should be used as a space for the
     *    event (proc,i+1), when that event is eventually fetched.
     *
     * 2) fch[proc][i] is not empty. Then fch[proc][i] contains all
     *    currently available information about the event (proc,i+1).
     *    This will be the case precisely when either the event
     *    (proc,i+1) is fetched, or the event (proc,i+1) is part of
     *    the uncommitted part of prefix. In the latter case,
     *    fch[proc][i] is then used to store the choices of read-from
     *    and coherence position that should be used for the event.
     */
    std::vector<std::vector<Event> > fch;

    /* Various information about a thread which is currently running
     * in this computation.
     */
    class Thread{
    public:
      Thread()
        : fch_count(0), committed_prefix(0), addr_known_prefix(0),
          last_fetched_eieio_idx(-1),
          last_fetched_lwsync_idx(-1),
          last_fetched_sync_idx(-1) {};
      /* The number of events which have been fetched for this thread
       * in the current computation.
       */
      int fch_count;
      /* The number of consecutive committed events of this thread
       * starting from and including the first fetched event.
       */
      int committed_prefix;
      /* The number of consecutive fetched events with no pending
       * address dependencies of this thread starting from and
       * including the first fetched event.
       */
      int addr_known_prefix;
      /* Indices of the events which are ctrl+isync dependencies for
       * all events fetched hereafter.
       */
      VecSet<int> ctrl_isync_deps;
      /* Indices of the events which are ctrl dependencies for all
       * events fetched hereafter, but which are not ctrl+isync
       * dependencies.
       */
      VecSet<int> ctrl_deps;
      /* -1 if no eieio (resp. lwsync, sync) fence has been fetched by
       * this thread. Otherwise the number of events that po-precede
       * the last fetched eieio (resp. lwsync, sync) fence.
       */
      int last_fetched_eieio_idx;
      int last_fetched_lwsync_idx;
      int last_fetched_sync_idx;
    };
    /* threads[proc] is the thread proc. */
    std::vector<Thread> threads;
    /* Flag signalling that this computation has been aborted. */
    bool is_aborted;

    /* A map from the address of a byte in memory, to a Mem object
     * keeping track of information about accesses to that byte.
     */
    std::map<void*,Mem<MemMod,CB,Event> > mem;

    /* The clock index that will be used for the next fetched
     * event.
     */
    int next_clock_index;
    /* cpids[proc] is the CPid of thread proc. */
    std::vector<CPid> cpids;
    /* A CPidSystem of the threads that have been spawned in this
     * computation.
     */
    CPidSystem CPS;
    /* A trace recorder. (Typically only used when an error trace is
     * being replayed.)
     */
    TraceRecorder TRec;

    int get_prefix_index(const IID<int> &iid){
      int i = int(prefix.size())-1;
      while(0 <= i && prefix[i] != iid) --i;
      return i;
    };
    Event &get_evt(const IID<int> &iid) {
      assert(0 <= iid.get_pid() && iid.get_pid() < int(fch.size()));
      assert(0 < iid.get_index() && iid.get_index() <= int(fch[iid.get_pid()].size()));
      return fch[iid.get_pid()][iid.get_index()-1];
    };
    const Event &get_evt(const IID<int> &iid) const {
      assert(0 <= iid.get_pid() && iid.get_pid() < int(fch.size()));
      assert(0 < iid.get_index() && iid.get_index() <= int(fch[iid.get_pid()].size()));
      return fch[iid.get_pid()][iid.get_index()-1];
    };
    /* Update the value of committed_prefix in threads[proc].
     */
    void update_committed_prefix(int proc);
    /* Update the value of addr_known_prefix in threads[proc].
     */
    void update_addr_known_prefix(int proc);
    /* Returns true iff evt is neither committed nor sleeping and
     * there is no other uncommitted event by the same thread which
     * necessarily is cb-before evt.
     */
    bool committable(Event &evt);
    /* Attempts to commit evt. Returns true on success, false on
     * failure. Failure happens when there are no choices of read-from
     * and coherence relation such that evt can be committed without
     * violating either the memory model, some requirement from the
     * DPOR algorithm (e.g. repeating a previous computation), or some
     * requirement from the semantics of the event itself
     * (e.g. acquiring an already locked mutex lock). On failure,
     * evt.sleeping is set to true.
     *
     * If replay is set, then the choices for read-from and coherence
     * are taken from current value of evt.cur_param. Otherwise new
     * parameters are populated, and a new parameter is chosen for the
     * current choices.
     *
     * On success, *values is setup such that it has one element per
     * load access of evt. The i:th element of *values will be the
     * value read by the i:th load access of evt.
     *
     * Pre: committable(evt)
     */
    bool commit(Event &evt, std::vector<MBlock> *values, bool replay);
    /* Compute all memory model-allowed parameters for evt. Each allowed
     * Param is added to evt.new_params.
     *
     * Pre: committable(evt)
     */
    void populate_parameters(Event &evt);
    /* Helper for populate_parameters */
    void populate_parameters_store(Event &evt,int stidx,const ByteAccess &S,const Param &B,std::vector<Param> &parameters);
    /* Helper for populate_parameters */
    void populate_parameters_load(Event &evt,int ldidx,const ByteAccess &L,const Param &B,std::vector<Param> &parameters);
    /* Setup *values based on the read-from choices evt.cur_param.
     * Afterward *values has one element per load access of evt. The
     * i:th element of *values will be the value read by the i:th load
     * access of evt.
     *
     * Pre: committable(evt)
     * Pre: evt.cur_param is set up with an allowed parameter.
     */
    void get_cur_load_values(Event &evt, std::vector<MBlock> *values);
    /* Setup *values based on the read-from choices in B. Afterward
     * *values has one element per load access of evt. The i:th
     * element of *values will be the value read by the i:th load
     * access of evt.
     *
     * Pre: committable(evt)
     * Pre: B is an allowed parameter for evt.
     */
    void get_load_values(Event &evt, std::vector<MBlock> *values, const Param &B);
    /* Helper for commit.
     *
     * Identify all R->W conflicts: For each event r_evt which is
     * committed, different from w_evt, and which has a load access
     * for some byte for which w_evt has a store access, and where it
     * is not the case that r_evt is cb-before w_evt, we consider
     * r_evt and w_evt to be in R->W conflict.
     */
    void detect_rw_conflicts(Event &w_evt);
    /* Normalises B.
     *
     * Rearranges the events in B on a normal form that respects the
     * cb order.
     */
    void normalise_branch(Branch &B);
    /* Computes the cb relation for evt, and returns the corresponding
     * backward clock.
     *
     * Pre: The following are up to date in evt:
     * - addr_deps, data_deps, ctrl_deps, ctrl_isync_deps
     * - evt.rel.rf (only required if include_rf is true)
     * - evt.lwsync_idx_before, evt.sync_idx_before
     * - evt.rel.poloc
     *
     * If include_rf is false, then incoming rf-edges to evt are not
     * included in the computed cb relation.
     */
    BVClock compute_cb(const Event &evt, bool include_rf = true);

    /* Update the event structure with edges implied by evt being
     * committed with parameter evt.cur_param.
     */
    void setup_relations_cur_param(Event &evt);
    /* For each relation edge evt->evt' or evt'->evt which is recorded
     * in evt, record the same edge in evt'
     *
     * Helper for commit.
     */
    void return_relations(Event &evt);
    /* Assigns 0 to the colour field of all fetched events. */
    void clear_event_colour();
    /* Updates hb as the hb relation of evt, according the fences
     * relation indicated by evt.
     */
    void fences_to_hb(Event &evt, Rel &hb);
    /* Populates evt.rel.poloc.bwd. Does nothing if
     * evt.poloc_bwd_computed is already set.
     *
     * Pre: All addresses of this event and po-earlier events must be
     * known.
     */
    void compute_poloc_bwd(Event &evt);
    /* Identifies the closest po-predecessor and po-successor of evt
     * among the committed accesses to M.
     *
     * If there is no po-predecessor, then before_iid is set to
     * null. Otherwise, before_iid is set to the identifier of the
     * closest po-predecessor, and before_type is set to the kind of
     * access of that event. before_pos is set to the index into
     * M.coherence (if a store) or M.loads (if a load) where the
     * predecessor was found.
     *
     * after_iid, after_pos, after_type are set correspondingly, to
     * identify the po-successor.
     */
    void find_poloc(Event &evt, Mem<MemMod,CB,Event> &M,
                    IID<int> &before_iid, int &before_pos, ByteAccess::Type &before_type,
                    IID<int> &after_iid, int &after_pos, ByteAccess::Type &after_type);
    /* Performs a search of the committed events following edges in
     * com and poloc backwards. Starts from the events in inits. Marks
     * all visited events with a non-zero colour. Considers all events
     * which already have a non-zero colour to be already
     * visited. Returns true if any event in tgt is considered visited
     * after the search. Returns false otherwise.
     *
     * Used to check SC PER LOCATION.
     */
    bool com_poloc_bwd_search(const VecSet<IID<int> > &inits, const VecSet<IID<int> > &tgt);
    /* Computes the new hb edges that would be added if evt were
     * committed with the ii0, ci0, cc0 relations given by rel. The
     * new edges which are directly connected to evt are added to
     * rel.hb. EHB is modified to represent the set of the remaining
     * new hb edges.
     */
    void ppo_to_hb(Event &evt, Relations &rel, ExtraHB &EHB);
    /* Computes the new prop edges that would be added if evt were
     * committed with the relations rel, and the hb edges in EHB were
     * added to the event structure. The new prop edges are entered
     * into rel.prop and ER.prop. The prop edges from evt are only
     * added to rel, not to ER.prop.
     *
     * Pre: rel.hb and ER.EHB have been populated by ppo_to_hb.
     */
    void calc_prop(Event &evt, Relations &rel, ExtraRel &ER);
    /* Helper for calc_prop. */
    void calc_prop_inits(Event &evt, Relations &rel, ExtraHB &EHB, VecSet<Event*> &new_prop);
    /* Searches the event structure for cycles in hb. Returns true iff
     * hb is acyclic. The event structure which is considered is the
     * one consisting of all currently committed events, and the hb
     * edges in EHB, and also the event evt, assumed to have relations
     * as given in rel.
     */
    bool no_thin_air(Event &evt, Relations &rel, ExtraHB &EHB);
    /* Searches the event structure for cycles in co U prop. Returns
     * true iff co U prop is acyclic. The event structure which is
     * considered is the one consisting of all currently committed
     * events, and the hb and prop edges in ER, and also the event
     * evt, assumed to have relations as given in rel.
     */
    bool propagation(Event &evt, Relations &rel, ExtraRel &ER);
    /* Searches the event structure for reflexivity in fre;prop;hb*.
     * Returns true iff irreflexive(fre;prop;hb*). The event structure
     * which is considered is the one consisting of all currently
     * committed events, and the hb edges in EHB, and also the event
     * evt, assumed to have relations as given in rel.
     *
     * Pre: The OBSERVATION axiom holds for the event structure
     * without evt and EHB.
     *
     * Pre: The SC PER LOCATION, NO THIN AIR and PROPAGATION axioms
     * hold for the event structure including evt and EHB.
     */
    bool observation(Event &evt, Relations &rel, ExtraHB &EHB);
    /* Helper for observation.
     *
     * Assigns to inits a set of load events which is a superset of
     * all loads e such that (e,e) in fre;prop;hb*, under the same
     * assumptions as for the TB::observation method.
     *
     * Pre: The OBSERVATION axiom holds for the event structure
     * without evt and EHB.
     *
     * Pre: The SC PER LOCATION, NO THIN AIR and PROPAGATION axioms
     * hold for the event structure including evt and EHB.
     */
    void observation_find_inits(Event &evt, Relations &rel, ExtraHB &EHB,
                                VecSet<Event*> &inits);
    /* Helper for observation.
     *
     * Assigns to fre the set of stores w_evt such that (r_evt,w_evt)
     * is in fre. The same assumptions about the event structure are
     * made here as in observation.
     */
    void observation_get_fre(Event &r_evt, Event &new_evt, Relations &rel,
                             VecSet<Event*> &fre);
    /* Checks if the error e, encountered in this computation, should
     * be considered real. An encountered error may be considered not
     * real if it occurs in computations which turn out to be
     * forbidden by the memory model.
     *
     * Checked when the computation is finished.
     */
    bool error_is_real(Error &e) const {
      /* Check that all events that are before e in (po U rf) are
       * committed.
       */
      IID<CPid> ciid = e.get_location();
      int proc = 0;
      while(cpids[proc] != ciid.get_pid()) ++proc;
      std::vector<int> pfx(threads.size(),0);
      std::vector<IID<int> > stack;
      stack.push_back(IID<int>(proc,ciid.get_index()));
      while(stack.size()){
        IID<int> iid = stack.back();
        stack.pop_back();
        if(threads[iid.get_pid()].committed_prefix < iid.get_index()){
          return false;
        }
        for(int i = pfx[iid.get_pid()]; i < iid.get_index(); ++i){
          for(const IID<int> wiid : fch[iid.get_pid()][i].rel.rf.bwd){
            stack.push_back(wiid);
          }
        }
        pfx[iid.get_pid()] = iid.get_index();
      }
      return true;
    };

    /* Replaces A with the set A union C, where C is the set of all
     * elements in B except the smallest element in B.
     */
    void union_except_one(VecSet<Branch> &A, const VecSet<Branch> &B);

  };

  class PATrace : public Trace {
  public:
    struct Evt{
      IID<int> iid;
      Param param;
    };
    PATrace(const std::vector<Evt> &events,
            const std::vector<CPid> &cpids,
            const Configuration &conf,
            const std::vector<Error*> &errors,
            const std::string &str_rep = "",
            bool blocked = false)
      : Trace(errors,blocked), events(events), cpids(cpids), string_rep(str_rep), conf(conf) {};
    virtual ~PATrace(){};
    virtual std::string to_string(int ind = 0) const;
  protected:
    std::vector<Evt> events;
    std::vector<CPid> cpids;
    std::string string_rep;
    Configuration conf;
  };

}

#endif
