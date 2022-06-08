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

#ifdef HAVE_BOOST_UNIT_TEST_FRAMEWORK
#include <boost/test/unit_test.hpp>

#include "GenMap.h"

#include <random>
#include <set>
#include <functional>

namespace {
  /* This checks for double free, and lets us check for leaks with valgrind */
  struct heap_int {
    heap_int() : heap_int(0) {}
    heap_int(int v) : p(new int(v)) {}
    heap_int(const heap_int &i) : heap_int(*i.p) {}
    heap_int &operator=(const heap_int &i) { *p = *i.p; return *this; }
    heap_int &operator=(int i) { *p = i; return *this; }
    ~heap_int() {
      if (!p)
        BOOST_TEST_ERROR("Double free!");
      else
        delete(std::exchange(p, nullptr));
    }
    operator int() const { return *p; }
  private:
    int *p = nullptr;
  };

  std::mt19937_64 random(std::uint_fast64_t seed = std::mt19937_64::default_seed) {
    std::mt19937_64 engine(seed);
    return engine;
  }

  template<typename Hash>
  void test_iterate(const gen::map<heap_int,heap_int,Hash> &m,
                    std::size_t expected_size) {
    std::size_t size = 0;
    static volatile int sum = 0; // Grr, don't optimise me away
    gen::for_each(m, [&] (const std::pair<const heap_int,heap_int> &pair) {
                       ++size;
                       sum += pair.first + pair.second;
                     });
    BOOST_CHECK_EQUAL(size, expected_size);
  }

  struct id_hash {
    std::size_t operator()(heap_int i) { return static_cast<int>(i); }
  };

  struct constant_hash {
    std::size_t operator()(heap_int i) { return 42; }
  };

  typedef gen::map<heap_int,heap_int,constant_hash> id_hash_map;
}  // namespace

namespace std {
  template <> struct hash<heap_int> {
    typedef std::size_t result_type;
    typedef const heap_int &argument_type;
    std::size_t operator()(const heap_int &i) {
      return std::hash<int>()(i);
    }
  };
}  // namespace std

/* Workaround broken boost version */
#if BOOST_VERSION < 106700 // 1.67
void boost::test_tools::tt_detail::
print_log_value<std::nullptr_t>::operator()(std::ostream &os, std::nullptr_t) {
    os << "null";
}
#endif


BOOST_AUTO_TEST_SUITE(GenMap_test)

BOOST_AUTO_TEST_CASE(Init_default) {
  BOOST_CHECK_EQUAL((id_hash_map().size()),0);
}

BOOST_AUTO_TEST_CASE(Init_foreach) {
  id_hash_map m;
  test_iterate(m, 0);
}

BOOST_AUTO_TEST_CASE(Put_get){
  id_hash_map m;
  BOOST_CHECK_EQUAL(m.find(3),nullptr);
  m.mut(3) = 4;
  BOOST_CHECK_EQUAL(m.size(),1);
  test_iterate(m, 1);
  BOOST_CHECK_EQUAL(m.at(3),4);
  BOOST_CHECK_EQUAL(m.find(4),nullptr);
  BOOST_CHECK_EQUAL(m.find(3+32),nullptr);
}

BOOST_AUTO_TEST_CASE(Put_two){
  id_hash_map m;
  m.mut(3) = 4;
  m.mut(5) = 6;
  BOOST_CHECK_EQUAL(m.size(),2);
  test_iterate(m, 2);
  BOOST_CHECK_EQUAL(m.at(3),4);
  BOOST_CHECK_EQUAL(m.at(5),6);
  BOOST_CHECK_EQUAL(m.find(6),nullptr);
  BOOST_CHECK_EQUAL(m.find(5+32),nullptr);
}

BOOST_AUTO_TEST_CASE(Put_two_reverse){
  id_hash_map m;
  m.mut(5) = 4;
  m.mut(3) = 2;
  BOOST_CHECK_EQUAL(m.size(),2);
  test_iterate(m, 2);
  BOOST_CHECK_EQUAL(m.at(3),2);
  BOOST_CHECK_EQUAL(m.at(5),4);
  BOOST_CHECK_EQUAL(m.find(5+32),nullptr);
  BOOST_CHECK_EQUAL(m.find(3+32),nullptr);
  BOOST_CHECK_EQUAL(m.find(6),nullptr);
}

