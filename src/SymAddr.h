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

#include <config.h>

#ifndef __SYM_ADDR_H__
#define __SYM_ADDR_H__

#include <cstdint>
#include <cassert>
#include <algorithm>
#include <string>
#include <functional>
#include <memory>

struct SymMBlock {
  static SymMBlock Null() {
    return SymMBlock();
  }
  static SymMBlock Global(unsigned no) {
    assert(no <= INT16_MAX);
    return SymMBlock(UINT16_MAX, no);
  }
  static SymMBlock Stack(int pid, unsigned no) {
    assert(no <= INT16_MAX);
    return SymMBlock(pid, -1-no);
  }
  static SymMBlock Heap(int pid, unsigned no) {
    assert(no <= INT16_MAX);
    return SymMBlock(pid, no);
  }

  bool operator==(const SymMBlock &o) const { return pid == o.pid && alloc == o.alloc; };
  bool operator!=(const SymMBlock &o) const { return !(*this == o); };
  bool operator<=(const SymMBlock &o) const {
    return pid < o.pid || (pid == o.pid && alloc <= o.alloc);
  }
  bool operator<(const SymMBlock &o)  const { return *this <= o && *this != o; }
  bool operator>=(const SymMBlock &o) const { return !(*this < o); }
  bool operator>(const SymMBlock &o)  const { return !(*this <= o); }

  bool is_null() const { return pid == UINT16_MAX && alloc == -1; }
  bool is_global() const { return pid == UINT16_MAX; }
  bool is_stack() const { return !is_global() && alloc < 0; }
  bool is_heap() const { return !is_global() && alloc >= 0; }

  unsigned get_no() const {
    if (alloc < 0) return -1-alloc;
    else return alloc;
  }

  std::string to_string(std::function<std::string(int)> pid_str
                        = (std::string(&)(int))std::to_string) const;

private:
  SymMBlock() : pid(UINT16_MAX), alloc(-1) {}
  SymMBlock(int pid, int alloc) : pid(pid), alloc(alloc) {
    assert(0 <= pid && pid <= UINT16_MAX);
    assert(INT16_MIN <= alloc && alloc <= INT16_MAX);
  }
  friend struct std::hash<struct SymAddr>;
  uint16_t pid;
  int16_t alloc;
};

struct SymAddr {
public:
  SymAddr() : block(SymMBlock::Null()), offset(0) {}
  SymAddr(std::nullptr_t) : SymAddr() {}
  SymAddr(SymMBlock block, uintptr_t offset)
    : block(std::move(block)), offset(offset) {
    assert(!block.is_null());
    assert(offset <= UINT32_MAX);
  }
  SymMBlock block;
  uint32_t offset;

  bool operator==(const SymAddr &o) const { return block == o.block && offset == o.offset; };
  bool operator!=(const SymAddr &o) const { return !(*this == o); };
  bool operator<=(const SymAddr &o) const {
    return block < o.block || (block == o.block && offset <= o.offset);
  }
  bool operator<(const SymAddr &o)  const { return *this <= o && *this != o; }
  bool operator>=(const SymAddr &o) const { return !(*this < o); }
  bool operator>(const SymAddr &o)  const { return !(*this <= o); }
  SymAddr operator++(int) {
    SymAddr copy = *this;
    ++offset;
    return copy;
  };
  SymAddr &operator++() {
    ++offset;
    return *this;
  };
  SymAddr operator--(int) {
    SymAddr copy = *this;
    --offset;
    return copy;
  };
  SymAddr &operator--() {
    --offset;
    return *this;
  };
  SymAddr operator+(intptr_t addend) const {
    SymAddr copy = *this;
    copy.offset += addend;
    return copy;
  };
  SymAddr operator-(intptr_t addend) const {
    SymAddr copy = *this;
    copy.offset -= addend;
    return copy;
  };
  intptr_t operator-(const SymAddr &other) const {
    assert(block == other.block);
    return offset - other.offset;
  };

