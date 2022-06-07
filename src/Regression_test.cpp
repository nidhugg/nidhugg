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
#ifdef HAVE_VALGRIND_VALGRIND_H
#  include <valgrind/valgrind.h>
#  include <cstdlib>
#endif

#ifdef HAVE_VALGRIND_VALGRIND_H
#  define TEST_CONDITION \
  (getenv("NIDHUGG_NO_SKIP_REGRESSION_TEST") || !RUNNING_ON_VALGRIND)
#else
#  define TEST_CONDITION 1
#endif
// boost::unit_test::precondition is available in 1.59, but is broken until 1.68
// (see https://svn.boost.org/trac/boost/ticket/12095)
#if BOOST_VERSION >= 106800
namespace {
  struct unless_valgrind {
    boost::test_tools::assertion_result operator()
    (boost::unit_test::test_unit_id) {
#ifdef HAVE_VALGRIND_VALGRIND_H
      return TEST_CONDITION;
#else
      return true;
#endif
    }
  };
}  // namespace
#  define TEST_SUITE_DECORATOR                          \
  , * boost::unit_test::precondition(unless_valgrind())
#  define TEST_CASE_CHECK_SKIP() ((void)0)
#else
#  define TEST_SUITE_DECORATOR
#  define TEST_CASE_CHECK_SKIP() do {           \
    if (!TEST_CONDITION) return;                \
  } while(0)
#endif

BOOST_AUTO_TEST_SUITE(Regression_test TEST_SUITE_DECORATOR)

BOOST_AUTO_TEST_CASE(Sigma_6){
  TEST_CASE_CHECK_SKIP();
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
  TEST_CASE_CHECK_SKIP();
  /* Regression test for "TSOTraceBuilder: Fix obs_sleep_wake edge case" */
  Configuration conf = DPORDriver_test::get_sc_conf();
  conf.dpor_algorithm = Configuration::OBSERVERS;
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

BOOST_AUTO_TEST_CASE(mcs_spinlock_SC_Optimal){
  TEST_CASE_CHECK_SKIP();
  /* Regression test for "Fix merging non-unity events under Optimal" */
  Configuration conf = DPORDriver_test::get_sc_conf();
  conf.dpor_algorithm = Configuration::OPTIMAL;
  std::string module = StrModule::portasm(R"(
%mcs_spinlock = type { %mcs_spinlock*, i32, i32 }
%attr_t = type { i64, [48 x i8] }

@lock = global %mcs_spinlock* null, align 8
@nodes = global [4 x %mcs_spinlock] zeroinitializer, align 16
@shared = global i32 0, align 4
@idx = global [4 x i32] zeroinitializer, align 16

define i8* @thread_n(i8* ) {
  %2 = bitcast i8* %0 to i32*
  %3 = load i32, i32* %2, align 4
  %4 = sext i32 %3 to i64
  %5 = getelementptr inbounds [4 x %mcs_spinlock], [4 x %mcs_spinlock]* @nodes, i64 0, i64 %4
  tail call void @mcs_spin_lock(%mcs_spinlock* %5)
  store i32 42, i32* @shared, align 4
  tail call void @mcs_spin_unlock(%mcs_spinlock* %5)
  ret i8* null
}

define void @mcs_spin_lock(%mcs_spinlock*) {
  %2 = getelementptr inbounds %mcs_spinlock, %mcs_spinlock* %0, i64 0, i32 1
  store atomic i32 0, i32* %2 monotonic, align 8
  %3 = bitcast %mcs_spinlock* %0 to i64*
  store atomic i64 0, i64* %3 monotonic, align 8
  %4 = ptrtoint %mcs_spinlock* %0 to i64
  %5 = atomicrmw xchg i64* bitcast (%mcs_spinlock** @lock to i64*), i64 %4 acq_rel
  %6 = icmp eq i64 %5, 0
  br i1 %6, label %12, label %7

; <label>:7:                                      ; preds = %1
  %8 = inttoptr i64 %5 to i64*
  store atomic i64 %4, i64* %8 monotonic, align 8
  br label %9

; <label>:9:                                      ; preds = %7
  %10 = load atomic i32, i32* %2 acquire, align 8
  %11 = icmp eq i32 %10, 0
  %notcond = xor i1 %11, true
  call void @__VERIFIER_assume(i1 %notcond)
  br label %12

; <label>:12:                                     ; preds = %9, %1
  ret void
}

define internal void @mcs_spin_unlock(%mcs_spinlock*) unnamed_addr {
  %2 = bitcast %mcs_spinlock* %0 to i64*
  %3 = load atomic i64, i64* %2 monotonic, align 8
  %4 = icmp eq i64 %3, 0
  br i1 %4, label %5, label %14

; <label>:5:                                      ; preds = %1
  %6 = ptrtoint %mcs_spinlock* %0 to i64
)"
#if defined(LLVM_CMPXCHG_SEPARATE_SUCCESS_FAILURE_ORDERING)
R"(
  %7 = cmpxchg i64* bitcast (%mcs_spinlock** @lock to i64*), i64 %6, i64 0 release monotonic
  %8 = extractvalue { i64, i1 } %7, 0)"
#else
R"(
  %7 = cmpxchg i64* bitcast (%mcs_spinlock** @lock to i64*), i64 %6, i64 0 release
  %8 = add i64 %7, 0)"
#endif
R"(
  %9 = inttoptr i64 %8 to %mcs_spinlock*
  %10 = icmp eq %mcs_spinlock* %9, %0
  br i1 %10, label %18, label %11

; <label>:11:                                     ; preds = %5
  %12 = load atomic i64, i64* %2 monotonic, align 8
  %13 = icmp eq i64 %12, 0
  %notcond = xor i1 %13, true
  call void @__VERIFIER_assume(i1 %notcond)
  br label %14

; <label>:14:                                     ; preds = %11, %1
  %15 = phi i64 [ %3, %1 ], [ %12, %11 ]
  %16 = inttoptr i64 %15 to %mcs_spinlock*
  %17 = getelementptr inbounds %mcs_spinlock, %mcs_spinlock* %16, i64 0, i32 1
  store atomic i32 1, i32* %17 release, align 8
  br label %18

; <label>:18:                                     ; preds = %14, %5
  ret void
}

