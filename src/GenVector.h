/* Copyright (C) 2020 Magnus Lång
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
#ifndef __GEN_VECTOR_H__
#define __GEN_VECTOR_H__

#ifndef NDEBUG
/* For reference-count */
#  include <atomic>
#endif
#include <algorithm>
#include <array>
#include <climits>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace gen {

  template <typename X>
  static constexpr unsigned vector_default_limb_size
  = std::max<std::size_t>(8, 128/sizeof(X));

  /* gen::vector is a vector implementation which offers efficient duplication
   * through copy-on-write structural sharing and also single-owner RAII memory
   * management (no reference counters). It is thread-safe and performant even
   * on large parallel computers.
   *
   * It offers these features by placing restrictions on its use. Whenever a
   * (non-empty) vector is duplicated (through the copy constructor or copy
   * assignment operator), it is required that the source vector is not modified
   * or destroyed for the lifetime of the copy.
   *
   * When compiling with !defined(NDEBUG), the vector will check that these
   * rules are followed (at a cost to performance).
   */
  template<typename T, std::size_t limb_size = vector_default_limb_size<T>>
  class vector {
  public:
    typedef std::size_t size_type;

    /* Empty */
    vector();
    /* Copy-on-write duplicate another vector. It must not be modified after this. */
    explicit vector(const vector &);
    /* Steal another vector. It will be empty after this. It is allowed to steal
     * a vector that has COW references, but then the new vector may not be
     * modified. */
    vector(vector &&);
    ~vector();
    /* Copy-on-write duplicate another vector. It must not be modified after this. */
    vector &operator=(const vector &other);
    /* Steal another vector. It will be empty after this. It is allowed to steal
     * a vector that has COW references, but then *this may not be modified
     * further. */
    vector &operator=(vector &&other);

    const T &operator[](size_type index) const;
    const T &at(size_type index) const;
    T &mut(size_type index);
    const T &back() const { return (*this)[_size-1]; }

    size_type size() const noexcept { return _size; }
    bool empty() const noexcept { return !_size; }

    void push_back(const T& val);
    void push_back(T&& val);
    template <typename... Args> T &emplace_back(Args&&... args);

  private:
#ifndef NDEBUG
    typedef std::atomic_uint_fast32_t _debug_refcount_type;
    _debug_refcount_type *_debug_parent = nullptr;
    std::unique_ptr<std::atomic_uint_fast32_t> _debug_refcount =
        std::make_unique<_debug_refcount_type>(0);
    void assert_writable() const;
#else
    inline constexpr void assert_writable() const {}
#endif
    typedef T limb_type[limb_size];
    static_assert(sizeof(T) * limb_size == sizeof(limb_type), "Packing assumption");
    typedef uint_fast8_t bitmap_uint;
    static_assert(alignof(limb_type) >= alignof(bitmap_uint), "Packing assumption");
    static constexpr std::size_t bits_per_uint = sizeof(bitmap_uint) * CHAR_BIT;
    size_type limb_count() const;
    static inline size_type next_capacity(size_type old_capacity) {
      return std::max(old_capacity+2, old_capacity + (old_capacity+1)/2);
    }
    static inline size_type bitmap_size(size_type capacity) {
      return (capacity + bits_per_uint - 1) / bits_per_uint;
    }

    static inline const T *deref(const limb_type *const *start, size_type index);
    inline bool root_is_cow_or_empty() const { return !last; }
    inline bool limb_is_cow(size_type limb_index) const;
    inline void uncow_if_needed(size_type limb_index);
    inline void uncow_root_if_needed();
    inline void uncow_root_if_needed_assume_nonempty();
    /* Does not copy bitmap, only elements */
    limb_type **alloc_root_copy_from
    (size_type capacity, size_type bitmap_size,
     const limb_type *const *source, size_type old_capacity);
    void uncow(size_type limb_index);
    void uncow_root();
    void copy_root(const limb_type **old_start, const limb_type **old_capacity);
    void grow();
    /* Adds a new element at the end without initializing it, returning its address. */
    T *push_back_internal();

    size_type _size;
    limb_type **start, **last;

    /* Overly permissive, how do I fix? */
    template<typename U, std::size_t S, typename Fn>
    friend void for_each(const vector<U,S> &v, Fn&& f);
    template<typename U, std::size_t S, typename P>
    friend bool all_of(const vector<U,S> &v, P&& f);

  public:
    class const_iterator {
      const limb_type *const *start;
      size_type index;
      const_iterator(const limb_type *const *start_init, size_type index)
        : start(start_init), index(index) {}
      friend class vector;
    public:
      typedef const T value_type, *pointer, &reference;
      typedef size_type difference_type;
      typedef std::bidirectional_iterator_tag iterator_category;
      const_iterator &operator++() { ++index; return *this; }
      const_iterator &operator--() { --index; return *this; }
      const T &operator*() { return *vector::deref(start, index); }
      bool operator!=(const const_iterator &other) { return !((*this) == other); }
      bool operator==(const const_iterator &other) { return index == other.index; }
    };
    const_iterator begin() const { return {start, 0}; }
    const_iterator end() const { return {start, _size}; }
  };

  template<typename T, std::size_t limb_size, typename Fn>
  void for_each(const vector<T,limb_size> &v, Fn&& f);

  template<typename T, std::size_t limb_size, typename Fn>
  bool all_of(const vector<T,limb_size> &v, Fn&& f);

}  // namespace gen

