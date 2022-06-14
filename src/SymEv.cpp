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

#include "SymEv.h"
#include "Debug.h"

#include <sstream>

bool SymEv::is_compatible_with(SymEv other) const {
  if (kind != other.kind
      && !(kind == STORE && other.kind == UNOBS_STORE)
      && !(kind == M_TRYLOCK && other.kind == M_TRYLOCK_FAIL)
      && !(kind == M_TRYLOCK_FAIL && other.kind == M_TRYLOCK))
    return false;
  if (kind == RMW) {
    if (arg2.rmw_kind != other.arg2.rmw_kind) return false;
    if (_expected) { /* Optimal-DPOR erases data in the wakeup tree */
      assert(other._expected);
      // We need stable addresses
      // if (memcmp(_expected.get(), other._expected.get(), arg.addr.size) != 0)
      //   return false;
    }
  }
  switch(kind) {
  case LOAD_AWAIT: case XCHG_AWAIT:
    if (arg2.await_op != other.arg2.await_op) return false;
    if (_expected) {
      assert(other._expected);
      // We need stable addresses
      // if (memcmp(_expected.get(), other._expected.get(), arg.addr.size) != 0)
      //   return false;
    }
    /* fallthrough */
  case LOAD:
  case M_INIT: case M_LOCK: case M_UNLOCK: case M_DELETE:
  case M_TRYLOCK: case M_TRYLOCK_FAIL:
  case C_INIT: case C_SIGNAL: case C_BRDCST: case C_DELETE:
  case C_WAIT: case C_AWAKE:
  case RMW: case CMPXHG: case CMPXHGFAIL:
  case STORE: case UNOBS_STORE:
    if (arg.addr != other.arg.addr) return false;
    break;
  case SPAWN: case JOIN:
  case NONDET:
    if (arg.num != other.arg.num) return false;
    break;
  case FULLMEM:
  case NONE:
    break;
  default:
    abort();
  }
  return true;
}

static std::string block_to_string(const SymData::block_type &blk, unsigned size) {
  if (!blk) return "-";
  int i = size-1;
  uint8_t *ptr = (uint8_t*)blk.get();
  std::stringstream res;
  res << "0x" << std::hex;
  for (; i > 0 && ptr[i] == 0; --i) {}
  res << (int)ptr[i--];
  /* No leading zeroes on first digit */
  res.width(2);
  res.fill('0');
  for ( ; i >= 0; --i) {
    res << (int)ptr[i];
  }
  return res.str();
}

std::string SymEv::to_string(std::function<std::string(int)> pid_str) const {
    switch(kind) {
    case NONE:     return "None()";
    case NONDET:   return "Nondet(" + std::to_string(arg.num) + ")";

    case LOAD:     return "Load("    + arg.addr.to_string(pid_str) + ")";
    case LOAD_AWAIT:
      return "LoadAwait(" + arg.addr.to_string(pid_str) + ", "
        + AwaitCond::name(arg2.await_op) + " "
        + block_to_string(_expected, arg.addr.size) + ")";
    case STORE:    return "Store("   + arg.addr.to_string(pid_str)
        + "," + block_to_string(_written, arg.addr.size) + ")";
    case FULLMEM:  return "Fullmem()";

    case M_INIT:   return "MInit("   + arg.addr.to_string(pid_str) + ")";
    case M_LOCK:   return "MLock("   + arg.addr.to_string(pid_str) + ")";
    case M_TRYLOCK: return "MTryLock(" + arg.addr.to_string(pid_str) + ")";
    case M_TRYLOCK_FAIL: return "MTryLockFail(" + arg.addr.to_string(pid_str)
        + ")";
    case M_UNLOCK: return "MUnlock(" + arg.addr.to_string(pid_str) + ")";
    case M_DELETE: return "MDelete(" + arg.addr.to_string(pid_str) + ")";

    case C_INIT:   return "CInit("   + arg.addr.to_string(pid_str) + ")";
    case C_SIGNAL: return "CSignal(" + arg.addr.to_string(pid_str) + ")";
    case C_BRDCST: return "CBrdcst(" + arg.addr.to_string(pid_str) + ")";
    case C_WAIT:   return "CWait("   + arg.addr.to_string(pid_str) + ")";
    case C_AWAKE:  return "CAwake("  + arg.addr.to_string(pid_str) + ")";
    case C_DELETE: return "CDelete(" + arg.addr.to_string(pid_str) + ")";

    case SPAWN: return "Spawn(" + pid_str(arg.num) + ")";
    case JOIN:  return "Join("  + pid_str(arg.num) + ")";

    case UNOBS_STORE: return "UnobsStore(" + arg.addr.to_string(pid_str)
        + "," + block_to_string(_written, arg.addr.size) + ")";

    case RMW: return "Rmw(" + arg.addr.to_string(pid_str)
        + "," + block_to_string(_written, arg.addr.size)
        + "," + RmwAction::name(arg2.rmw_kind)
        + " " + block_to_string(_expected, arg.addr.size) + ")";
    case XCHG_AWAIT: return "XchgAwait(" + arg.addr.to_string(pid_str)
        + "," + block_to_string(_written, arg.addr.size) + ", "
        + AwaitCond::name(arg2.await_op) + " "
        + block_to_string(_expected, arg.addr.size)+ ")";
    case CMPXHG: return "CmpXhg("
        + arg.addr.to_string(pid_str)
        + "," + block_to_string(_expected, arg.addr.size)
        + "," + block_to_string(_written, arg.addr.size) + ")";
    case CMPXHGFAIL: return "CmpXhgFail("
        + arg.addr.to_string(pid_str)
        + "," + block_to_string(_expected, arg.addr.size)
        + "," + block_to_string(_written, arg.addr.size) + ")";
    }
    abort();
}

