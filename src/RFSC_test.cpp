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

#ifdef HAVE_VALGRIND_VALGRIND_H
#include <valgrind/valgrind.h>
#endif
#include "DPORDriver.h"
#include "DPORDriver_test.h"
#include "StrModule.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(RFSC_test)

BOOST_AUTO_TEST_CASE(Condvars_not_supported) {
  Configuration conf = DPORDriver_test::get_sc_conf();
  conf.dpor_algorithm = Configuration::READS_FROM;
  std::string module = StrModule::portasm(R"(
@cnd = global i32 0, align 4

define i32 @main(){
  call i32 @pthread_cond_init(i32* @cnd, i32* null)

  ret i32 0
}
declare i32 @pthread_cond_init(i32*,i32*) nounwind
)");

  std::unique_ptr<DPORDriver> driver(DPORDriver::parseIR(module, conf));
  DPORDriver::Result res = driver->run();

  BOOST_REQUIRE(res.has_errors());
  BOOST_REQUIRE_EQUAL(res.error_trace->get_errors().size(), 1);
  BOOST_REQUIRE_NE(res.error_trace->get_errors()[0]->to_string().find
                   ("not supported:") , std::string::npos);
}

BOOST_AUTO_TEST_CASE(Threads_prune) {
  Configuration conf = DPORDriver_test::get_sc_conf();
  conf.dpor_algorithm = Configuration::READS_FROM;
  conf.n_threads = 4;
  conf.explore_all_traces = false;
  std::string module = StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p(i8* %arg){
  %old = load atomic i32, i32* @x seq_cst, align 4
  %inc = add i32 %old, 1
  store atomic i32 %inc, i32* @x seq_cst, align 4

  %old.1 = load atomic i32, i32* @x seq_cst, align 4
  %inc.1 = add i32 %old.1, 1
  store atomic i32 %inc.1, i32* @x seq_cst, align 4

  %old.2 = load atomic i32, i32* @x seq_cst, align 4
  %inc.2 = add i32 %old.2, 1
  store atomic i32 %inc.2, i32* @x seq_cst, align 4

  %bad = icmp sgt i32 %inc.2, 7
  br i1 %bad, label %fail, label %succ

fail:
  call void @__assert_fail()
  unreachable

succ:
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  ret i32 0
}
%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare void @__assert_fail() nounwind noreturn
)");

  std::unique_ptr<DPORDriver> driver(DPORDriver::parseIR(module, conf));
  DPORDriver::Result res = driver->run();

  BOOST_REQUIRE(res.has_errors());
  BOOST_REQUIRE_EQUAL(res.error_trace->get_errors().size(), 1);
}

BOOST_AUTO_TEST_SUITE_END()

#endif
