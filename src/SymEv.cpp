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

void SymEv::set(SymEv other) {
  // if (kind != EMPTY) {
    if(kind != other.kind
       && !(kind == STORE && other.kind == UNOBS_STORE)) {
      llvm::dbgs() << "Merging incompatible events " << *this << " and "
                   << other << "\n";
      assert(false);
    }
#ifndef NDEBUG
    switch(kind) {
    case LOAD: case STORE:
    case M_INIT: case M_LOCK: case M_UNLOCK: case M_DELETE:
    case C_INIT: case C_SIGNAL: case C_BRDCST: case C_DELETE:
    case C_WAIT: case C_AWAKE:
    case UNOBS_STORE:
      assert(arg.addr == other.arg.addr);
      break;
    case SPAWN: case JOIN:
    case NONDET:
      assert(arg.num == other.arg.num);
      break;
    case FULLMEM:
      break;
    default:
      assert(false && "Unknown kind");
    }
#endif
  // }
  *this = other;
}

std::string SymEv::to_string(std::function<std::string(int)> pid_str) const {
    switch(kind) {
    // case EMPTY:    return "Empty()";
    case NONDET:   return "Nondet(" + std::to_string(arg.num) + ")";

    case LOAD:     return "Load("    + arg.addr.to_string(pid_str) + ")";
    case STORE:    return "Store("   + arg.addr.to_string(pid_str) + ")";
    case FULLMEM:  return "Fullmem()";

    case M_INIT:   return "MInit("   + arg.addr.to_string(pid_str) + ")";
    case M_LOCK:   return "MLock("   + arg.addr.to_string(pid_str) + ")";
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

    case UNOBS_STORE: return "UnobsStore(" + arg.addr.to_string(pid_str) + ")";

    default:
      abort();
    }
}

bool SymEv::has_addr() const {
  switch(kind) {
  case LOAD: case STORE:
  case M_INIT: case M_LOCK: case M_UNLOCK: case M_DELETE:
  case C_INIT: case C_SIGNAL: case C_BRDCST: case C_DELETE:
  case C_WAIT: case C_AWAKE:
  case UNOBS_STORE:
    return true;
  case FULLMEM: case NONDET:
  case SPAWN: case JOIN:
    return false;
  default:
    abort();
  }
}

bool SymEv::has_num() const {
  switch(kind) {
  case SPAWN: case JOIN:
  case NONDET:
    return true;
  case C_WAIT: case C_AWAKE:
  case FULLMEM:
  case LOAD: case STORE:
  case M_INIT: case M_LOCK: case M_UNLOCK: case M_DELETE:
  case C_INIT: case C_SIGNAL: case C_BRDCST: case C_DELETE:
  case UNOBS_STORE:
    return false;
  default:
    abort();
  }
}

bool SymEv::operator==(const SymEv &s) const{
  if (kind != s.kind) return false;
  if (has_addr() && addr() != s.addr()) return false;
  if (has_num() && num() != s.num()) return false;
  return true;
}
