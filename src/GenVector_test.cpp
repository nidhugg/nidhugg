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

#include "GenVector.h"

BOOST_AUTO_TEST_SUITE(GenVector_test)

BOOST_AUTO_TEST_CASE(Init_default){
    BOOST_CHECK_EQUAL(gen::vector<int>().size(),0);
}

BOOST_AUTO_TEST_CASE(Push_one){
    gen::vector<int> v;
    v.push_back(42);
    BOOST_CHECK_EQUAL(v.size(),1);
    BOOST_CHECK_EQUAL(v[0],42);
    BOOST_CHECK_EQUAL(v.mut(0),42);
    v.mut(0) = 44;
    BOOST_CHECK_EQUAL(v.mut(0),44);
}

BOOST_AUTO_TEST_CASE(Push_move){
    struct move_only {
        move_only() = default;
        move_only(const move_only &) { BOOST_TEST_ERROR("Copy disallowed"); }
        move_only(move_only &&) = default;
        move_only &operator=(const move_only &) {
            BOOST_TEST_ERROR("Copy disallowed");
            return *this;
        }
        move_only &operator=(move_only &&) = default;
    };
    gen::vector<move_only> v;
    v.push_back(move_only());
}

BOOST_AUTO_TEST_CASE(Push_several){
    for (int i = 2; i < 6; ++i) {
        gen::vector<int, 2> v;
        for (int j = 0; j < i; ++j) {
            v.push_back(42+j);
            BOOST_CHECK_EQUAL(v.size(),j+1);
        }
        for (int j = 0; j < i; ++j) {
            BOOST_CHECK_EQUAL(v[j],42+j);
        }
    }
}

#define CONSTRUCT_INITER(T,N,V) T N(V)
#define ASSIGN_INITER(T,N,V) T N; N = (V)
#define CASE_COW__same_address(Name, Initer)            \
    BOOST_AUTO_TEST_CASE(COW_##Name##_same_address) {   \
        for (int i = 2; i < 6; ++i) {                   \
            gen::vector<int, 2> v;                      \
            for (int j = 0; j < i; ++j) {               \
                v.push_back(42 + j);                    \
            }                                           \
            typedef gen::vector<int, 2> gvi2;           \
            Initer(gvi2, w, v);                         \
            BOOST_CHECK_EQUAL(v.size(), w.size());      \
            for (int j = 0; j < i; ++j) {               \
                BOOST_CHECK_EQUAL(&v[j], &w[j]);        \
            }                                           \
        }                                               \
  }
CASE_COW__same_address(construct, CONSTRUCT_INITER)
CASE_COW__same_address(assign, ASSIGN_INITER)

BOOST_AUTO_TEST_CASE(COW_push) {
    for (int i = 0; i < 5; ++i) {
        gen::vector<int, 2> v;
        for (int k = 0; k < i; ++k) {
            v.push_back(42 + k);
        }
        for (int j = i; j < 6; ++j) {
            gen::vector<int, 2> w(v);
            for (int k = i; k < j; ++k) {
                w.push_back(42 + k);
                BOOST_CHECK_EQUAL(w.size(),k+1);
            }
            BOOST_CHECK_EQUAL(v.size(),i);
            for (int k = 0; k < i; ++k)
                BOOST_CHECK_EQUAL(v[k],42+k);
            for (int k = 0; k < j; ++k)
                BOOST_CHECK_EQUAL(w[k],42+k);
        }
    }
}

BOOST_AUTO_TEST_CASE(COW_mut) {
    for (int i = 1; i < 5; ++i) {
        gen::vector<int, 2> v;
        for (int k = 0; k < i; ++k) {
            v.push_back(42 + k);
        }
        for (int j = 0; j < i; ++j) {
            for (int k = 0; j < i; ++j) {
                gen::vector<int, 2> w(v);
                w.mut(j) = 42+i;
                if (k != j) w.mut(k) = 43+i;
                BOOST_CHECK_EQUAL(v.size(),i);
                BOOST_CHECK_EQUAL(w.size(),i);
                for (int l = 0; l < i; ++l)
                    BOOST_CHECK_EQUAL(v[l],42+l);
                for (int l = 0; l < j; ++l) {
                    int expected = (l ==j ) ? 42+i : ((l == k) ? 43+i : 42+l);
                    BOOST_CHECK_EQUAL(w[l],expected);
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(COW_push_mut) {
    for (int i = 1; i < 5; ++i) {
        gen::vector<int, 2> v;
        for (int k = 0; k < i; ++k) {
            v.push_back(42 + k);
        }
        for (int j = 0; j < i+1; ++j) {
            gen::vector<int, 2> w(v);
            w.push_back(42+i);
            w.mut(j) = 43+i;
            BOOST_CHECK_EQUAL(v.size(),i);
            BOOST_CHECK_EQUAL(w.size(),i+1);
            for (int l = 0; l < i; ++l)
                BOOST_CHECK_EQUAL(v[l],42+l);
            for (int l = 0; l < j; ++l) {
                int expected = (l == j) ? 43+i : 42+l;
                BOOST_CHECK_EQUAL(w[l],expected);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(COW_mut_push) {
    for (int i = 1; i < 5; ++i) {
        gen::vector<int, 2> v;
        for (int k = 0; k < i; ++k) {
            v.push_back(42 + k);
        }
        for (int j = 0; j < i; ++j) {
            gen::vector<int, 2> w(v);
            w.mut(j) = 43+i;
            w.push_back(42+i);
            BOOST_CHECK_EQUAL(v.size(),i);
            BOOST_CHECK_EQUAL(w.size(),i+1);
            for (int l = 0; l < i; ++l)
                BOOST_CHECK_EQUAL(v[l],42+l);
            for (int l = 0; l < j; ++l) {
                int expected = (l == j) ? 43+i : 42+l;
                BOOST_CHECK_EQUAL(w[l],expected);
            }
        }
    }
}

#define CASE_move__push(Name, Initer)                           \
    BOOST_AUTO_TEST_CASE(move_ ## Name ## _push) {              \
        for (int i = 2; i < 6; ++i) {                           \
            gen::vector<int, 2> v;                              \
            for (int j = 0; j < i; ++j) {                       \
                v.push_back(42+j);                              \
            }                                                   \
            typedef gen::vector<int, 2> gvi2;                   \
            Initer(gvi2, w, std::move(v));                      \
            BOOST_CHECK_EQUAL(v.size(), 0);                     \
            BOOST_CHECK_EQUAL(w.size(), i);                     \
            for (int j = 0; j < i; ++j) {                       \
                BOOST_CHECK_EQUAL(w[j],42+j);                   \
            }                                                   \
            /* v is divorced from w, we can push to either */   \
            v.push_back(99);                                    \
            w.push_back(101);                                   \
            BOOST_CHECK_EQUAL(v.size(), 1);                     \
            BOOST_CHECK_EQUAL(w.size(), i+1);                   \
            for (int j = 0; j < i; ++j) {                       \
                BOOST_CHECK_EQUAL(w[j],42+j);                   \
            }                                                   \
            BOOST_CHECK_EQUAL(w[i], 101);                       \
            BOOST_CHECK_EQUAL(v[0], 99);                        \
        }                                                       \
    }
CASE_move__push(assign, ASSIGN_INITER)
CASE_move__push(construct, CONSTRUCT_INITER)

BOOST_AUTO_TEST_SUITE_END()
#endif
