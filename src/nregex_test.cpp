/* Copyright (C) 2016-2017 Carl Leonardsson
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

#include "nregex.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(nregex_test)

BOOST_AUTO_TEST_CASE(No_match_1){
  BOOST_CHECK_EQUAL(nregex::regex_replace("abc","def","ghi"), "abc");
}

BOOST_AUTO_TEST_CASE(Replace_1){
  BOOST_CHECK_EQUAL(nregex::regex_replace("foobar","o*b","OOB"), "fOOBar");
}

BOOST_AUTO_TEST_CASE(Replace_2){
  BOOST_CHECK_EQUAL(nregex::regex_replace("foobar","o","(o)"), "f(o)(o)bar");
}

BOOST_AUTO_TEST_CASE(Replace_3){
  BOOST_CHECK_EQUAL(nregex::regex_replace("foobar","","()"), "()f()o()o()b()a()r()");
}

BOOST_AUTO_TEST_CASE(Replace_4){
  BOOST_CHECK_EQUAL(nregex::regex_replace("foobar","o*","(o)"), "(o)f(o)b(o)a(o)r(o)");
}

BOOST_AUTO_TEST_CASE(Replace_5){
  BOOST_CHECK_EQUAL(nregex::regex_replace("foobar","o+","(o)"), "f(o)bar");
}

BOOST_AUTO_TEST_CASE(Replace_6){
  BOOST_CHECK_EQUAL(nregex::regex_replace("","","A"), "A");
}

BOOST_AUTO_TEST_CASE(No_match_2){
  BOOST_CHECK_EQUAL(nregex::regex_replace("","a","A"), "");
}

BOOST_AUTO_TEST_CASE(Replace_7){
  BOOST_CHECK_EQUAL(nregex::regex_replace("","",""), "");
}

BOOST_AUTO_TEST_CASE(Replace_fmt_1){
  BOOST_CHECK_EQUAL(nregex::regex_replace("foobar","(o|a)","$1$1"),"foooobaar");
}

BOOST_AUTO_TEST_CASE(Replace_fmt_2){
  BOOST_CHECK_EQUAL(nregex::regex_replace("foObar","(o|a)(o|O|a)?","$2$1$2"),"fOoOb$2a$2r");
}

BOOST_AUTO_TEST_CASE(Replace_fmt_3){
  BOOST_CHECK_EQUAL(nregex::regex_replace("foobar","(o*)","($1)"),"()f(oo)b()a()r()");
}

BOOST_AUTO_TEST_SUITE_END()

#endif

