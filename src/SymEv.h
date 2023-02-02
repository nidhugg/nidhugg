/* Copyright (C) 2021 Magnus Lång
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

#ifndef __SYM_EV_H__
#define __SYM_EV_H__

#include "MRef.h"
#include "CPid.h"
#include "SymAddr.h"
#include "RMWAction.h"
#include "AwaitCond.h"

#include <llvm/Support/raw_ostream.h>

#include <functional>
#include <string>
#include <utility>

/* Symbolic representation of an event */
struct SymEv {
  public:
  enum kind {
    NONE,

    NONDET,

    LOAD,
    LOAD_AWAIT,
    STORE,
    FULLMEM, /* Observe & clobber everything */

    RMW,
    XCHG_AWAIT,
    CMPXHG,
    CMPXHGFAIL,

    M_INIT,
    M_LOCK,
    M_TRYLOCK,
    M_TRYLOCK_FAIL,
    M_UNLOCK,
    M_DELETE,

    C_INIT,
    C_SIGNAL,
    C_BRDCST,
    C_WAIT,
    C_AWAKE,
    C_DELETE,

    SPAWN,
    JOIN,

    UNOBS_STORE,
  } kind;
  union arg {
    SymAddrSize addr;
    int num;

    arg() {}
    arg(SymAddrSize addr) : addr(addr) {}
    arg(int num) : num(num) {}
    // ~arg_union() {}
  } arg;
  union arg2 {
    RmwAction::Kind rmw_kind;
    AwaitCond::Op await_op;

    arg2() {}
    arg2(RmwAction::Kind kind) : rmw_kind(kind) {}
    arg2(AwaitCond::Op await_op) : await_op(await_op) {}
  } arg2;
  bool _rmw_result_used;
  /* This is a dangerous thing. The enclosing SymEv is responsible for
   * destruction */
  union arg3 {
    struct data {SymData::block_type _expected, _written; } d;
    CPid cpid;
    arg3() {}
    arg3(struct data &&d) : d(std::move(d)) {}
    arg3(CPid cpid) : cpid(std::move(cpid)) {}
    ~arg3() {}
  } arg3;

  /* We're paying the price for having an union arg3 */
  ~SymEv();
  SymEv(const SymEv &o)
    : kind(o.kind), arg(o.arg), arg2(o.arg2), _rmw_result_used(o._rmw_result_used) {
    if (arg3_is_data())  new (&arg3) union arg3(arg3::data(o.arg3.d));
    else if (has_cpid()) new (&arg3) union arg3(o.arg3.cpid);
  }
  SymEv(SymEv && o)
    : kind(o.kind), arg(o.arg), arg2(o.arg2), _rmw_result_used(o._rmw_result_used) {
    if (arg3_is_data())  new (&arg3) union arg3(std::move(o.arg3.d));
    else if (has_cpid()) new (&arg3) union arg3(std::move(o.arg3.cpid));
  }
  SymEv &operator=(const SymEv &o)  { this->~SymEv(); return *new(this) SymEv(o); }
  SymEv &operator=(const SymEv &&o) { this->~SymEv(); return *new(this) SymEv(std::move(o)); }

  SymEv() : kind(NONE) {}
  static SymEv None() { return {NONE}; }
  static SymEv Nondet(int count) { return {NONDET, count}; }

  static SymEv Load(SymAddrSize addr) { return {LOAD, addr}; }
  static SymEv LoadAwait(SymAddrSize addr, AwaitCond cond) {
    return {LOAD_AWAIT, addr, std::move(cond)};
  }
  static SymEv Store(SymData addr) { return {STORE, std::move(addr)}; }
  static SymEv Rmw(SymData addr, RmwAction action) {
    assert(addr.get_block());
    return {RMW, std::move(addr), std::move(action)};
  }
  static SymEv XchgAwait(SymData addr, AwaitCond cond) {
    return {XCHG_AWAIT, std::move(addr), std::move(cond)};
  }
  static SymEv CmpXhg(SymData addr, SymData::block_type expected) {
    return {CMPXHG, addr, expected};
  }
  static SymEv CmpXhgFail(SymData addr, SymData::block_type expected) {
    return {CMPXHGFAIL, addr, expected};
  }
  static SymEv Fullmem() { return {FULLMEM}; }

  static SymEv MInit(SymAddrSize addr) { return {M_INIT, addr}; }
  static SymEv MLock(SymAddrSize addr) { return {M_LOCK, addr}; }
  static SymEv MTryLock(SymAddrSize addr) { return {M_TRYLOCK, addr}; }
  static SymEv MTryLockFail(SymAddrSize addr) { return {M_TRYLOCK_FAIL, addr}; }
  static SymEv MUnlock(SymAddrSize addr) { return {M_UNLOCK, addr}; }
  static SymEv MDelete(SymAddrSize addr) { return {M_DELETE, addr}; }