BOOST_AUTO_TEST_CASE(Put_two_prefix_collision){
  id_hash_map m;
  m.mut(3) = 4;
  m.mut(3+32) = 5;
  BOOST_CHECK_EQUAL(m.size(),2);
  test_iterate(m, 2);
  BOOST_CHECK_EQUAL(m.at(3),4);
  BOOST_CHECK_EQUAL(m.at(3+32),5);
  BOOST_CHECK_EQUAL(m.find(4),nullptr);
}

BOOST_AUTO_TEST_CASE(Put_three_above){
  id_hash_map m;
  m.mut(3) = 4;
  m.mut(5) = 6;
  m.mut(7) = 8;
  BOOST_CHECK_EQUAL(m.size(),3);
  test_iterate(m, 3);
  BOOST_CHECK_EQUAL(m.at(3),4);
  BOOST_CHECK_EQUAL(m.at(5),6);
  BOOST_CHECK_EQUAL(m.at(7),8);
  BOOST_CHECK_EQUAL(m.find(4),nullptr);
  BOOST_CHECK_EQUAL(m.find(6),nullptr);
  BOOST_CHECK_EQUAL(m.find(5+32),nullptr);
}

BOOST_AUTO_TEST_CASE(Put_three_middle){
  id_hash_map m;
  m.mut(3) = 6;
  m.mut(5) = 7;
  m.mut(4) = 8;
  BOOST_CHECK_EQUAL(m.size(),3);
  test_iterate(m, 3);
  BOOST_CHECK_EQUAL(m.at(3),6);
  BOOST_CHECK_EQUAL(m.at(4),8);
  BOOST_CHECK_EQUAL(m.at(5),7);
  BOOST_CHECK_EQUAL(m.find(2),nullptr);
  BOOST_CHECK_EQUAL(m.find(6),nullptr);
  BOOST_CHECK_EQUAL(m.find(4+32),nullptr);
}

BOOST_AUTO_TEST_CASE(Put_three_below){
  id_hash_map m;
  m.mut(3) = 4;
  m.mut(5) = 6;
  m.mut(2) = 7;
  BOOST_CHECK_EQUAL(m.size(),3);
  test_iterate(m, 3);
  BOOST_CHECK_EQUAL(m.at(2),7);
  BOOST_CHECK_EQUAL(m.at(3),4);
  BOOST_CHECK_EQUAL(m.at(5),6);
  BOOST_CHECK_EQUAL(m.find(4),nullptr);
  BOOST_CHECK_EQUAL(m.find(1),nullptr);
  BOOST_CHECK_EQUAL(m.find(2+32),nullptr);
}

BOOST_AUTO_TEST_CASE(Put_sequence_idmap){
  std::vector<int> v;
  for (int i = 0; i < 100; ++i) v.push_back(i);
  id_hash_map m;
  int size = 0;
  for (int i : v) {
    m.mut(i) = i+999;
    test_iterate(m, ++size);
  }
  BOOST_CHECK_EQUAL(m.size(),100);
  for (int i : v) {
    BOOST_CHECK_EQUAL(m.at(i),i+999);
    BOOST_CHECK_EQUAL(m.mut(i),i+999);
  }
  for (int i = -40; i < 0; ++i) {
    BOOST_CHECK_EQUAL(m.find(i),nullptr);
  }
  for (int i = 100; i < 140; ++i) {
    BOOST_CHECK_EQUAL(m.find(i),nullptr);
  }
}

BOOST_AUTO_TEST_CASE(Put_random){
  std::vector<int> v;
  for (int i = 0; i < 100; ++i) v.push_back(i);
  shuffle(v.begin(), v.end(), random(42));
  id_hash_map m;
  int size = 0;
  for (int i : v) {
    m.mut(i) = i+999;
    test_iterate(m, ++size);
  }
  BOOST_CHECK_EQUAL(m.size(),100);
  shuffle(v.begin(), v.end(), random(43));
  for (int i : v) {
    BOOST_CHECK_EQUAL(m.at(i),i+999);
    BOOST_CHECK_EQUAL(m.mut(i),i+999);
    test_iterate(m, 100);
  }
  for (int i = -40; i < 0; ++i) {
    BOOST_CHECK_EQUAL(m.find(i),nullptr);
  }
  for (int i = 100; i < 140; ++i) {
    BOOST_CHECK_EQUAL(m.find(i),nullptr);
  }
}

