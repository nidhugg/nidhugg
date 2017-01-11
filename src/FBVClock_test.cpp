/* Copyright (C) 2015-2017 Carl Leonardsson
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

#include "FBVClock.h"

BOOST_AUTO_TEST_SUITE(FBVClock_test)

BOOST_AUTO_TEST_CASE(Initialization){
  FBVClock::ClockSystemID cid = FBVClock::new_clock_system();
  FBVClock a(cid,0);
  FBVClock b(cid,1);
  FBVClock c(cid,2);
  FBVClock b2(cid,1);
  BOOST_CHECK(a.to_string() == "100");
  BOOST_CHECK(b.to_string() == "010");
  BOOST_CHECK(c.to_string() == "001");
  BOOST_CHECK(b2.to_string() == "010");
}

BOOST_AUTO_TEST_CASE(Addition_1){
  FBVClock::ClockSystemID cid = FBVClock::new_clock_system();
  FBVClock a(cid,0);
  FBVClock b(cid,1);
  FBVClock c(cid,2);
  c += a;
  BOOST_CHECK(a.to_string() == "100");
  BOOST_CHECK(b.to_string() == "010");
  BOOST_CHECK(c.to_string() == "101");
}

BOOST_AUTO_TEST_CASE(Addition_2){
  FBVClock::ClockSystemID cid = FBVClock::new_clock_system();
  FBVClock a(cid,0);
  FBVClock b(cid,1);
  FBVClock c(cid,2);
  c += b;
  b += a;
  BOOST_CHECK(a.to_string() == "100");
  BOOST_CHECK(b.to_string() == "110");
  BOOST_CHECK(c.to_string() == "111");
}

BOOST_AUTO_TEST_CASE(Addition_3){
  FBVClock::ClockSystemID cid = FBVClock::new_clock_system();
  FBVClock a(cid,0);
  FBVClock b(cid,1);
  FBVClock c(cid,2);
  FBVClock d(cid,3);
  d += c;
  c += b;
  b += a;
  BOOST_CHECK(a.to_string() == "1000");
  BOOST_CHECK(b.to_string() == "1100");
  BOOST_CHECK(c.to_string() == "1110");
  BOOST_CHECK(d.to_string() == "1111");
}

BOOST_AUTO_TEST_CASE(Addition_4){
  FBVClock::ClockSystemID cid = FBVClock::new_clock_system();
  FBVClock a(cid,0);
  FBVClock b(cid,1);
  FBVClock c(cid,2);
  b += a;
  BOOST_CHECK(a.to_string() == "100");
  BOOST_CHECK(b.to_string() == "110");
  BOOST_CHECK(c.to_string() == "001");
}

BOOST_AUTO_TEST_CASE(Elem_access_1){
  FBVClock::ClockSystemID cid = FBVClock::new_clock_system();
  FBVClock a(cid,0);
  FBVClock b(cid,1);
  FBVClock c(cid,2);
  b += c;
  BOOST_CHECK(a[0]);
  BOOST_CHECK(!a[1]);
  BOOST_CHECK(!a[2]);
  BOOST_CHECK(!b[0]);
  BOOST_CHECK(b[1]);
  BOOST_CHECK(b[2]);
  BOOST_CHECK(!c[0]);
  BOOST_CHECK(!c[1]);
  BOOST_CHECK(c[2]);
}

BOOST_AUTO_TEST_SUITE_END()

#endif

