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

#include "DPORDriver.h"
#include "DPORDriver_test.h"
#include "StrModule.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(Regression_test)

BOOST_AUTO_TEST_CASE(Sigma_6){
  /* This is a regression test for "TSOTraceBuilder: Fix merging different sizes in WuT"
   *
   */
  Configuration conf = DPORDriver_test::get_sc_conf();
  conf.dpor_algorithm = Configuration::OPTIMAL;
  std::string module = StrModule::portasm(R"(
%attr_t = type { i64, [48 x i8] }

@array = global [6 x i32] zeroinitializer
@array_index = global i32 0

define i8* @thread(i8*) {
  %2 = load i32, i32* @array_index, align 4
  %3 = sext i32 %2 to i64
  %4 = getelementptr inbounds [6 x i32], [6 x i32]* @array, i64 0, i64 %3
  store i32 1, i32* %4, align 4
  ret i8* null
}

define i32 @main(i32, i8**) {
start:
  %pids = alloca [6 x i64], align 16
  store i32 -1, i32* @array_index
  br label %create_loop

create_loop:
  %ci = phi i64 [ 0, %start ], [ %cip1, %create_loop ]
  %ai = load i32, i32* @array_index, align 4
  %aip1 = add i32 %ai, 1
  store i32 %aip1, i32* @array_index, align 4
  %cpidp = getelementptr inbounds [6 x i64], [6 x i64]* %pids, i64 0, i64 %ci
  call i32 @pthread_create(i64* %cpidp, %attr_t* null, i8* (i8*)* @thread, i8* null)
  %cip1 = add i64 %ci, 1
  %ccond = icmp eq i64 %cip1, 6
  br i1 %ccond, label %join_loop, label %create_loop

end:
  ret i32 0

join_loop:
  %ji = phi i64 [ %jip1, %join_loop ], [ 0, %create_loop ]
  %jpidp = getelementptr inbounds [6 x i64], [6 x i64]* %pids, i64 0, i64 %ji
  %jpid = load i64, i64* %jpidp, align 8
  call i32 @pthread_join(i64 %jpid, i8** null)
  %jip1 = add i64 %ji, 1
  %jcond = icmp eq i64 %jip1, 6
  br i1 %jcond, label %end, label %join_loop
}

declare i32 @pthread_create(i64*, %attr_t*, i8* (i8*)*, i8*)
declare i32 @pthread_join(i64, i8**)
)");

  std::unique_ptr<DPORDriver> driver(DPORDriver::parseIR(module,conf));
  DPORDriver::Result res = driver->run();

  /* No trace_set_spec is provided, as it is too big to express in C++ */
  BOOST_CHECK(res.trace_count == 10395);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Eratosthenes4){
  /* Regression test for "TSOTraceBuilder: Fix obs_sleep_wake edge case" */
  Configuration conf = DPORDriver_test::get_sc_conf();
  conf.dpor_algorithm = Configuration::OPTIMAL;
  conf.observers = true;
  std::string module = StrModule::portasm(R"(
%attr_t = type { i64, [48 x i8] }

@naturals = global [19 x i32] zeroinitializer, align 16

define i8* @eratosthenes(i8*) {
  br label %2

  %3 = phi i64 [ 2, %1 ], [ %16, %15 ]
  %4 = phi i64 [ 4, %1 ], [ %17, %15 ]
  %5 = getelementptr [19 x i32], [19 x i32]* @naturals, i64 0, i64 %3
  %6 = load volatile i32, i32* %5
  %7 = icmp eq i32 %6, 0
  %8 = icmp ult i64 %3, 10
  %9 = and i1 %7, %8
  br i1 %9, label %10, label %15

  %11 = phi i64 [ %13, %10 ], [ %4, %2 ]
  %12 = getelementptr [19 x i32], [19 x i32]* @naturals, i64 0, i64 %11
  store volatile i32 2, i32* %12
  %13 = add i64 %11, %3
  %14 = icmp ult i64 %13, 19
  br i1 %14, label %10, label %15

  %16 = add i64 %3, 1
  %17 = add i64 %4, 2
  %18 = icmp eq i64 %16, 19
  br i1 %18, label %19, label %2

  ret i8* null
}

define i32 @main(i32, i8**) {
  %3 = alloca i64, align 8
  %4 = alloca i64, align 8
  call i32 @pthread_create(i64* %3, %attr_t* null, i8* (i8*)* @eratosthenes, i8* null)
  call i32 @pthread_create(i64* %4, %attr_t* null, i8* (i8*)* @eratosthenes, i8* null)
  ret i32 0
}

declare i32 @pthread_create(i64*, %attr_t*, i8* (i8*)*, i8*)
)");

  std::unique_ptr<DPORDriver> driver(DPORDriver::parseIR(module,conf));
  DPORDriver::Result res = driver->run();

  /* No trace_set_spec is provided, as it is too big to express in C++ */
  BOOST_CHECK(res.trace_count == 12074);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_SUITE_END()

#endif