/* Start implementation */

#include <cassert>
#ifndef NDEBUG
#  define IFDEBUG(...) __VA_ARGS__
#else
#  define IFDEBUG(...)
#endif

namespace gen {
  template<typename T, std::size_t limb_size>
  vector<T,limb_size>::vector()
    : IFDEBUG(_debug_parent(nullptr),)
      _size(0), start(nullptr), last(nullptr) {}
  template<typename T, std::size_t limb_size>
  vector<T,limb_size>::vector(const vector &other) {
    _size = other._size;
    start = other.start;
    /* Encodes that start is a cow pointer */
    last = nullptr;
#ifndef NDEBUG
    if (start) {
      _debug_parent = other._debug_refcount.get();
      (*other._debug_refcount)++;
    } else {
      /* When copying an empty vector, we become a non-cow empty vector */
      _debug_parent = nullptr;
    }
#endif
  }
  template<typename T, std::size_t limb_size>
  vector<T,limb_size>::vector(vector &&other) {
#ifndef NDEBUG
    _debug_parent = std::exchange(other._debug_parent, nullptr);
    std::swap(_debug_refcount, other._debug_refcount);
#endif
    _size = std::exchange(other._size, 0);
    start = std::exchange(other.start, nullptr);
    last = std::exchange(other.last, nullptr);
  }
  template<typename T, std::size_t limb_size>
  auto vector<T,limb_size>::operator=(const vector &other) -> vector & {
    this->~vector();
    new(this) vector(other);
    return *this;
  }
  template<typename T, std::size_t limb_size>
  auto vector<T,limb_size>::operator=(vector &&other) -> vector & {
    this->~vector();
    new(this) vector(std::move(other));
    return *this;
  }
  template<typename T, std::size_t limb_size>
  vector<T,limb_size>::~vector() {
    assert_writable();
#ifndef NDEBUG
    if (_debug_parent) (*_debug_parent)--;
#endif
    if (last) {
      const size_type full_limb_count = size() / limb_size;
      for (size_type i = 0; i < full_limb_count; ++i) {
        if (!limb_is_cow(i)) {
          for (size_type j = 0; j < limb_size; ++j) {
            (*start[i])[j].~T();
          }
          operator delete(start[i]);
        }
      }
      const size_type elements_in_last_limb = size() % limb_size;
      if (elements_in_last_limb != 0 && !limb_is_cow(full_limb_count)) {
        for (size_type i = 0; i < elements_in_last_limb; ++i) {
          (*start[full_limb_count])[i].~T();
        }
        operator delete(start[full_limb_count]);
      }
      operator delete(start);
    }
  }
#ifndef NDEBUG
  template<typename T, std::size_t limb_size>
  void vector<T,limb_size>::assert_writable() const {
    assert((*_debug_refcount) == 0 && "Writable");
  }
#endif
  template<typename T, std::size_t limb_size>
  bool vector<T,limb_size>::limb_is_cow(size_type limb_index) const {
    const bitmap_uint *bitmap = reinterpret_cast<const bitmap_uint*>(last);
    return (bitmap[limb_index / bits_per_uint] >> (limb_index % bits_per_uint)) & 1;
  }
  template<typename T, std::size_t limb_size>
  auto vector<T,limb_size>::limb_count() const -> size_type {
    return (_size + limb_size - 1) / limb_size;
  }
  template<typename T, std::size_t limb_size>
  void vector<T,limb_size>::uncow_if_needed(size_type limb_index) {
    assert(last);
    if (limb_is_cow(limb_index)) uncow(limb_index);
  }
  template<typename T, std::size_t limb_size>
  void vector<T,limb_size>::uncow(size_type limb_index) {
    assert(limb_is_cow(limb_index));
    const size_type limb_valid_elements
        = std::min(size() - limb_index*limb_size, limb_size);
    assert(limb_valid_elements > 0);
    const limb_type *old_limb = start[limb_index];
    std::unique_ptr<limb_type> new_limb
        (static_cast<limb_type*>(operator new (sizeof(limb_type))));
    for (size_type i = 0; i < limb_valid_elements; ++i) {
      new (std::addressof((*new_limb)[i])) T((*old_limb)[i]);
    }
    start[limb_index] = new_limb.release();
    /* Clear COW bit */
    bitmap_uint *bitmap = reinterpret_cast<bitmap_uint*>(last);
    bitmap[limb_index / bits_per_uint] &= ~(1 << (limb_index % bits_per_uint));
  }
  template<typename T, std::size_t limb_size>
  void vector<T,limb_size>::uncow_root_if_needed() {
    if (!last && start) uncow_root();
  }
  template<typename T, std::size_t limb_size>
  void vector<T,limb_size>::uncow_root_if_needed_assume_nonempty() {
    assert(start);
    if (root_is_cow_or_empty()) uncow_root();
  }
  template<typename T, std::size_t limb_size>
  auto vector<T,limb_size>::alloc_root_copy_from
   (size_type capacity, size_type bitmap_size,
    const limb_type *const *source, size_type old_capacity) -> limb_type** {
    const std::size_t alloc_size = capacity * sizeof(limb_type*)
      + bitmap_size * sizeof(bitmap_uint);
    limb_type **new_start = static_cast<limb_type **>(operator new(alloc_size));
    std::copy(start, start+old_capacity,
              std::raw_storage_iterator<limb_type**, limb_type*>(new_start));
    return new_start;
  }
  template<typename T, std::size_t limb_size>
  void vector<T,limb_size>::uncow_root() {
    assert(root_is_cow_or_empty() && start);
    const size_type old_capacity = limb_count();
    const size_type new_capacity = next_capacity(old_capacity);
    const size_type bitmap_uints = bitmap_size(new_capacity);
    limb_type **new_start = alloc_root_copy_from(new_capacity, bitmap_uints,
                                                 start, old_capacity);
    limb_type **new_last  = new_start + new_capacity;
    bitmap_uint *new_bitmap = reinterpret_cast<bitmap_uint*>(new_last);
    /* Set first old_capacity bits, clear rest */
    for (size_type i = 0; i < old_capacity/bits_per_uint; ++i) {
      new_bitmap[i] = ~0;
    }
    new_bitmap[old_capacity/bits_per_uint] = (1 << (old_capacity%bits_per_uint))-1;
    for (size_type i = old_capacity/bits_per_uint+1; i < bitmap_uints; ++i) {
      new_bitmap[i] = 0;
    }
    start = new_start;
    last = new_last;
  }
  template<typename T, std::size_t limb_size>
  void vector<T,limb_size>::grow() {
    assert(!root_is_cow_or_empty() || empty());
    assert(size() % limb_size == 0);
    if (limb_count() == size_type(last-start)) {
      /* The root is too small, grow it! */
      const size_type old_capacity = last - start;
      const size_type new_capacity = next_capacity(old_capacity);
      const size_type bitmap_uints = bitmap_size(new_capacity);
      limb_type **new_start = alloc_root_copy_from(new_capacity, bitmap_uints,
                                                   start, old_capacity);
      limb_type **new_last  = new_start + new_capacity;
      const bitmap_uint *bitmap = reinterpret_cast<const bitmap_uint*>(last);
      bitmap_uint *new_bitmap = reinterpret_cast<bitmap_uint*>(new_last);
      const size_type old_bitmap_uints = bitmap_size(old_capacity);
      /* Copy bitmap from old root, clear new bits */
      std::copy(bitmap, bitmap+old_bitmap_uints,
                std::raw_storage_iterator<bitmap_uint*, bitmap_uint>(new_bitmap));
      for (size_type i = old_bitmap_uints; i < bitmap_uints; ++i) {
        new_bitmap[i] = 0;
      }
      /* Delete old root */
      operator delete(reinterpret_cast<void*>(start));
      start = new_start;
      last = new_last;
    }
    assert(limb_count() < size_type(last-start));
    start[limb_count()] = static_cast<limb_type*>(operator new (sizeof(limb_type)));
  }

