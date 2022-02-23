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

#include "RMWAction.h"
#include <cstring>
#include <climits>
/* One of these contains llvm::sys::IsBigEndianHost */
#include <llvm/Support/Host.h>          /* On llvm < 11 */
#include <llvm/Support/SwapByteOrder.h> /* On llvm >= 11 */

namespace {
  int apcmp(const void *lhsv, const void *rhsv, std::size_t count) {
    if (llvm::sys::IsBigEndianHost) {
      return std::memcmp(lhsv, rhsv, count);
    } else {
      const uint8_t *lhs = static_cast<const uint8_t*>(lhsv);
      const uint8_t *rhs = static_cast<const uint8_t*>(rhsv);
      while(count--) {
        if (lhs[count] > rhs[count]) return 1;
        else if (lhs[count] < rhs[count]) return -1;
      }
      return 0;
    }
  }

  int sapcmp(const void *lhs, const void *rhs, std::size_t count) {
    if (!count) return 0;
    std::size_t big_byte_index;
    if (llvm::sys::IsBigEndianHost) {
      big_byte_index = 0;
    } else {
      big_byte_index = count-1;
    }
    bool lsign = ((const char*)lhs)[big_byte_index] >> (CHAR_BIT-1);
    bool rsign = ((const char*)rhs)[big_byte_index] >> (CHAR_BIT-1);
    if (lsign != rsign) return lsign ? -1 : 1;
    return apcmp(lhs, rhs, count);
  }
}

const char *RmwAction::name(Kind kind) {
  const static char *names[] = {
    nullptr, "XCHG", "ADD", "SUB", "AND", "NAND", "OR", "XOR", "MAX", "MIN",
    "UMAX", "UMIN",
  };
  return names[kind];
}

void RmwAction::apply_to(SymData &dst, const SymData &data) {
  assert(dst.get_ref() == data.get_ref());
  assert(dst.get_shared_block().unique());
  apply_to(dst.get_block(), dst.get_ref().size, data.get_block());
}

void RmwAction::apply_to(void *dst, std::size_t size, void *data) {
  uint8_t *outp = static_cast<uint8_t*>(dst);
  const uint8_t *inp, *argp;
  inp = static_cast<uint8_t*>(data);
  argp = static_cast<uint8_t*>(operand.get());

  switch (kind) {
  case ADD: case SUB: {
    int multiplier =    llvm::sys::IsBigEndianHost ? -1 : 1;
    std::size_t shift = llvm::sys::IsBigEndianHost ? size - 1 : 0;
    outp += shift;
    inp += shift;
    argp += shift;
    int8_t carry = 0;
    for (size_t i = 0; i < size; ++i) {
      uint16_t sum;
      if (kind == ADD) sum = inp[i*multiplier] + argp[i*multiplier] + carry;
      else sum = inp[i*multiplier] - argp[i*multiplier] + carry;
      outp[i*multiplier] = sum;
      carry = sum >> 8;
    }
  } break;
  case AND: case NAND: case OR: case XOR:  {
    for (size_t i = 0; i < size; ++i) {
      uint8_t res, lhs, rhs;
      lhs = inp[i];
      rhs = argp[i];
      switch(kind) {
      case AND:  res = lhs & rhs; break;
      case NAND: res = ~(lhs & rhs); break;
      case OR:   res = lhs | rhs; break;
      case XOR:  res = lhs ^ rhs; break;
      default:abort();
      }
      outp[i] = res;
    }
  } break;
  case MAX: case MIN: case UMAX: case UMIN:  {
    bool is_signed = kind == MAX || kind == MIN;
    int out = (is_signed ? sapcmp : apcmp)(data, operand.get(), size);
    bool take_rhs = out > 0;
    if (kind == MAX || kind == UMAX) take_rhs = !take_rhs;
    const uint8_t *res = take_rhs ? argp : inp;
    memcpy(outp, res, size);
  } break;
  case XCHG: {
    memcpy(outp, argp, size);
  } break;
  default: abort();
  }
}
