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

SymData::SymData(const SymAddrSize ref, int alloc_size)
  : ref(ref) {
  block = malloc(alloc_size);
  ptr_counter = new int(1);
  *ptr_counter = 1;
}

SymData::SymData(const SymData &B)
  : ref(B.ref), block(B.block), ptr_counter(B.ptr_counter) {
  if(ptr_counter) ++*ptr_counter;
}

SymData &SymData::operator=(const SymData &B) {
  if (this != &B) {
    this->~SymData();
    new (this) SymData(B);
  }
  return *this;
}

SymData::~SymData() {
  if(ptr_counter){
    --*ptr_counter;
    if(*ptr_counter == 0){
      free(block);
      delete ptr_counter;
    }
  }
}