bool SymEv::has_addr() const {
  switch(kind) {
  case LOAD: case LOAD_AWAIT: case STORE:
  case M_INIT: case M_LOCK: case M_UNLOCK: case M_DELETE:
  case M_TRYLOCK: case M_TRYLOCK_FAIL:
  case C_INIT: case C_SIGNAL: case C_BRDCST: case C_DELETE:
  case C_WAIT: case C_AWAKE:
  case UNOBS_STORE:
  case RMW: case XCHG_AWAIT: case CMPXHG: case CMPXHGFAIL:
    return true;
  case NONE:
  case FULLMEM: case NONDET:
  case SPAWN: case JOIN:
    return false;
  }
  abort();
}

bool SymEv::has_num() const {
  switch(kind) {
  case SPAWN: case JOIN:
  case NONDET:
    return true;
  case NONE:
  case C_WAIT: case C_AWAKE:
  case FULLMEM:
  case LOAD: case LOAD_AWAIT: case STORE:
  case M_INIT: case M_LOCK: case M_UNLOCK: case M_DELETE:
  case M_TRYLOCK: case M_TRYLOCK_FAIL:
  case C_INIT: case C_SIGNAL: case C_BRDCST: case C_DELETE:
  case UNOBS_STORE:
  case RMW: case XCHG_AWAIT: case CMPXHG: case CMPXHGFAIL:
    return false;
  }
  abort();
}

bool SymEv::has_data() const {
  switch(kind) {
  case STORE: case UNOBS_STORE:
  case RMW: case XCHG_AWAIT: case CMPXHG: case CMPXHGFAIL:
    return (bool)_written;
  case NONE:
  case SPAWN: case JOIN:
  case NONDET:
  case C_WAIT: case C_AWAKE:
  case FULLMEM:
  case LOAD: case LOAD_AWAIT:
  case M_INIT: case M_LOCK: case M_UNLOCK: case M_DELETE:
  case M_TRYLOCK: case M_TRYLOCK_FAIL:
  case C_INIT: case C_SIGNAL: case C_BRDCST: case C_DELETE:
    return false;
  }
  abort();
}

bool SymEv::has_expected() const {
  switch(kind) {
  case CMPXHG: case CMPXHGFAIL:
    return (bool)_written;
  case NONE:
  case SPAWN: case JOIN:
  case NONDET:
  case C_WAIT: case C_AWAKE:
  case FULLMEM:
  case LOAD: case LOAD_AWAIT:
  case STORE: case UNOBS_STORE:
  case M_INIT: case M_LOCK: case M_UNLOCK: case M_DELETE:
  case M_TRYLOCK: case M_TRYLOCK_FAIL:
  case C_INIT: case C_SIGNAL: case C_BRDCST: case C_DELETE:
  case RMW: case XCHG_AWAIT:
    return false;
  }
  abort();
}

bool SymEv::operator==(const SymEv &s) const{
  if (kind != s.kind) return false;
  if (has_addr() && addr() != s.addr()) return false;
  if (has_num() && num() != s.num()) return false;
  return true;
}

void SymEv::purge_data() {
  _written.reset();
  _expected.reset();
}

void SymEv::set_observed(bool observed) {
  if (observed) {
    if (kind == UNOBS_STORE) {
      kind = STORE;
    } else {
      assert(kind == STORE);
    }
  } else {
    if (kind == STORE) {
      kind = UNOBS_STORE;
    } else {
      assert(kind == UNOBS_STORE);
    }
  }
}
