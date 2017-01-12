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

#ifdef HAVE_BOOST_UNIT_TEST_FRAMEWORK
#include <boost/test/unit_test.hpp>

#include "VClock.h"

BOOST_AUTO_TEST_SUITE(VClock_int_test)

BOOST_AUTO_TEST_CASE(Init_default){
  BOOST_CHECK_EQUAL(VClock<int>()[0],0);
  BOOST_CHECK_EQUAL(VClock<int>()[42],0);
}

BOOST_AUTO_TEST_CASE(Init_vector){
  VClock<int> vc(std::vector<int>({1,2,3,0,2}));
  BOOST_CHECK_EQUAL(vc[0],1);
  BOOST_CHECK_EQUAL(vc[1],2);
  BOOST_CHECK_EQUAL(vc[2],3);
  BOOST_CHECK_EQUAL(vc[3],0);
  BOOST_CHECK_EQUAL(vc[4],2);
  BOOST_CHECK_EQUAL(vc[5],0);
  BOOST_CHECK_EQUAL(vc[100],0);
}

BOOST_AUTO_TEST_CASE(Init_initializer_list){
  VClock<int> vc = {3,2,1};
  BOOST_CHECK_EQUAL(vc[0],3);
  BOOST_CHECK_EQUAL(vc[1],2);
  BOOST_CHECK_EQUAL(vc[2],1);
  BOOST_CHECK_EQUAL(vc[3],0);
}

BOOST_AUTO_TEST_CASE(Assignment_1){
  VClock<int> vc1 = {1,2,3};
  VClock<int> vc2 = {4,5,6};
  vc1 = vc2;
  BOOST_CHECK_EQUAL(vc1[0],4);
  BOOST_CHECK_EQUAL(vc1[1],5);
  BOOST_CHECK_EQUAL(vc1[2],6);
  BOOST_CHECK_EQUAL(vc1[3],0);
}

BOOST_AUTO_TEST_CASE(Assignment_2){
  VClock<int> vc1 = {1};
  VClock<int> vc2 = {4,5,6};
  vc1 = vc2;
  BOOST_CHECK_EQUAL(vc1[0],4);
  BOOST_CHECK_EQUAL(vc1[1],5);
  BOOST_CHECK_EQUAL(vc1[2],6);
  BOOST_CHECK_EQUAL(vc1[3],0);
}

BOOST_AUTO_TEST_CASE(Assignment_3){
  VClock<int> vc1 = {1,2,3};
  VClock<int> vc2 = {4};
  vc1 = vc2;
  BOOST_CHECK_EQUAL(vc1[0],4);
  BOOST_CHECK_EQUAL(vc1[1],0);
  BOOST_CHECK_EQUAL(vc1[2],0);
  BOOST_CHECK_EQUAL(vc1[3],0);
}

BOOST_AUTO_TEST_CASE(Equality){
  BOOST_CHECK(VClock<int>({1,2,3}) == VClock<int>({1,2,3}));
  BOOST_CHECK(VClock<int>({1,2,3}) == VClock<int>({1,2,3,0,0}));
  BOOST_CHECK(VClock<int>({1,2,3}) != VClock<int>({1,1,3}));
  BOOST_CHECK(VClock<int>({1,2,3}) != VClock<int>({1,2}));
  BOOST_CHECK(VClock<int>({0,0,0}) == VClock<int>());
  BOOST_CHECK(VClock<int>({1}) != VClock<int>());
}

BOOST_AUTO_TEST_CASE(Less){
  BOOST_CHECK(VClock<int>({1,2,3}).lt(VClock<int>({1,3,3})));
  BOOST_CHECK(VClock<int>({1,3,3}).gt(VClock<int>({1,2,3})));
  BOOST_CHECK(!(VClock<int>({1,2,3}).lt(VClock<int>({1,2,3}))));
  BOOST_CHECK(VClock<int>({1,2,3}).leq(VClock<int>({1,2,3})));
  BOOST_CHECK(VClock<int>({1,2,3}).geq(VClock<int>({1,2,3})));
  BOOST_CHECK(!(VClock<int>({1,2,3}).gt(VClock<int>({1,2,3}))));
  BOOST_CHECK(VClock<int>({1,2}).lt(VClock<int>({1,2,0,1})));
  BOOST_CHECK(!(VClock<int>({1,2,0,1}).lt(VClock<int>({1,2}))));
  BOOST_CHECK(VClock<int>().leq(VClock<int>({0,0})));
  BOOST_CHECK(!(VClock<int>().lt(VClock<int>({0,0}))));
}

BOOST_AUTO_TEST_CASE(Addition){
  BOOST_CHECK_EQUAL(VClock<int>() + VClock<int>({1,2,3}),VClock<int>({1,2,3}));
  BOOST_CHECK_EQUAL(VClock<int>({1,2,3}) + VClock<int>({3,2,1}),VClock<int>({3,2,3}));
  BOOST_CHECK_EQUAL(VClock<int>({1,0,0,0,3})+VClock<int>({1,2,0,0,2,4}),VClock<int>({1,2,0,0,3,4}));
}

BOOST_AUTO_TEST_CASE(Addition_2){
  VClock<int> vc;
  VClock<int> vc2 = {1,2,3};
  vc += vc2;
  BOOST_CHECK_EQUAL(vc,vc2);
  vc = {4,0,1};
  vc += vc2;
  BOOST_CHECK_EQUAL(vc,VClock<int>({4,2,3}));
}

BOOST_AUTO_TEST_SUITE_END()
#endif
