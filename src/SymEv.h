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

#include <llvm/Support/raw_ostream.h>

#include "MRef.h"

/* For now, addresses are concrete (and thus not stable between executions) */
typedef ConstMRef SymAddr;

/* Symbolic representation of an event */
struct SymEv {
  public:
  enum kind {
    // EMPTY,

    LOAD,
    STORE,
    FULLMEM, /* Observe & clobber everything */

    M_INIT,
    M_LOCK,
    M_UNLOCK,
    M_DELETE,

    C_INIT,
    C_SIGNAL,
    C_BRDCST,
    C_WAIT,
    C_DELETE,

    NONDET,
  } kind;
  union arg {
  public:
    SymAddr addr;
    struct apair { SymAddr first; SymAddr second; } apair;

    arg() {}
    arg(SymAddr addr) : addr(addr) {}
    arg(struct apair apair) : apair(apair) {}
    // ~arg_union() {}
  } arg;

  SymEv() = delete;
  // SymEv() : kind(EMPTY) {};
  // static SymEv Empty() { return {EMPTY, {}}; }
  static SymEv Load(SymAddr addr) { return {LOAD, addr}; }
  static SymEv Store(SymAddr addr) { return {STORE, addr}; }
  static SymEv Fullmem() { return {FULLMEM, {}}; }

  static SymEv MInit(SymAddr addr) { return {M_INIT, addr}; }
  static SymEv MLock(SymAddr addr) { return {M_LOCK, addr}; }
  static SymEv MUnlock(SymAddr addr) { return {M_UNLOCK, addr}; }
  static SymEv MDelete(SymAddr addr) { return {M_DELETE, addr}; }

  static SymEv CInit(SymAddr addr) { return {C_INIT, addr}; }
  static SymEv CSignal(SymAddr addr) { return {C_SIGNAL, addr}; }
  static SymEv CBrdcst(SymAddr addr) { return {C_SIGNAL, addr}; }
  static SymEv CWait(SymAddr cond, SymAddr mutex) {
    return {C_WAIT, {{cond, mutex}}};
  }
  static SymEv CDelete(SymAddr addr) { return {C_DELETE, addr}; }

  static SymEv Nondet() { return {NONDET, {}}; }

  void set(SymEv other);
  std::string to_string() const;

  bool has_addr() const;
  bool has_apair() const;
  /* TODO: kind assertions */
  const SymAddr &addr()   const { assert(has_addr()); return arg.addr; }
  const SymAddr &apair1() const { assert(has_apair()); return arg.apair.first; }
  const SymAddr &apair2() const { assert(has_apair()); return arg.apair.second; }

private:
  SymEv(enum kind kind, union arg arg) : kind(kind), arg(arg) {};
};

inline std::ostream &operator<<(std::ostream &os, const SymEv &e){
  return os << e.to_string();
}

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const SymEv &e){
  return os << e.to_string();
}

#endif
