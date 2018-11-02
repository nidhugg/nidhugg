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

#include "SymAddr.h"

std::string SymMBlock::to_string(std::function<std::string(int)> pid_str) const {
  std::string ret;
  if (is_null()) return "Null()";
  if (is_global()) {
    ret = "Global(";
  } else {
    if (is_stack()) {
      ret = "Stack(";
    } else {
      assert(is_heap());
      ret = "Heap(";
    }
    ret += pid_str(pid) + ",";
  }
  ret += std::to_string(get_no()) + ")";
  return ret;
}

std::string SymAddr::to_string(std::function<std::string(int)> pid_str) const {
  if (offset == 0) return block.to_string(pid_str);
  return "(" + block.to_string(pid_str) + "+" + std::to_string(offset) + ")";
}

std::string SymAddrSize::to_string(std::function<std::string(int)> pid_str) const {
  return addr.to_string(pid_str) + "(" + std::to_string(size) + ")";
}

bool SymAddrSize::subsetof(const SymAddrSize &other) const {
  if (addr.block != other.addr.block) return false;
  uintptr_t this_low = addr.offset;
  uintptr_t this_high = this_low + size;
  uintptr_t other_low = other.addr.offset;
  uintptr_t other_high = other_low + other.size;
  return this_low >= other_low && this_high <= other_high;
}

bool SymAddrSize::overlaps(const SymAddrSize &other) const {
  if (addr.block != other.addr.block) return false;
  uintptr_t this_low = addr.offset;
  uintptr_t this_high = this_low + size;
  uintptr_t other_low = other.addr.offset;
  uintptr_t other_high = other_low + other.size;
  return other_low < this_high && this_low < other_high;
}

SymData::block_type SymData::alloc_block(int alloc_size){
  return block_type(new uint8_t[alloc_size], std::default_delete<uint8_t[]>());
}

SymData::SymData(const SymAddrSize ref, int alloc_size)
  : ref(ref), block(alloc_block(alloc_size)) {}

SymData::SymData(SymAddrSize ref, block_type block)
  : ref(ref), block(std::move(block)) {}
