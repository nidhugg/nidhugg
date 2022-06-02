/* Copyright (C) 2019 Magnus Lång
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

#include <config.h>

#ifndef __AWAIT_COND_H__
#define __AWAIT_COND_H__

#include "SymAddr.h"

#include <utility>

struct AwaitCond {
  SymData::block_type operand;
  /* An Op is a bitfield, with the following meanings for bits 3..0, msb
   * first:
   *   Signed  - If the comparison is of signed values
   *   Less    - If the comparison is true when lhs is less than rhs
   *   Equal   - If the comparison is true when lhs is equal to rhs
   *   Greater - If the comparison is true when lhs is greater than rhs
   */
  enum Op : uint8_t {
    None,
    UGT = 0b0001,
    EQ  = 0b0010,
    UGE = 0b0011,
    ULT = 0b0100,
    NE  = 0b0101,
    ULE = 0b0110,
    SGT = 0b1001,
    SGE = 0b1011,
    SLT = 0b1100,
    SLE = 0b1110,
  } op = None;

  AwaitCond() {}
  AwaitCond(Op op, SymData::block_type operand)
    : operand(std::move(operand)), op(op) { assert(op != None); }

  static const char *name(Op op);

  bool satisfied_by(const SymData &data) const {
    return satisfied_by(data.get_block(), data.get_ref().size);
  }
  bool satisfied_by(const void *data, std::size_t size) const;
};

#endif
