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
#ifndef __GEN_MAP_H__
#define __GEN_MAP_H__

#include "GenHash.h"

#ifndef NDEBUG
/* For reference-count */
#  include <atomic>
#  include <memory>
#endif
#include <cstdint>
#include <functional>
#include <tuple>
#include <utility>

namespace gen {

  template<typename Key, typename T, typename Hash = hash<Key>,
           class KeyEqual = std::equal_to<Key>, class ValueType = std::pair<const Key, T>>
  class map;

  namespace impl {
    template<typename Key, typename T, class ValueType>
    struct map_value_container {
      map_value_container(const Key &k) : value(k, T()) {}
      ValueType value;
    };
    template<typename Key, typename T>
    struct map_value_container<Key, T, std::tuple<const Key, T>> {
      map_value_container(const Key &k)
        : value(std::piecewise_construct, std::make_tuple<const Key &>(k),
                std::make_tuple<>()) {}
      std::tuple<const Key, T> value;
    };
  }  // namespace impl

  template<typename Key, typename T, typename Hash, class KeyEqual,
           class ValueType>
  class map {
    const unsigned log2_arity = 5;
    const unsigned arity = 1 << log2_arity; // 32
    typedef uint32_t bitmap_type;
    typedef int32_t gen_type;
    typedef uint32_t size_type;
    typedef uint_fast32_t fast_size_type;
    typedef ValueType value_type;
    /* Just as to not require */
    typedef typename std::remove_cv<
      typename std::remove_reference<
        typename std::result_of<Hash(const Key&)>::type>::type>::type hash_type;
    static_assert(std::is_same<hash_type, std::size_t>::value, "Coward");
    static hash_type hash_key(const Key &k) { return Hash()(k); }

    struct child {
      /* negative generation encodes a leaf */
      gen_type _generation;
      bool is_leaf() const { return _generation < 0; }
      gen_type get_generation() const {
        return _generation < 0 ? -1-_generation : _generation;
      }
    };

    typedef impl::map_value_container<Key, T, ValueType> value_container;

    /* TODO: Can we shrink memory use of this one somehow? */
    struct leaf : child, value_container {
      leaf(gen_type generation, const Key &k) : value_container(k) {
        child::_generation = -1-generation;
      }
      leaf(gen_type generation, const leaf &other)
        : value_container(other), next(other.next) {
        child::_generation = -1-generation;
      }
      leaf(const leaf&) = delete;
      /* Always a leaf */
      child *next;
    };

    struct node : child {
      node(gen_type generation, bitmap_type bitmap) : bitmap(bitmap) {
        child::_generation = generation;
      }
      bitmap_type bitmap;
      /* This is a variable-sized array. I believe this is not legal C++, so if
       * there are weird heisenbugs, blame this. */
      child *array[1];
    };

    gen_type generation = 0;
    size_type _size = 0;
    child *root = 0;

    node *uncow_node(child **);
    void uncow_leaf(child **);
    node *alloc_node(fast_size_type arity, bitmap_type bitmap);
    node *realloc_node(node *, fast_size_type arity);
    void free_child(child *);

#ifndef NDEBUG
    typedef std::atomic_uint_fast32_t _debug_refcount_type;
    _debug_refcount_type *_debug_parent = nullptr;
    std::unique_ptr<_debug_refcount_type> _debug_refcount =
      std::make_unique<_debug_refcount_type>(0);
    void assert_writable() const;
#else
    inline constexpr void assert_writable() const {}
#endif

    template<typename Fn>
    static void for_each(const child *, Fn &f);

  public:
    /* Empty */
    map() {}
    /* Copy-on-write duplicate another map. It must not be modified after this. */
    explicit map(const map &);
    /* Steal another map. It will be empty after this. It is allowed to steal
     * a map that has COW references, but then the new map may not be
     * modified. */
    map(map &&other) { swap(other); }
    ~map();
    /* Copy-on-write duplicate another map. It must not be modified after this. */
    map &operator=(const map &other);
    /* Steal another map. Its contents will be swapped with this one. It
     * is allowed to steal a map that has COW references, but then *this
     * may not be modified further. */
    map &operator=(map &&other) { assert_writable(); swap(other); return *this; }

    /* Swap contents and ownership with another map */
    void swap(map &other);

