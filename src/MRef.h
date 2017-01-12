/* Copyright (C) 2014-2017 Carl Leonardsson
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

#ifndef __MREF_H__
#define __MREF_H__

#include <cassert>
#include <iterator>

class ConstMRef;

/* An MRef object identifies a continuous non-empty sequence of bytes
 * in memory.
 */
class MRef {
public:
  /* The address of the first byte in the sequence. */
  void *ref;
  /* The number of bytes in the sequence. */
  int size;
  MRef(void *r, int sz) : ref(r), size(sz) { assert(0 < size); };
  /* Returns true iff all bytes in this MRef are also in mr. */
  bool subsetof(const MRef &mr) const;
  bool subsetof(const ConstMRef &mr) const;
  /* Returns true iff some bytes are both in this MRef and in mr. */
  bool overlaps(const MRef &mr) const;
  bool overlaps(const ConstMRef &mr) const;
  /* Returns true iff the byte pointed to by bptr is in this
   * sequence.
   */
  bool includes(void const *bptr) const {
    return ref <= bptr &&
      bptr < (void const*)((uint8_t const*)ref + size);
  };
  bool operator==(const MRef &ml) const { return ref == ml.ref && size == ml.size; };
  bool operator!=(const MRef &ml) const { return ref != ml.ref || size != ml.size; };
  bool operator<(const MRef &ml) const { return ref < ml.ref || (ref == ml.ref && size < ml.size); };
  bool operator<=(const MRef &ml) const { return ref <= ml.ref && (ref != ml.ref || size <= ml.size); };
  bool operator>(const MRef &ml) const { return ref > ml.ref || (ref == ml.ref && size > ml.size); };
  bool operator>=(const MRef &ml) const { return ref >=ml.ref && (ref != ml.ref || size >= ml.size); };

  class const_iterator{
  public:
    typedef std::bidirectional_iterator_tag iterator_category;
    typedef void const * value_type;
    typedef void const *& reference;
    typedef void const ** pointer;
    typedef unsigned difference_type;

    bool operator<(const const_iterator &it) const { return ptr < it.ptr; };
    bool operator>(const const_iterator &it) const { return ptr > it.ptr; };
    bool operator==(const const_iterator &it) const { return ptr == it.ptr; };
    bool operator!=(const const_iterator &it) const { return ptr != it.ptr; };
    bool operator<=(const const_iterator &it) const { return ptr <= it.ptr; };
    bool operator>=(const const_iterator &it) const { return ptr >= it.ptr; };
    const_iterator operator++(int) {
      const_iterator it = *this;
      ++ptr;
      return it;
    };
    const_iterator &operator++() {
      ++ptr;
      return *this;
    };
    const_iterator operator--(int) {
      const_iterator it = *this;
      --ptr;
      return it;
    };
    const_iterator &operator--() {
      --ptr;
      return *this;
    };
    void const *operator*() const{ return ptr; };
  private:
    const_iterator(void const *ref, int sz, int i) {
      assert(0 <= sz);
      assert(0 <= i);
      assert(i <= sz);
      ptr = (uint8_t const *)ref + i;
    };
    friend class MRef;
    friend class ConstMRef;
    uint8_t const *ptr;
  };

  const_iterator begin() const { return const_iterator(ref,size,0); };
  const_iterator end() const { return const_iterator(ref,size,size); };
};

/* A ConstMRef object is as an MRef object, but refers to const
 * memory. */
class ConstMRef {
public:
  /* The address of the first byte in the sequence. */
  const void *ref;
  /* The number of bytes in the sequence. */
  int size;
  ConstMRef(const void *r, int sz) : ref(r), size(sz) { assert(0 < size); };
  ConstMRef(const MRef &R) : ref(R.ref), size(R.size) { assert(0 < size); };
  /* Returns true iff all bytes in this ConstMRef are also in mr. */
  bool subsetof(const MRef &mr) const;
  bool subsetof(const ConstMRef &mr) const;
  /* Returns true iff some bytes are both in this ConstMRef and in mr. */
  bool overlaps(const MRef &mr) const;
  bool overlaps(const ConstMRef &mr) const;
  /* Returns true iff the byte pointed to by bptr is in this
   * sequence.
   */
  bool includes(void const *bptr) const {
    return ref <= bptr &&
      bptr < (void const*)((uint8_t const*)ref + size);
  };
  bool operator==(const ConstMRef &ml) const { return ref == ml.ref && size == ml.size; };
  bool operator!=(const ConstMRef &ml) const { return ref != ml.ref || size != ml.size; };
  bool operator<(const ConstMRef &ml) const { return ref < ml.ref || (ref == ml.ref && size < ml.size); };
  bool operator<=(const ConstMRef &ml) const { return ref <= ml.ref && (ref != ml.ref || size <= ml.size); };
  bool operator>(const ConstMRef &ml) const { return ref > ml.ref || (ref == ml.ref && size > ml.size); };
  bool operator>=(const ConstMRef &ml) const { return ref >=ml.ref && (ref != ml.ref || size >= ml.size); };

  MRef::const_iterator begin() const { return MRef::const_iterator(ref,size,0); };
  MRef::const_iterator end() const { return MRef::const_iterator(ref,size,size); };
};

/* An MBlock object is an MRef ref together with a chunk of memory,
 * block, associated with the memory location described by ref. The
 * chunk in block is assumed to be a local version of the memory
 * location described by ref, such as an entry in a cache or a store
 * buffer.
 */
class MBlock{
private:
  MRef ref;
  /* Block points to at least ref.size bytes of memory. */
  void *block;
  /* Pointer counter for block and itself. 0 if the block is not owned. */
  int *ptr_counter;
public:
  /* Create an MBlock with reference ref, and a fresh block of memory
   * of alloc_size bytes.
   *
   * Pre: ref.size <= alloc_size
   */
  MBlock(const MRef &ref, int alloc_size);
  /* Create an MBlock with reference ref, using the actual memory
   * location referenced by ref as its memory block. Does not take
   * ownership.
   */
  MBlock(const MRef &ref);
  MBlock(const MBlock &B);
  MBlock &operator=(const MBlock &B);
  ~MBlock();
  const MRef &get_ref() const { return ref; };
  void *get_block() const { return block; };
};

#endif

