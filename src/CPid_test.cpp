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

#include "CPid.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(CPid_test)

BOOST_AUTO_TEST_CASE(Spawn_Aux){
  CPid p0;
  BOOST_CHECK_EQUAL(p0.spawn(1),CPid({0,1}));
  BOOST_CHECK_EQUAL(p0.spawn(2).spawn(3),CPid({0,2,3}));
  BOOST_CHECK_EQUAL(p0.spawn(4).spawn(0).aux(3),CPid({0,4,0},3));
  BOOST_CHECK_EQUAL(p0.aux(0),CPid({0},0));
}

BOOST_AUTO_TEST_CASE(CPid_parent){
  BOOST_CHECK_EQUAL(CPid({0,1,2}).parent(),CPid({0,1}));
  BOOST_CHECK_EQUAL(CPid({0,1}).aux(2).parent(),CPid({0,1}));
}

BOOST_AUTO_TEST_CASE(CPidSystem_spawn){
  CPidSystem CPS;
  BOOST_CHECK_EQUAL(CPS.spawn(CPid({0})),CPid({0,0}));
  BOOST_CHECK_EQUAL(CPS.spawn(CPid({0})),CPid({0,1}));
  BOOST_CHECK_EQUAL(CPS.spawn(CPid({0})),CPid({0,2}));
  BOOST_CHECK_EQUAL(CPS.spawn(CPid({0,1})),CPid({0,1,0}));
  BOOST_CHECK_EQUAL(CPS.spawn(CPid({0,1})),CPid({0,1,1}));
  BOOST_CHECK_EQUAL(CPS.spawn(CPid({0,1,1})),CPid({0,1,1,0}));
}

BOOST_AUTO_TEST_CASE(CPidSystem_new_aux){
  CPidSystem CPS;
  BOOST_CHECK_EQUAL(CPS.new_aux(CPid({0})),CPid({0},0));
  BOOST_CHECK_EQUAL(CPS.new_aux(CPid({0})),CPid({0},1));
  BOOST_CHECK_EQUAL(CPS.new_aux(CPid({0})),CPid({0},2));
  BOOST_CHECK_EQUAL(CPS.spawn(CPid({0})),CPid({0,0}));
  BOOST_CHECK_EQUAL(CPS.new_aux(CPid({0,0})),CPid({0,0},0));
  BOOST_CHECK_EQUAL(CPS.spawn(CPid({0})),CPid({0,1}));
  BOOST_CHECK_EQUAL(CPS.new_aux(CPid({0,0})),CPid({0,0},1));
  BOOST_CHECK_EQUAL(CPS.new_aux(CPid({0,1})),CPid({0,1},0));
}

BOOST_AUTO_TEST_CASE(CPidSystem_copy){
  CPidSystem CPS0, CPS1;
  /* Setup CPS0 */
  BOOST_CHECK_EQUAL(CPS0.spawn(CPid({0})),CPid({0,0}));
  BOOST_CHECK_EQUAL(CPS0.spawn(CPid({0})),CPid({0,1}));
  BOOST_CHECK_EQUAL(CPS0.spawn(CPid({0,0})),CPid({0,0,0}));
  BOOST_CHECK_EQUAL(CPS0.new_aux(CPid({0,1})),CPid({0,1},0));
  /* Setup CPS1 */
  BOOST_CHECK_EQUAL(CPS1.spawn(CPid({0})),CPid({0,0}));
  BOOST_CHECK_EQUAL(CPS1.new_aux(CPid({0})),CPid({0},0));
  /* Copy */
  CPS0 = CPS1;
  /* Check CPS0 after copy */
  BOOST_CHECK_EQUAL(CPS0.spawn(CPid({0})),CPid({0,1}));
  BOOST_CHECK_EQUAL(CPS0.spawn(CPid({0,0})),CPid({0,0,0}));
  BOOST_CHECK_EQUAL(CPS0.new_aux(CPid({0})),CPid({0},1));
  BOOST_CHECK_EQUAL(CPS0.new_aux(CPid({0,1})),CPid({0,1},0));
  BOOST_CHECK_EQUAL(CPS0.new_aux(CPid({0,0})),CPid({0,0},0));
}

BOOST_AUTO_TEST_CASE(CPidSystem_copy_construct){
  CPidSystem CPS1;
  /* Setup CPS1 */
  BOOST_CHECK_EQUAL(CPS1.spawn(CPid({0})),CPid({0,0}));
  BOOST_CHECK_EQUAL(CPS1.new_aux(CPid({0})),CPid({0},0));
  /* Copy construct */
  CPidSystem CPS0(CPS1);
  /* Check CPS0 after copy */
  BOOST_CHECK_EQUAL(CPS0.spawn(CPid({0})),CPid({0,1}));
  BOOST_CHECK_EQUAL(CPS0.spawn(CPid({0,0})),CPid({0,0,0}));
  BOOST_CHECK_EQUAL(CPS0.new_aux(CPid({0})),CPid({0},1));
  BOOST_CHECK_EQUAL(CPS0.new_aux(CPid({0,1})),CPid({0,1},0));
  BOOST_CHECK_EQUAL(CPS0.new_aux(CPid({0,0})),CPid({0,0},0));
}

BOOST_AUTO_TEST_SUITE_END()

#endif
