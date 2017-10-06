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

#ifdef HAVE_VALGRIND_VALGRIND_H
#include <valgrind/valgrind.h>
#endif
#include "DPORDriver.h"
#include "DPORDriver_test.h"
#include "StrModule.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(SC_test)

BOOST_AUTO_TEST_CASE(Thread_local_1){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(R"(
@x = thread_local global i32 0, align 4

define i8* @w(i8* %arg){
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @w, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @w, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @w, i8* null)
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)",conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 1); // No conflicts on @x
}

BOOST_AUTO_TEST_CASE(Thread_local_2){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = thread_local global i32 0, align 4
@p = global i32* null, align 8

define i8* @w(i8* %arg){
  store i32* @x, i32** @p, align 8
  store i32 1, i32* @x, align 4
  %p = load i32*, i32** @p, align 8
  load i32, i32* %p, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @w, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @w, i8* null)
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0, P1 = P0.spawn(0), P2 = P0.spawn(1);
  IID<CPid> wp1(P1,1), wx1(P1,2), rp1(P1,3), rpx1(P1,4),
    wp2(P2,1), wx2(P2,2), rp2(P2,3), rpx2(P2,4);
  DPORDriver_test::trace_set_spec expected =
    {{{wp1,wp2},{rp1,wp2}},
     {{wp1,wp2},{wp2,rp1},{rpx1,wx2}},
     {{wp1,wp2},{wp2,rp1},{wx2,rpx1}},
     {{wp2,wp1},{rp2,wp1}},
     {{wp2,wp1},{wp1,rp2},{rpx2,wx1}},
     {{wp2,wp1},{wp1,rp2},{wx1,rpx2}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Thread_local_3){ // Initialization
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = thread_local global i32 666, align 4

define i8* @w(i8* %arg){
  %x = load i32, i32* @x, align 4
  %c = icmp eq i32 %x, 666
  br i1 %c, label %exit, label %error
error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @w, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @w, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @w, i8* null)
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare void @__assert_fail()
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(CAS_load){
  /* Check that a failing CAS does not have a conflict with loads. */
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @r(i8* %arg){
  load i32, i32* @x, align 4
  ret i8* null
}

define i8* @w(i8* %arg){
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @r, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @w, i8* null)
)"
#if defined(LLVM_CMPXCHG_SEPARATE_SUCCESS_FAILURE_ORDERING)
R"(  cmpxchg i32* @x, i32 1, i32 2 seq_cst seq_cst)"
#else
R"(  cmpxchg i32* @x, i32 1, i32 2 seq_cst)"
#endif
R"(
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0, P1 = P0.spawn(0), P2 = P0.spawn(1);
  IID<CPid> cas(P0,3), r(P1,1), w(P2,1);
  DPORDriver_test::trace_set_spec expected =
    {{{cas,w},{r,w}},
     {{cas,w},{w,r}},
     {{w,cas},{r,w}},
     {{w,cas},{w,r},{r,cas}},
     {{w,cas},{cas,r}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Compiler_fence_dekker){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 1, i32* @y, align 4
  call void asm sideeffect "", "~{memory}"()
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  store i32 1, i32* @x, align 4
  call void asm sideeffect "", "~{memory}"()
  load i32, i32* @y, align 4
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();

  BOOST_CHECK(res.trace_count == 3);

  delete driver;
}

BOOST_AUTO_TEST_CASE(Intrinsic_return){
  /* Regression test.
   *
   * Nidhugg would segmentation fault when analyzing this
   * sample. (Similarly under TSO, PSO, POWER and ARM.)
   *
   * The bug was caused by Execution not handling llvm.dbg.declare
   * correctly: When a thread first visits a llvm.dbg.declare
   * statement, the statement is removed from its basic
   * block. Therefore if any other thread's program counter (the
   * iterator ExecutionContext::CurInst) points into that basic block,
   * the iterator will become invalid. For this reason, the thread
   * that removes the llvm.dbg.declare needs to check all other
   * program counters and restore them if necessary. This was already
   * properly done for the program pointer of the top-most stack frame
   * of each other thread. However, program pointers in lower stack
   * frames were erroneously neglected and left in an invalid state.
   *
   * This test is designed to trigger that error. Both threads P0 and
   * P1 enter function f. From f they enter function g. Then one of
   * the threads is delayed inside g (a schedule which delays the
   * thread is triggered by having races in g). The other thread exits
   * g and visits the llvm.dbg.declare statement which immediately
   * follows the call to g from f. This will invalidate the return
   * address in the second to top stack frame for the other thread,
   * since it is pointing to that very statement.
   *
   * A complication is that in order to trigger the error, the above
   * described situation must happen in the first run after the test
   * module has been reparsed by DPORDriver. (Otherwise the
   * llvm.dbg.declare statements are no longer present in the module.)
   * Reparse happens (at the time of writing) every 1000 runs. This
   * test has 7 stores to x in g, which means that the situation is
   * caused to appear in (7*2)!/(7!*7!) = 3432 consecutive runs (save
   * the first and last run).
   */
  Configuration conf = DPORDriver_test::get_sc_conf();

#ifdef LLVM_METADATA_IS_VALUE
  std::string declarecall = "call void @llvm.dbg.declare(metadata !{i32 %id}, metadata !0)";
  std::string declaredeclare = R"(
declare void @llvm.dbg.declare(metadata, metadata) nounwind readnone
!llvm.module.flags = !{!1}
!0 = metadata !{i32 0}
!1 = metadata !{i32 2, metadata !"Debug Info Version", i32 )" LLVM_METADATA_VERSION_NUMBER_STR R"(}
)";
#else
#ifdef LLVM_DBG_DECLARE_TWO_ARGS
  std::string declarecall = "call void @llvm.dbg.declare(metadata i32 %id, metadata !0)";
  std::string declaredeclare = R"(
declare void @llvm.dbg.declare(metadata, metadata) nounwind readnone
!llvm.module.flags = !{!1}
!0 = !{i32 0}
!1 = !{i32 2, !"Debug Info Version", i32 )" LLVM_METADATA_VERSION_NUMBER_STR R"(}
)";
#else
  std::string declarecall = "call void @llvm.dbg.declare(metadata i32 %id, metadata !0, metadata !0)";
  std::string declaredeclare = R"(
declare void @llvm.dbg.declare(metadata, metadata, metadata) nounwind readnone
!llvm.module.flags = !{!1}
!0 = !{i32 0}
!1 = !{i32 2, !"Debug Info Version", i32 )" LLVM_METADATA_VERSION_NUMBER_STR R"(}
)";
#endif
#endif

  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define void @g(i32 %id){
  store i32 1, i32* @x, align 4
  store i32 1, i32* @x, align 4
  store i32 1, i32* @x, align 4
  store i32 1, i32* @x, align 4
  store i32 1, i32* @x, align 4
  store i32 1, i32* @x, align 4
  store i32 1, i32* @x, align 4
  ret void
}

define void @f(i32 %id){
  call void @g(i32 %id)
  )"+declarecall+R"(
  ret void
}

define i8* @p1(i8* %arg){
  call void @f(i32 1)
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call void @f(i32 0)
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)"+declaredeclare),conf);

  DPORDriver::Result res = driver->run();

  // No test. We only check for crashes.

  delete driver;
}

BOOST_AUTO_TEST_SUITE_END()

#endif