    size_type size() const noexcept { return _size; }
    const T *find(const Key &k) const;
    T &mut(const Key &k);
    const T &at(const Key &k) const;
    /* If possible, use find or at instead. Use of operator[] instantiates a
     * global default T variable to be returned if the key is missing
     */
    const T &operator[](const Key &k) const;
    fast_size_type count(const Key &k) const { return find(k) != nullptr; }

    template<class Fn> void for_each(Fn &&f) const;
  };

  template<typename Key, typename T, typename Hash, class KeyEqual, class VT>
  inline void swap(map<Key,T,Hash,KeyEqual,VT> &a, map<Key,T,Hash,KeyEqual,VT> &b) {
    a.swap(b);
  }

  template<class K, class T, class H, class KE, class VT, class Fn>
  inline void for_each(const map<K,T,H,KE,VT> &v, Fn&& f) {
    return v.for_each(std::forward<Fn>(f));
  }
}  // namespace gen

/* Start implementation */

#include <cassert>

namespace gen {

  template<typename Key, typename T, typename Hash, class KeyEqual, class VT>
  map<Key,T,Hash,KeyEqual,VT>::map(const map &other) {
    _size = other._size;
    if ((root = other.root)) {
#ifndef NDEBUG
      ++*(_debug_parent = other._debug_refcount.get());
#endif
      generation = other.generation + 1;
    }
  }

  template<typename Key, typename T, typename Hash, class KeyEqual, class VT>
  map<Key,T,Hash,KeyEqual,VT>::~map() {
    assert_writable();
#ifndef NDEBUG
    if (_debug_parent) --*_debug_parent;
#endif
    free_child(root);
  }

  template<typename Key, typename T, typename Hash, class KeyEqual, class VT>
  auto map<Key,T,Hash,KeyEqual,VT>::operator=(const map &other) -> map & {
    this->~map();
    new(this) map(other);
    return *this;
  }

#ifndef NDEBUG
  template<typename Key, typename T, typename Hash, class KeyEqual, class VT>
  void map<Key,T,Hash,KeyEqual,VT>::assert_writable() const {
    assert((*_debug_refcount) == 0 && "Writable");
  }
#endif


  template<typename Key, typename T, typename Hash, class KeyEqual, class VT>
  void map<Key,T,Hash,KeyEqual,VT>::swap(map &other) {
    std::swap(generation, other.generation);
    std::swap(_size, other._size);
    std::swap(root, other.root);
#ifndef NDEBUG
    std::swap(_debug_parent, other._debug_parent);
    std::swap(_debug_refcount, other._debug_refcount);
#endif
  }

  namespace impl {
    inline unsigned popcount(uint32_t bitmap) {
      static_assert(sizeof(unsigned) >= sizeof(uint32_t), "No truncation");
// #ifdef HAVE___POPCNT
//       return __popcount(bitmap);
// #elif HAVE__mm_popcnt_u32
//       return _mm_popcnt_u32(bitmap);
// #elif defined(HAVE___BUILTIN_POPCOUNT)
      return __builtin_popcount(bitmap);
// #else
// #  error "A popcount intrinsic is needed for good performance"
// #endif
    }

    template<typename T> struct const_ref_provider {
      static const T val;
    };
    template<typename T> const T const_ref_provider<T>::val = T();
  }  // namespace impl

  template<typename Key, typename T, typename Hash, class KeyEqual, class VT>
  void map<Key,T,Hash,KeyEqual,VT>::free_child(child *child) {
    if (!child || child->get_generation() != generation) return;
    if (child->is_leaf()) {
      leaf *leaf = static_cast<struct leaf *>(child);
      do {
        struct leaf *next = static_cast<struct leaf*>(leaf->next);
        delete leaf;
        leaf = next;
      } while(leaf && leaf->get_generation() == generation);
    } else {
      node *node = static_cast<struct node *>(child);
      fast_size_type size = impl::popcount(node->bitmap);
      while(size) {
        free_child(node->array[--size]);
      }
      node->~node();
      ::free(node);
    }
  }