BOOST_AUTO_TEST_CASE(Put_sequence){
  std::vector<int> v;
  for (int i = 0; i < 100; ++i) v.push_back(i);
  gen::map<heap_int,heap_int> m;
  int size = 0;
  for (int i : v) {
    m.mut(i) = i+999;
    test_iterate(m, ++size);
  }
  BOOST_CHECK_EQUAL(m.size(),100);
  for (int i : v) {
    BOOST_CHECK_EQUAL(m.at(i),i+999);
    BOOST_CHECK_EQUAL(m.mut(i),i+999);
  }
  for (int i = -40; i < 0; ++i) {
    BOOST_CHECK_EQUAL(m.find(i),nullptr);
  }
  for (int i = 100; i < 140; ++i) {
    BOOST_CHECK_EQUAL(m.find(i),nullptr);
  }
}

BOOST_AUTO_TEST_CASE(COW_construct_same_address) {
  std::vector<int> v;
  for (int i = 0; i < 100; ++i) v.push_back(i);
  shuffle(v.begin(), v.end(), random(44));
  gen::map<heap_int,heap_int> m;
  for (int i : v) {
    m.mut(i) = i+999;
  }
  shuffle(v.begin(), v.end(), random(45));
  {
    const auto &mr = m;
    gen::map<heap_int,heap_int> n(mr);
    BOOST_CHECK_EQUAL(m.size(),100);
    BOOST_CHECK_EQUAL(n.size(),100);
    test_iterate(n, 100);
    for (int i : v) {
      const heap_int &mi = m.at(i);
      const heap_int &ni = n.at(i);
      BOOST_CHECK_EQUAL(&mi, &ni);
      BOOST_CHECK_EQUAL(mi,i+999);
    }
    for (int i = -40; i < 0; ++i) {
      BOOST_CHECK_EQUAL(n.find(i),nullptr);
    }
    for (int i = 100; i < 140; ++i) {
      BOOST_CHECK_EQUAL(n.find(i),nullptr);
    }
  }
  BOOST_CHECK_EQUAL(m.size(),100);
  test_iterate(m, 100);
  for (int i : v) {
    BOOST_CHECK_EQUAL(m[i],i+999);
  }
}

BOOST_AUTO_TEST_CASE(COW_update_collision) {
  std::vector<int> v;
  for (int i = 0; i < 100; ++i) v.push_back(i);
  shuffle(v.begin(), v.end(), random(49));
  gen::map<heap_int,heap_int,constant_hash> m;
  for (int i = 0; i < 100; ++i) {
    m.mut(v[i]) = v[i]+999;
    test_iterate(m, i+1);
  }
  shuffle(v.begin(), v.end(), random(50));
  {
    const auto &mr = m;
    gen::map<heap_int,heap_int,constant_hash> n(mr);
    BOOST_CHECK_EQUAL(m.size(),100);
    BOOST_CHECK_EQUAL(n.size(),100);
    test_iterate(n, 100);
    for (int i = 0; i < 100; ++i) {
      n.mut(v[i]) = v[i] + 1999;
      test_iterate(m, 100);
      test_iterate(n, 100);
    }
    for (int i = 0; i < 100; ++i) {
      BOOST_CHECK_EQUAL(m.at(i), i+999);
      BOOST_CHECK_EQUAL(n.at(i), i+1999);
    }
  }
  BOOST_CHECK_EQUAL(m.size(),100);
  test_iterate(m, 100);
  for (int i = 0; i < 100; ++i) {
    BOOST_CHECK_EQUAL(m[i],i+999);
  }
}

BOOST_AUTO_TEST_CASE(COW_add_new){
  std::vector<int> v;
  for (int i = 0; i < 100; ++i) v.push_back(i);
  shuffle(v.begin(), v.end(), random(46));
  gen::map<heap_int,heap_int> m;
  for (int i = 0; i < 50; ++i) {
    m.mut(v[i]) = v[i]+999;
  }
  // gen::__debug_set_tracing(true);
  {
    const auto &mr = m;
    gen::map<heap_int,heap_int> n(mr);
    BOOST_CHECK_EQUAL(m.size(),50);
    BOOST_CHECK_EQUAL(n.size(),50);
    for (int i = 50; i < 100; ++i) {
      n.mut(v[i]) = v[i]+999;
      test_iterate(n, i+1);
      test_iterate(m, 50);
    }
    BOOST_CHECK_EQUAL(m.size(),50);
    BOOST_CHECK_EQUAL(n.size(),100);
    for (int i = -40; i < 0; ++i) {
      BOOST_CHECK_EQUAL(m.find(i),nullptr);
      BOOST_CHECK_EQUAL(n.find(i),nullptr);
    }
    for (int i = 0; i < 100; ++i) {
      int x = v[i];
      const heap_int &ni = n.at(x);
      BOOST_CHECK_EQUAL(ni,x+999);
      if (i < 50) {
        const heap_int &mi = m.at(x);
        BOOST_CHECK_EQUAL(&ni,&mi);
      } else {
        BOOST_CHECK_EQUAL(m.find(x), nullptr);
      }
    }
    for (int i = 100; i < 140; ++i) {
      BOOST_CHECK_EQUAL(m.find(i),nullptr);
      BOOST_CHECK_EQUAL(n.find(i),nullptr);
    }
  }
  BOOST_CHECK_EQUAL(m.size(),50);
  test_iterate(m, 50);
  for (int i = 0; i < 50; ++i) {
    BOOST_CHECK_EQUAL(m[v[i]],v[i]+999);
  }
  // gen::__debug_set_tracing(false);
}