define i32 @main() {
  %1 = alloca [4 x i64], align 16
  %2 = bitcast [4 x i64]* %1 to i8*
  br label %6

; <label>:3:                                      ; preds = %6
  %4 = icmp ult i64 %14, 4
  br i1 %4, label %16, label %5

; <label>:5:                                      ; preds = %58, %47, %36, %25, %3
  ret i32 0

; <label>:6:                                      ; preds = %0
  %7 = phi i64 [ 0, %0 ]
  %8 = getelementptr inbounds [4 x i32], [4 x i32]* @idx, i64 0, i64 %7
  %9 = trunc i64 %7 to i32
  store i32 %9, i32* %8, align 4
  %10 = getelementptr inbounds [4 x i64], [4 x i64]* %1, i64 0, i64 %7
  %11 = bitcast i32* %8 to i8*
  %12 = call i32 @pthread_create(i64* %10, %attr_t* null, i8* (i8*)* @thread_n, i8* %11)
  %13 = icmp eq i32 %12, 0
  %14 = add nuw nsw i64 %7, 1
  br i1 %13, label %3, label %15

; <label>:15:                                     ; preds = %49, %38, %27, %16, %6
  call void @abort()
  unreachable

diverge:                                          ; preds = %58, %diverge
  call void @__VERIFIER_assume(i1 false)
  br label %diverge

; <label>:16:                                     ; preds = %3
  %17 = phi i64 [ %14, %3 ]
  %18 = getelementptr inbounds [4 x i32], [4 x i32]* @idx, i64 0, i64 %17
  %19 = trunc i64 %17 to i32
  store i32 %19, i32* %18, align 4
  %20 = getelementptr inbounds [4 x i64], [4 x i64]* %1, i64 0, i64 %17
  %21 = bitcast i32* %18 to i8*
  %22 = call i32 @pthread_create(i64* %20, %attr_t* null, i8* (i8*)* @thread_n, i8* %21)
  %23 = icmp eq i32 %22, 0
  %24 = add nuw nsw i64 %17, 1
  br i1 %23, label %25, label %15

; <label>:25:                                     ; preds = %16
  %26 = icmp ult i64 %24, 4
  br i1 %26, label %27, label %5

; <label>:27:                                     ; preds = %25
  %28 = phi i64 [ %24, %25 ]
  %29 = getelementptr inbounds [4 x i32], [4 x i32]* @idx, i64 0, i64 %28
  %30 = trunc i64 %28 to i32
  store i32 %30, i32* %29, align 4
  %31 = getelementptr inbounds [4 x i64], [4 x i64]* %1, i64 0, i64 %28
  %32 = bitcast i32* %29 to i8*
  %33 = call i32 @pthread_create(i64* %31, %attr_t* null, i8* (i8*)* @thread_n, i8* %32)
  %34 = icmp eq i32 %33, 0
  %35 = add nuw nsw i64 %28, 1
  br i1 %34, label %36, label %15

; <label>:36:                                     ; preds = %27
  %37 = icmp ult i64 %35, 4
  br i1 %37, label %38, label %5

; <label>:38:                                     ; preds = %36
  %39 = phi i64 [ %35, %36 ]
  %40 = getelementptr inbounds [4 x i32], [4 x i32]* @idx, i64 0, i64 %39
  %41 = trunc i64 %39 to i32
  store i32 %41, i32* %40, align 4
  %42 = getelementptr inbounds [4 x i64], [4 x i64]* %1, i64 0, i64 %39
  %43 = bitcast i32* %40 to i8*
  %44 = call i32 @pthread_create(i64* %42, %attr_t* null, i8* (i8*)* @thread_n, i8* %43)
  %45 = icmp eq i32 %44, 0
  %46 = add nuw nsw i64 %39, 1
  br i1 %45, label %47, label %15

; <label>:47:                                     ; preds = %38
  %48 = icmp ult i64 %46, 4
  br i1 %48, label %49, label %5

; <label>:49:                                     ; preds = %47
  %50 = phi i64 [ %46, %47 ]
  %51 = getelementptr inbounds [4 x i32], [4 x i32]* @idx, i64 0, i64 %50
  %52 = trunc i64 %50 to i32
  store i32 %52, i32* %51, align 4
  %53 = getelementptr inbounds [4 x i64], [4 x i64]* %1, i64 0, i64 %50
  %54 = bitcast i32* %51 to i8*
  %55 = call i32 @pthread_create(i64* %53, %attr_t* null, i8* (i8*)* @thread_n, i8* %54)
  %56 = icmp eq i32 %55, 0
  %57 = add nuw nsw i64 %50, 1
  br i1 %56, label %58, label %15

; <label>:58:                                     ; preds = %49
  %59 = icmp ult i64 %57, 4
  br i1 %59, label %diverge, label %5
}

declare i32 @pthread_create(i64*, %attr_t*, i8* (i8*)*, i8*)
declare void @abort()
declare void @__VERIFIER_assume(i1)
)");

  std::unique_ptr<DPORDriver> driver(DPORDriver::parseIR(module,conf));
  DPORDriver::Result res = driver->run();

  /* No trace_set_spec is provided, as it is too big to express in C++ */
  BOOST_CHECK(!res.has_errors());
  BOOST_CHECK_EQUAL(res.trace_count, 1080);
  BOOST_CHECK_EQUAL(res.assume_blocked_trace_count, 1944);
}

BOOST_AUTO_TEST_SUITE_END()

#endif