  template<typename Key, typename T, typename Hash, class KeyEqual, class VT>
  const T *map<Key,T,Hash,KeyEqual,VT>::find(const Key &k) const {
    hash_type hash = hash_key(k);
    const child *child = root;
    while (child && !child->is_leaf()) {
      const node *node = static_cast<const struct node*>(child);
      fast_size_type index = hash % arity;
      hash = hash / arity;
      if (((node->bitmap >> index) & 1) != 1) return nullptr;
      fast_size_type position = impl::popcount(node->bitmap >> index)-1;
      child = node->array[position];
    }
    KeyEqual eq;
    while(child) {
      assert(child->is_leaf());
      const leaf *leaf = static_cast<const struct leaf*>(child);
      if (eq(k,std::get<0>(leaf->value))) {
        return std::addressof(std::get<1>(leaf->value));
      }
      child = leaf->next;
    }
    return nullptr;
  }

  template<typename Key, typename T, typename Hash, class KeyEqual, class VT>
  const T &map<Key,T,Hash,KeyEqual,VT>::operator[](const Key &k) const {
    if (const T *p = find(k)) {
      return *p;
    } else {
      return impl::const_ref_provider<T>::val;
    }
  }

  template<typename Key, typename T, typename Hash, class KeyEqual, class VT>
  const T &map<Key,T,Hash,KeyEqual,VT>::at(const Key &k) const {
    if (const T *p = find(k)) {
      return *p;
    } else {
      throw new std::out_of_range("k not in map");
      assert(false && "Key not found in at()");
      abort();
    }
  }