  std::string to_string(std::function<std::string(int)> pid_str
                        = (std::string(&)(int))std::to_string) const;
};

namespace std {
template<> struct hash<SymAddr>{
public:
  hash() {}
  std::size_t operator()(const SymAddr &a) const {
    /* Intentionally laid out so that this becomes a single 64-bit load. */
    return std::size_t(a.block.pid)
      | std::size_t(uint16_t(a.block.alloc)) << 16
      | std::size_t(a.offset) << 32;
  }
};
}

struct SymAddrSize {
public:
  SymAddrSize(SymAddr addr, uintptr_t size)
    : addr(std::move(addr)), size(size) {
    assert(size <= UINT16_MAX);
  }
  SymAddr addr;
  uint16_t size;

  /* Returns true iff all bytes in this SymAddrSize are also in sa. */
  bool subsetof(const SymAddrSize &sa) const;
  /* Returns true iff some bytes are both in this SymAddrSize and in sa. */
  bool overlaps(const SymAddrSize &sa) const;

  bool includes(const SymAddr &sa) const {
    return addr.block == sa.block && addr.offset <= sa.offset
      && (addr.offset + size) > sa.offset;
  }

  bool operator==(const SymAddrSize &o) const { return addr == o.addr && size == o.size; };
  bool operator!=(const SymAddrSize &o) const { return !(*this == o); };
  bool operator<(const SymAddrSize &o) const {
    return addr < o.addr || (addr == o.addr && size < o.size);
  }

  std::string to_string(std::function<std::string(int)> pid_str
                        = (std::string(&)(int))std::to_string) const;

  class iterator{
  public:
    typedef std::bidirectional_iterator_tag iterator_category;
    typedef SymAddr  value_type;
    typedef SymAddr& reference;
    typedef SymAddr* pointer;
    typedef unsigned difference_type;
    bool operator<(const iterator &it) const { return addr < it.addr; };
    bool operator>(const iterator &it) const { return addr > it.addr; };
    bool operator==(const iterator &it) const { return addr == it.addr; };
    bool operator!=(const iterator &it) const { return addr != it.addr; };
    bool operator<=(const iterator &it) const { return addr <= it.addr; };
    bool operator>=(const iterator &it) const { return addr >= it.addr; };
    iterator operator++(int) {
      iterator it = *this;
      ++addr;
      return it;
    };
    iterator &operator++() {
      ++addr;
      return *this;
    };
    iterator operator--(int) {
      iterator it = *this;
      --addr;
      return it;
    };
    iterator &operator--() {
      --addr;
      return *this;
    };
    SymAddr operator*() const { return addr; };

  private:
    iterator(SymAddr addr) : addr(addr) {}
    friend struct SymAddrSize;
    SymAddr addr;
  };

  iterator begin() const { return iterator(addr); };
  iterator end() const { return iterator(addr + size); };
};

/* An SymData object is an SymAddrSize ref together with a chunk of memory,
 * block, associated with the memory location described by ref. The
 * chunk in block is assumed to be a local version of the memory
 * location described by ref, such as an entry in a cache or a store
 * buffer.
 */
class SymData {
public:
  typedef std::shared_ptr<uint8_t> block_type;
  static block_type alloc_block(int alloc_size);
private:
  SymAddrSize ref;
  block_type block;
public:
  /* Create an SymData with reference ref, and a fresh block of memory
   * of alloc_size bytes.
   *
   * Pre: ref.size <= alloc_size
   */
  SymData(SymAddrSize ref, int alloc_size);
  /* Create an SymData with reference ref, using block as the block of memory.
   *
   * Pre: The allocated size of block must be at least ref.size elements.
   */
  SymData(SymAddrSize ref, block_type block);
  uint8_t &operator [](SymAddr addr) const {
    assert(ref.includes(addr));
    return block.get()[addr-ref.addr];
  }
  const SymAddrSize &get_ref() const { return ref; };
  void *get_block() const { return block.get(); };
  block_type &get_shared_block() { return block; };
};

#endif