  template<typename T, std::size_t limb_size>
  T *vector<T,limb_size>::push_back_internal() {
    assert_writable();
    size_type limb = _size / limb_size;
    size_type limb_ix = _size % limb_size;
    uncow_root_if_needed();
    if (limb_ix != 0) uncow_if_needed(limb);
    else grow();
    ++_size;
    return (*start[limb]) + limb_ix;
  }
  template<typename T, std::size_t limb_size>
  void vector<T,limb_size>::push_back(const T &val) {
    new (push_back_internal()) T(val);
  }
  template<typename T, std::size_t limb_size>
  void vector<T,limb_size>::push_back(T &&val) {
    new (push_back_internal()) T(std::move(val));
  }
  template<typename T, std::size_t limb_size> template<typename... Args>
  T &vector<T, limb_size>::emplace_back(Args&&... args) {
    return *new (push_back_internal()) T(std::forward<Args>(args)...);
  }
  template<typename T, std::size_t limb_size>
  const T *vector<T,limb_size>::deref(const limb_type *const *start, size_type ix) {
    size_type limb = ix / limb_size;
    size_type limb_ix = ix % limb_size;
    return (*start[limb]) + limb_ix;
  }
  template<typename T, std::size_t limb_size>
  const T &vector<T,limb_size>::operator[](size_type ix) const {
    assert(ix < _size);
    return *deref(start, ix);
  }
  template<typename T, std::size_t limb_size>
  T &vector<T,limb_size>::mut(size_type ix) {
    assert(ix < _size);
    assert_writable();
    size_type limb = ix / limb_size;
    size_type limb_ix = ix % limb_size;
    uncow_root_if_needed_assume_nonempty();
    uncow_if_needed(limb);
    return (*start[limb])[limb_ix];
  }
  template<typename T, std::size_t limb_size>
  const T &vector<T,limb_size>::at(size_type ix) const {
    if (ix >= _size) throw std::out_of_range("ix");
    return *deref(start, ix);
  }