  template<typename Key, typename T, typename Hash, class KeyEqual, class VT>
  T &map<Key,T,Hash,KeyEqual,VT>::mut(const Key &k) {
    assert_writable();
    hash_type hash = hash_key(k);
    fast_size_type depth = 0;
    child **child = &root;
    while (*child && !(*child)->is_leaf()) {
      node *node = static_cast<struct node*>(*child);
      fast_size_type index = hash % arity;
      hash /= arity;
      depth += log2_arity;
      if (((node->bitmap >> index) & 1) == 1) {
        fast_size_type position = impl::popcount(node->bitmap >> index)-1;
        if ((*child)->get_generation() != generation) {
          node = uncow_node(child);
          node = static_cast<struct node*>(*child);
        }
        child = &node->array[position];
      } else {
        /* Grow the node to include the new child */
        bitmap_type old_bitmap = node->bitmap;
        size_t count = impl::popcount(old_bitmap)+1;
        /* Notice the lack of -1 here; we're using the old bitmap */
        fast_size_type position = impl::popcount(old_bitmap >> index);
        bitmap_type new_bitmap = old_bitmap | (1 << index);
        const struct node *old_node;
        if ((*child)->get_generation() == generation) {
          *child = node = realloc_node(node, count);
          old_node = node;
          node->bitmap = new_bitmap;
        } else {
          old_node = node;
          *child = node = alloc_node(count, new_bitmap);
          for (fast_size_type i = 0; i < position; ++i) {
            node->array[i] = old_node->array[i];
          }
        }
        for (fast_size_type i = count-1; i > position; --i) {
          /* Might alias, thus the reverse iteration order. */
          node->array[i] = old_node->array[i-1];
        }
        node->array[position] = nullptr;
        child = &node->array[position];
        break;
      }
    }
    /* There are many cases now:
     * 1. It exists in child list, and is not cow (victory, return reference)
     * 2. It exists in child list, but is cow (uncow it and all nodes before it)
     * 3a. It does not exist in child list (insert it first; no need to uncow
     *     any other)
     * 3b. The child list is empty (can be handled by same code as 3a)
     *
     * Is there any way we can handle 2 in one pass without uncowing other
     * elements in case 3a?
     */
    leaf *leaf = static_cast<struct leaf *>(*child);
    KeyEqual eq;
    while (leaf && !eq(k,std::get<0>(leaf->value))) {
      leaf = static_cast<struct leaf*>(leaf->next);
    }
    if (leaf) {
      if (leaf->get_generation() != generation) {
        /* We have to uncow all the way to leaf */
        while ((*child)->get_generation() == generation) {
          child = &static_cast<struct leaf*>(*child)->next;
        }
        while (*child != leaf) {
          uncow_leaf(child);
          child = &static_cast<struct leaf*>(*child)->next;
        }
        uncow_leaf(child);
        leaf = static_cast<struct leaf *>(*child);
        assert(eq(k, std::get<0>(leaf->value)));
      }
      return std::get<1>(leaf->value);
    } else {
      if (*child) {
        hash_type first_hash
          = hash_key(std::get<0>(static_cast<struct leaf *>(*child)->value));
        first_hash >>= depth;
        if (first_hash != hash) {
          while(1) {
            /* We have to introduce a new node to disambiguate */
            const fast_size_type index_for_existing = first_hash % arity;
            const fast_size_type index_for_new = hash % arity;
            const fast_size_type new_node_arity
              = index_for_existing == index_for_new ? 1 : 2;
            bitmap_type bitmap = (1 << index_for_existing) | (1 << index_for_new);
            struct node *node = alloc_node(new_node_arity, bitmap);
            const fast_size_type position_for_existing = index_for_existing < index_for_new;
            const fast_size_type position_for_new = index_for_new < index_for_existing;
            node->array[position_for_new] = nullptr;
            node->array[position_for_existing] = std::exchange(*child, node);
            child = &node->array[position_for_new];
            if (index_for_existing != index_for_new) {
              break;
            } else {
              hash /= arity;
              first_hash /= arity;
            }
          }
        }
      }
      leaf = new struct leaf(generation, k);
      leaf->next = std::exchange(*child, leaf);
      ++_size;
      return std::get<1>(leaf->value);
    }
  }
  template<typename Key, typename T, typename Hash, class KeyEqual, class VT>
  auto map<Key,T,Hash,KeyEqual,VT>::uncow_node(child **old_p) -> node * {
    const node *old = static_cast<const node*>(*old_p);
    fast_size_type arity = impl::popcount(old->bitmap);
    node *n = alloc_node(arity, old->bitmap);
    for (fast_size_type i = 0; i < arity; ++i) {
      n->array[i] = old->array[i];
    }
    *old_p = n;
    return n;
  }
  template<typename Key, typename T, typename Hash, class KeyEqual, class VT>
  void map<Key,T,Hash,KeyEqual,VT>::uncow_leaf(child **node) {
    *node = new leaf(generation, *static_cast<leaf*>(*node));
  }
  template<typename Key, typename T, typename Hash, class KeyEqual, class VT>
  auto map<Key,T,Hash,KeyEqual,VT>::alloc_node(fast_size_type arity, bitmap_type bitmap) -> node * {
    assert(impl::popcount(bitmap) == arity);
    assert(arity > 0);
    std::size_t size = sizeof(struct node) + sizeof(struct child*)*(arity-1);
    struct node *node = static_cast<struct node*>(::malloc(size));
    if (!node) {
      throw std::bad_alloc();
      abort();
    }
    new (node) struct node(generation, bitmap);
    return node;
  }
  template<typename Key, typename T, typename Hash, class KeyEqual, class VT>
  auto map<Key,T,Hash,KeyEqual,VT>::realloc_node(node *old, fast_size_type arity) -> node * {
    std::size_t size = sizeof(struct node) + sizeof(struct child*)*(arity-1);
    struct node *n = static_cast<struct node*>(::realloc(old, size));
    if (!n) {
      throw std::bad_alloc();
      abort();
    }
    return n;
  }

  template<typename Key, typename T, typename Hash, class KeyEqual, class VT>
  template<typename Fn>
  void map<Key,T,Hash,KeyEqual,VT>::for_each(Fn &&f) const {
    if (!root) return;
    map::for_each<Fn>(root, f);
  }

  template<typename Key, typename T, typename Hash, class KeyEqual, class VT>
  template<typename Fn>
  void map<Key,T,Hash,KeyEqual,VT>::for_each(const child *child, Fn &f) {
    if (child->is_leaf()) {
      const leaf *leaf = static_cast<const struct leaf *>(child);
      do {
        f(leaf->value);
        assert(!leaf->next || leaf->next->is_leaf());
      } while ((leaf = static_cast<const struct leaf*>(leaf->next)));
    } else {
      const node *node = static_cast<const struct node *>(child);
      const fast_size_type size = impl::popcount(node->bitmap);
      const struct child *const *const end = node->array + size;
      for (const struct child *const *elem = node->array; elem != end; ++elem) {
        for_each(*elem, f);
      }
    }
  }
}  // namespace gen

#endif