  static SymEv CInit(SymAddrSize addr) { return {C_INIT, addr}; }
  static SymEv CSignal(SymAddrSize addr) { return {C_SIGNAL, addr}; }
  static SymEv CBrdcst(SymAddrSize addr) { return {C_BRDCST, addr}; }
  static SymEv CWait(SymAddrSize cond) { return {C_WAIT, cond}; }
  static SymEv CAwake(SymAddrSize cond) { return {C_AWAKE, cond}; }
  static SymEv CDelete(SymAddrSize addr) { return {C_DELETE, addr}; }

  static SymEv Spawn(CPid proc) { return {SPAWN, std::move(proc)}; }
  static SymEv Join(CPid proc) { return {JOIN, std::move(proc)}; }

  static SymEv UnobsStore(SymData addr) {
    return {UNOBS_STORE, std::move(addr)};
  }

  bool is_compatible_with(SymEv other) const;
  std::string to_string(std::function<std::string(int)> pid_str
                        = (std::string(&)(int))std::to_string) const;

  bool operator==(const SymEv &s) const;
  bool operator!=(const SymEv &s) const { return !(*this == s); }

  bool has_addr() const;
  bool has_num() const;
  bool has_cpid() const { return kind == SPAWN || kind == JOIN; }
  bool has_data() const;
  bool has_expected() const;
  bool has_rmwaction() const { return kind == RMW; }
  bool has_cond() const { return kind == LOAD_AWAIT || kind == XCHG_AWAIT; }
  bool empty() const { return kind == NONE; }
  const SymAddrSize &addr()   const { assert(has_addr()); return arg.addr; }
        int          num()    const { assert(has_num()); return arg.num; }
  const CPid        &cpid()   const { assert(has_cpid()); return arg3.cpid; }
  SymData data() const { assert(has_data()); return {arg.addr, arg3.d._written}; }
  SymData expected() const {
    assert(has_expected());
    return {arg.addr, arg3.d._expected};
  }
  RmwAction rmwaction() const {
    assert(has_rmwaction());
    assert(arg3.d._expected);
    return {arg2.rmw_kind, arg3.d._expected, _rmw_result_used};
  }
  RmwAction::Kind rmw_kind() const {
    assert(has_rmwaction());
    return arg2.rmw_kind;
  }
  bool rmw_result_used() const {
    assert(has_rmwaction());
    return _rmw_result_used;
  }
  AwaitCond cond() const {
    assert(has_cond());
    assert(arg3.d._expected);
    return {arg2.await_op, arg3.d._expected};
  }

  void purge_data();
  void set_observed(bool observed);

private:
  SymEv(enum kind kind, union arg arg = {}) : kind(kind), arg(arg) {}
  SymEv(enum kind kind, CPid &&cpid) : kind(kind), arg3(std::move(cpid)) {}
  SymEv(enum kind kind, union arg arg, AwaitCond cond)
    : kind(kind), arg(arg), arg2(cond.op),
      arg3(arg3::data{std::move(cond.operand), {}}) {}
  SymEv(enum kind kind, SymData addr_written)
    : kind(kind), arg(std::move(addr_written.get_ref())),
      arg3(arg3::data{{}, std::move(addr_written.get_shared_block())}) {}
  SymEv(enum kind kind, SymData addr_written, SymData::block_type expected)
    : kind(kind), arg(std::move(addr_written.get_ref())),
      arg3(arg3::data{std::move(expected),
                      std::move(addr_written.get_shared_block())}) {}
  SymEv(enum kind kind, SymData addr_written, RmwAction action)
    : kind(kind), arg(addr_written.get_ref()), arg2(action.kind),
      _rmw_result_used(action.result_used),
      arg3(arg3::data{std::move(action.operand),
                      std::move(addr_written.get_shared_block())}) {
      assert(has_data());
    }
  SymEv(enum kind kind, SymData addr_written, AwaitCond cond)
    : kind(kind), arg(addr_written.get_ref()), arg2(cond.op),
      arg3(arg3::data{std::move(cond.operand),
                      std::move(addr_written.get_shared_block())}) {
      assert(has_data());
    }

  bool arg3_is_data() const;
};

inline std::ostream &operator<<(std::ostream &os, const SymEv &e){
  return os << e.to_string();
}

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const SymEv &e){
  return os << e.to_string();
}

#endif