BOOST_AUTO_TEST_CASE(COW_add_collision) {
  gen::map<heap_int,heap_int,constant_hash> m;
  for (int i = 0; i < 50; ++i) {
    m.mut(i) = i+999;
    test_iterate(m, i+1);
  }
  {
    const auto &mr = m;
    gen::map<heap_int,heap_int,constant_hash> n(mr);
    BOOST_CHECK_EQUAL(m.size(),50);
    BOOST_CHECK_EQUAL(n.size(),50);
    test_iterate(n, 50);
    for (int i = 50; i < 100; ++i) {
      n.mut(i) = i + 1999;
      test_iterate(m, 50);
      test_iterate(n, i+1);
    }
    for (int i = 0; i < 50; ++i) {
      BOOST_CHECK_EQUAL(m.at(i), i+999);
      BOOST_CHECK_EQUAL(n.at(i), i+999);
    }
    for (int i = 50; i < 100; ++i) {
      BOOST_CHECK_EQUAL(n.at(i), i+1999);
    }
  }
  BOOST_CHECK_EQUAL(m.size(),50);
  test_iterate(m, 50);
  for (int i = 0; i < 50; ++i) {
    BOOST_CHECK_EQUAL(m[i],i+999);
  }
}

BOOST_AUTO_TEST_CASE(COW_overwrite_some){
  std::vector<int> v, w;
  for (int i = 0; i < 100; ++i) v.push_back(i);
  shuffle(v.begin(), v.end(), random(48));
  gen::map<heap_int,heap_int> m;
  for (int i : v) {
    m.mut(i) = i+999;
  }
  w = v;
  shuffle(v.begin(), v.end(), random(49));
  v.resize(50);
  std::set<int> moved(v.begin(), v.end());
  // gen::__debug_set_tracing(true);
  {
    const auto &mr = m;
    gen::map<heap_int,heap_int> n(mr);
    BOOST_CHECK_EQUAL(m.size(),100);
    BOOST_CHECK_EQUAL(n.size(),100);
    for (int i : v) {
      n.mut(i) = i+1999;
      test_iterate(m, 100);
      test_iterate(n, 100);
    }
    BOOST_CHECK_EQUAL(m.size(),100);
    BOOST_CHECK_EQUAL(n.size(),100);
    for (int i = -40; i < 0; ++i) {
      BOOST_CHECK_EQUAL(m.find(i),nullptr);
      BOOST_CHECK_EQUAL(n.find(i),nullptr);
    }
    for (int i = 0; i < 100; ++i) {
      const heap_int &mi = m.at(i);
      BOOST_CHECK_EQUAL(mi,i+999);
      const heap_int &ni = n.at(i);
      if (moved.count(i)) {
        BOOST_CHECK_EQUAL(ni,i+1999);
      } else {
        /* If there's a hash collision, this may not be satisfied */
        BOOST_WARN_EQUAL(&ni,&mi);
      }
    }
    for (int i = 100; i < 140; ++i) {
      BOOST_CHECK_EQUAL(m.find(i),nullptr);
      BOOST_CHECK_EQUAL(n.find(i),nullptr);
    }
  }
  test_iterate(m, 100);
  for (int i : w) {
    BOOST_CHECK_EQUAL(m[i], i+999);
  }
  // gen::__debug_set_tracing(false);
}

BOOST_AUTO_TEST_CASE(memory_management){
  gen::map<heap_int, heap_int> m;
}

BOOST_AUTO_TEST_SUITE_END()
#endif
