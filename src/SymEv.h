/* Copyright (C) 2017 Magnus Lång
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

#include <functional>

#include <llvm/Support/raw_ostream.h>

#include "MRef.h"
#include "SymAddr.h"

/* Symbolic representation of an event */
struct SymEv {
  public:
  enum kind {
    // EMPTY,
    NONDET,

    LOAD,
    STORE,
    FULLMEM, /* Observe & clobber everything */

    CMPXHG,
    CMPXHGFAIL,

    M_INIT,
    M_LOCK,
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
  public:
    SymAddrSize addr;
    int num;

    arg() {}
    arg(SymAddrSize addr) : addr(addr) {}
    arg(int num) : num(num) {}
    // ~arg_union() {}
  } arg;
  SymData::block_type _expected, _written;

  SymEv() = delete;
  // SymEv() : kind(EMPTY) {};
  // static SymEv Empty() { return {EMPTY, {}}; }
  static SymEv Nondet(int count) { return {NONDET, count}; }

  static SymEv Load(SymAddrSize addr) { return {LOAD, addr}; }
  static SymEv Store(SymData addr) { return {STORE, std::move(addr)}; }
  static SymEv CmpXhg(SymData addr, SymData::block_type expected) {
    return {CMPXHG, addr, expected};
  }
  static SymEv CmpXhgFail(SymData addr, SymData::block_type expected) {
    return {CMPXHGFAIL, addr, expected};
  }
  static SymEv Fullmem() { return {FULLMEM, {}}; }

  static SymEv MInit(SymAddrSize addr) { return {M_INIT, addr}; }
  static SymEv MLock(SymAddrSize addr) { return {M_LOCK, addr}; }
  static SymEv MUnlock(SymAddrSize addr) { return {M_UNLOCK, addr}; }
  static SymEv MDelete(SymAddrSize addr) { return {M_DELETE, addr}; }

  static SymEv CInit(SymAddrSize addr) { return {C_INIT, addr}; }
  static SymEv CSignal(SymAddrSize addr) { return {C_SIGNAL, addr}; }
  static SymEv CBrdcst(SymAddrSize addr) { return {C_BRDCST, addr}; }
  static SymEv CWait(SymAddrSize cond) { return {C_WAIT, cond}; }
  static SymEv CAwake(SymAddrSize cond) { return {C_AWAKE, cond}; }
  static SymEv CDelete(SymAddrSize addr) { return {C_DELETE, addr}; }

  static SymEv Spawn(int proc) { return {SPAWN, proc}; }
  static SymEv Join(int proc) { return {JOIN, proc}; }

  static SymEv UnobsStore(SymData addr) {
    return {UNOBS_STORE, std::move(addr)};
  }

  void set(SymEv other);
  std::string to_string(std::function<std::string(int)> pid_str
                        = (std::string(&)(int))std::to_string) const;

  bool operator==(const SymEv &s) const;
  bool operator!=(const SymEv &s) const { return !(*this == s); };

  bool has_addr() const;
  bool has_num() const;
  bool has_data() const;
  bool has_expected() const;
  const SymAddrSize &addr()   const { assert(has_addr()); return arg.addr; }
        int          num()    const { assert(has_num()); return arg.num; }
  SymData data() const { assert(has_data()); return {arg.addr, _written}; }
  SymData expected() const {
    assert(has_expected());
    return {arg.addr, _expected};
  }

  void purge_data();
  void set_observed(bool observed);

private:
  SymEv(enum kind kind, union arg arg) : kind(kind), arg(arg) {};
  SymEv(enum kind kind, SymData addr_written)
    : kind(kind), arg(std::move(addr_written.get_ref())),
      _written(std::move(addr_written.get_shared_block())) {};
  SymEv(enum kind kind, SymData addr_written, SymData::block_type expected)
    : kind(kind), arg(std::move(addr_written.get_ref())),
      _expected(std::move(expected)),
      _written(std::move(addr_written.get_shared_block())) {};
};

inline std::ostream &operator<<(std::ostream &os, const SymEv &e){
  return os << e.to_string();
}

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const SymEv &e){
  return os << e.to_string();
}

#endif