  template<typename T, std::size_t limb_size, typename Fn>
  void for_each(const vector<T,limb_size> &v, Fn&& f) {
    typedef typename vector<T,limb_size>::size_type size_type;
    typedef typename vector<T,limb_size>::limb_type limb_type;
    size_type size = v.size();
    const limb_type *const *limb = v.start;
    const limb_type *const *const end = limb + size / limb_size;
    for (; limb != end; ++limb) {
      for (size_type limb_ix = 0; limb_ix != limb_size; ++limb_ix) {
        f((**limb)[limb_ix]);
      }
    }
    for (size_type limb_ix = 0; limb_ix < size % limb_size; ++limb_ix) {
      f((**limb)[limb_ix]);
    }
  }
  template<typename T, std::size_t limb_size, typename UP>
  bool all_of(const vector<T,limb_size> &v, UP&& p) {
    typedef typename vector<T,limb_size>::size_type size_type;
    typedef typename vector<T,limb_size>::limb_type limb_type;
    size_type size = v.size();
    const limb_type *const *limb = v.start;
    const limb_type *const *const end = limb + size / limb_size;
    for (; limb != end; ++limb) {
      for (size_type limb_ix = 0; limb_ix != limb_size; ++limb_ix) {
        if (!p((**limb)[limb_ix])) return false;
      }
    }
    for (size_type limb_ix = 0; limb_ix < size % limb_size; ++limb_ix) {
      if (!p((**limb)[limb_ix])) return false;
    }
    return true;
  }

} // namespace gen

#undef IFDEBUG

#endif
