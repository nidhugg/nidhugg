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

#ifdef HAVE_VALGRIND_VALGRIND_H
#include <valgrind/valgrind.h>
#endif
#include "Debug.h"
#include "DPORDriver.h"
#include "DPORDriver_test.h"
#include "StrModule.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(SC_test)

BOOST_AUTO_TEST_CASE(fib_simple_n2_sc){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@i = global i32 1, align 4
@j = global i32 1, align 4

define i8* @p1(i8* %arg){
  %1 = load i32, i32* @i, align 4
  %2 = load i32, i32* @j, align 4
  %3 = add i32 %1, %2
  store i32 %3, i32* @i, align 4
  %4 = load i32, i32* @i, align 4
  %5 = load i32, i32* @j, align 4
  %6 = add i32 %4, %5
  store i32 %6, i32* @i, align 4
  ret i8* null
}

define i8* @p2(i8* %arg){
  %1 = load i32, i32* @i, align 4
  %2 = load i32, i32* @j, align 4
  %3 = add i32 %1, %2
  store i32 %3, i32* @j, align 4
  %4 = load i32, i32* @i, align 4
  %5 = load i32, i32* @j, align 4
  %6 = add i32 %4, %5
  store i32 %6, i32* @j, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p2, i8* null)

  ;load i32, i32* @i, align 4
  ;load i32, i32* @j, align 4
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0;
  CPid P1 = P0.spawn(0);
  CPid P2 = P0.spawn(1);
  IID<CPid>
    JA(P1,2), IA(P1,4), JB(P1,6), IB(P1,8),
    ia(P2,1), ja(P2,4), ib(P2,5), jb(P2,8);
  DPORDriver_test::trace_set_spec expected =
    {{{IB,ia},{JB,ja}},                                 // 01: JIJIijij
     {{IA,ia},{ia,IB},{IB,ib},{JB,ja}},                 // 02: JIJiIjij
     {{IA,ia},{JB,ja},{ib,IB}},                         // 03: JIJijiIj
     {{IA,ia},{ja,JB},{IB,ib}},                         // 04: JIijJIij
     {{IA,ia},{ja,JB},{ib,IB},{JB,jb}},                 // 05: JIijJiIj
     {{IA,ia},{jb,JB}},                                 // 06: JIijijJI
     {{ia,IA},{JB,ja},{IB,ib}},                         // 07: JiIJIjij
     {{ia,IA},{JB,ja},{ib,IB}},                         // 08: JiIJjiIj
     {{ia,IA},{JA,ja},{ja,JB},{IB,ib}},                 // 09: JiIjJIij
     {{ia,IA},{JA,ja},{ja,JB},{IA,ib},{ib,IB},{JB,jb}}, // 10: JiIjJiIj
     {{ia,IA},{JA,ja},{IA,ib},{jb,JB}},                 // 11: JiIjijJI
     {{JA,ja},{ib,IA},{JB,jb}},                         // 12: JijiIJIj
     {{JA,ja},{ib,IA},{jb,JB}},                         // 13: JijiIjJI
     {{ja,JA},{IB,ib}},                                 // 14: ijJIJIij
     {{ja,JA},{IA,ib},{ib,IB},{JB,jb}},                 // 15: ijJIJiIj
     {{ja,JA},{IA,ib},{jb,JB}},                         // 16: ijJIijJI
     {{ja,JA},{ib,IA},{JB,jb}},                         // 17: ijJiIJIj
     {{ja,JA},{ib,IA},{JA,jb},{jb,JB}},                 // 18: ijJiIjJI
     {{jb,JA}}                                          // 19: ijijJIJI
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Intrinsic_2){
  /* Regression test: This test identifies an error where unknown
   * intrinsic functions would interfere with dryruns in
   * TSOInterpreter::run. The error would only occur when the
   * intrinsic functions reappear in the module as the module is
   * reparsed by DPORDriver. This happens every 1000 runs. The error
   * would cause crashes.
   */
  Configuration conf = DPORDriver_test::get_sc_conf();
#ifdef LLVM_METADATA_IS_VALUE
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p(i8* %arg){
  call void @llvm.dbg.declare(metadata !{i8* %arg}, metadata !0)
  store i32 1, i32* @x, align 4
  call void @llvm.dbg.declare(metadata !{i8* %arg}, metadata !0)
  store i32 2, i32* @x, align 4
  call void @llvm.dbg.declare(metadata !{i8* %arg}, metadata !0)
  store i32 3, i32* @x, align 4
  call void @llvm.dbg.declare(metadata !{i8* %arg}, metadata !0)
  ret i8* null
}

define i32 @main(){
  %T0 = alloca i64
  %T1 = alloca i64
  call i32 @pthread_create(i64* %T0, %attr_t* null, i8*(i8*)*@p, i8*null)
  call i32 @pthread_create(i64* %T1, %attr_t* null, i8*(i8*)*@p, i8*null)
  call i8* @p(i8* null)
  %T0val = load i64, i64* %T0
  %T1val = load i64, i64* %T1
  call i32 @pthread_join(i64 %T0val,i8** null)
  call i32 @pthread_join(i64 %T1val,i8** null)
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare void @llvm.dbg.declare(metadata, metadata) nounwind readnone
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_join(i64,i8**) nounwind

!llvm.module.flags = !{!1}
!0 = metadata !{i32 0}
!1 = metadata !{i32 2, metadata !"Debug Info Version", i32 )" LLVM_METADATA_VERSION_NUMBER_STR R"(}
)"),conf);
#else
#ifdef LLVM_DBG_DECLARE_TWO_ARGS
  std::string declarecall = "call void @llvm.dbg.declare(metadata i8* %arg, metadata !0)";
  std::string declaredeclare = "declare void @llvm.dbg.declare(metadata, metadata) nounwind readnone";
#else
  std::string declarecall = "call void @llvm.dbg.declare(metadata i8* %arg, metadata !0, metadata !0)";
  std::string declaredeclare = "declare void @llvm.dbg.declare(metadata, metadata, metadata) nounwind readnone";
#endif
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p(i8* %arg){
  )"+declarecall+R"(
  store i32 1, i32* @x, align 4
  )"+declarecall+R"(
  store i32 2, i32* @x, align 4
  )"+declarecall+R"(
  store i32 3, i32* @x, align 4
  )"+declarecall+R"(
  ret i8* null
}

define i32 @main(){
  %T0 = alloca i64
  %T1 = alloca i64
  call i32 @pthread_create(i64* %T0, %attr_t* null, i8*(i8*)*@p, i8*null)
  call i32 @pthread_create(i64* %T1, %attr_t* null, i8*(i8*)*@p, i8*null)
  call i8* @p(i8* null)
  %T0val = load i64, i64* %T0
  %T1val = load i64, i64* %T1
  call i32 @pthread_join(i64 %T0val,i8** null)
  call i32 @pthread_join(i64 %T1val,i8** null)
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
)"+declaredeclare+R"(
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_join(i64,i8**) nounwind

!llvm.module.flags = !{!1}
!0 = !{i32 0}
!1 = !{i32 2, !"Debug Info Version", i32 )" LLVM_METADATA_VERSION_NUMBER_STR R"(}
)"),conf);
#endif
  DPORDriver::Result res = driver->run();
  delete driver;

  /* Number of mazurkiewicz traces: (3*3)!/(3!^3) = 1680 */
  BOOST_CHECK(res.trace_count == 1680);
}

BOOST_AUTO_TEST_CASE(Atomic_1){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  %x = load i32, i32* @x, align 4
  %xcmp = icmp eq i32 %x, 1
  br i1 %xcmp, label %error, label %exit
error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i8* null
}

define void @__VERIFIER_atomic_updx(){
  store i32 1, i32* @x, align 4
  store i32 0, i32* @x, align 4
  ret void
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call void @__VERIFIER_atomic_updx()
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare void @__assert_fail()
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0; CPid P1 = P0.spawn(0);
  IID<CPid> ux0(P0,2), rx1(P1,1);
  DPORDriver_test::trace_set_spec expected =
    {{{ux0,rx1}},{{rx1,ux0}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Atomic_3){
  /* Multiple different memory locations */
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  load i32, i32* @x, align 4
  load i32, i32* @y, align 4
  ret i8* null
}

define void @__VERIFIER_atomic_updx(){
  store i32 1, i32* @x, align 4
  store i32 1, i32* @y, align 4
  ret void
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call void @__VERIFIER_atomic_updx()
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare void @__assert_fail()
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0; CPid P1 = P0.spawn(0);
  IID<CPid> uxy0(P0,2), rx1(P1,1), ry1(P1,2);
  DPORDriver_test::trace_set_spec expected =
    {{{uxy0,rx1}},{{rx1,uxy0},{uxy0,ry1}},{{ry1,uxy0}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Atomic_5){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define void @__VERIFIER_atomic_wx(){
  store i32 2, i32* @x, align 4
  ret void
}

define i8* @p(i8* %arg){
  %foo = alloca i32, align 4
  store i32 0, i32* %foo, align 4
  call void @__VERIFIER_atomic_wx()
  load i32, i32* @x
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call i8* @p(i8* null)
  ret i32 0
}

%attr_t = type{i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
declare void @__VERIFIER_assume(i32) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0; CPid P1 = P0.spawn(0);
  IID<CPid> wx0(P0,5), rx0(P0,6),
    wx1(P1,3), rx1(P1,4);
  DPORDriver_test::trace_set_spec expected =
    {{{wx0,wx1},{rx0,wx1}},
     {{wx0,wx1},{wx1,rx0}},
     {{wx1,wx0},{rx1,wx0}},
     {{wx1,wx0},{wx0,rx1}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Atomic_6){
  /* Blocking call inside atomic function */
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define void @__VERIFIER_atomic_block(){
  call void @__VERIFIER_assume(i32 0)
  ret void
}

define void @p3(){
  store i32 1, i32* @x, align 4
  store i32 2, i32* @x, align 4
  ret void
}

define void @p2(){
  call void @p3()
  ret void
}

define i8* @p(i8* %arg){
  call void @p2()
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@p, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@p, i8* null)
  call void @__VERIFIER_atomic_block()
  ret i32 0
}

%attr_t = type{i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
declare void @__VERIFIER_assume(i32)
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0; CPid P1 = P0.spawn(0); CPid P2 = P0.spawn(1);
  IID<CPid>
    wx1a(P1,3), wx1b(P1,4),
    wx2a(P2,3), wx2b(P2,4);
  DPORDriver_test::trace_set_spec expected =
    {{{wx1b,wx2a}},
     {{wx1a,wx2a},{wx2a,wx1b},{wx1b,wx2b}},
     {{wx1a,wx2a},{wx2b,wx1b}},
     {{wx2a,wx1a},{wx1b,wx2b}},
     {{wx2a,wx1a},{wx1a,wx2b},{wx2b,wx1b}},
     {{wx2b,wx1a}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Atomic_7){
  /* Blocking call inside atomic function */
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define void @__VERIFIER_atomic_block(){
  %x = load i32, i32* @x, align 4
  call void @__VERIFIER_assume(i32 %x)
  ret void
}

define i8* @p(i8* %arg){
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@p, i8* null)
  call void @__VERIFIER_atomic_block()
  ret i32 0
}

%attr_t = type{i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
declare void @__VERIFIER_assume(i32)
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0; CPid P1 = P0.spawn(0); CPid P2 = P0.spawn(1);
  IID<CPid> rx0(P0,2), wx1(P1,1);
  DPORDriver_test::trace_set_spec expected =
    {{{wx1,rx0}},{{rx0,wx1}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Atomic_8){
  /* Read depending on read depending on store, all inside atomic
   * function. */
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define void @__VERIFIER_atomic_f(){
  load i32, i32* @x, align 4
  %p = alloca i32*, align 8
  store i32* @x, i32** %p, align 8
  %addr = load i32*, i32** %p, align 8
  load i32, i32* %addr, align 4
  ret void
}

define i8* @p1(i8* %arg){
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call void @__VERIFIER_atomic_f()
  ret i32 0
}

%attr_t = type{i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0; CPid P1 = P0.spawn(0);
  IID<CPid> x0(P0,2), x1(P1,1);
  DPORDriver_test::trace_set_spec expected =
    {{{x0,x1}},{{x1,x0}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Atomic_9){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define void @__VERIFIER_atomic_foox(){
  %y = alloca i32, align 4
  %z = alloca i32, align 4
  store i32 1, i32* %y, align 4
  store i32 1, i32* %z, align 4
  store i32 2, i32* @x, align 4
  ret void
}

define i8* @p(i8* %arg){
  %u = alloca i32, align 4
  store i32 1, i32* %u, align 4
  call void @__VERIFIER_atomic_foox()
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  ret i32 0
}

%attr_t = type{i64,[48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
  /* Number of traces:
   * 6!/((2!)^3) = 90
   *
   * 3 threads, each executing two stores to x.
   */
  BOOST_CHECK(res.trace_count == 90);
}

BOOST_AUTO_TEST_CASE(Full_conflict_1){
  /* This test assumes that memcpy has full memory conflict. If that
   * is no longer the case, replace the call to memcpy with any
   * function that does have a full memory conflict.
   */
  Configuration conf = DPORDriver_test::get_sc_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@p1, i8* null)
  call i8* @memcpy(i8* null, i8* null, i32 0)
  ret i32 0
}

%attr_t = type { i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
declare i8* @memcpy(i8*, i8*, i32) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0; CPid P1 = P0.spawn(0);
  IID<CPid> r1(P1,1), mc0(P0,2);
  DPORDriver_test::trace_set_spec expected =
    {{{r1,mc0}},{{mc0,r1}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Full_conflict_2){
  /* This test assumes that memcpy has full memory conflict. If that
   * is no longer the case, replace the call to memcpy with any
   * function that does have a full memory conflict.
   */
  Configuration conf = DPORDriver_test::get_sc_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  call i8* @memcpy(i8* null, i8* null, i32 0)
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@p1, i8* null)
  load i32, i32* @x, align 4
  ret i32 0
}

%attr_t = type { i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
declare i8* @memcpy(i8*, i8*, i32) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0; CPid P1 = P0.spawn(0);
  IID<CPid> r0(P0,2), mc1(P1,1);
  DPORDriver_test::trace_set_spec expected =
    {{{r0,mc1}},{{mc1,r0}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Full_conflict_3){
  /* This test assumes that memcpy has full memory conflict. If that
   * is no longer the case, replace the call to memcpy with any
   * function that does have a full memory conflict.
   */
  Configuration conf = DPORDriver_test::get_sc_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  call i8* @memcpy(i8* null, i8* null, i32 0)
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@p1, i8* null)
  call i8* @memcpy(i8* null, i8* null, i32 0)
  ret i32 0
}

%attr_t = type { i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
declare i8* @memcpy(i8*, i8*, i32) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0; CPid P1 = P0.spawn(0);
  IID<CPid> mc0(P0,2), mc1(P1,1);
  DPORDriver_test::trace_set_spec expected =
    {{{mc0,mc1}},{{mc1,mc0}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Full_conflict_4){
  /* This test assumes that memcpy has full memory conflict. If that
   * is no longer the case, replace the call to memcpy with any
   * function that does have a full memory conflict.
   */
  Configuration conf = DPORDriver_test::get_sc_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 1, i32* @x, align 4
  store i32 1, i32* @x, align 4
  call i8* @memcpy(i8* null, i8* null, i32 0)
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@p1, i8* null)
  call i8* @memcpy(i8* null, i8* null, i32 0)
  store i32 1, i32* @y, align 4
  ret i32 0
}

%attr_t = type { i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
declare i8* @memcpy(i8*, i8*, i32) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0; CPid P1 = P0.spawn(0);
  IID<CPid> p1wx1(P1,1), p1wx2(P1,2), p1mc(P1,3),
    p0mc(P0,2), p0wy(P0,3);
  DPORDriver_test::trace_set_spec expected =
    {
      {{p0mc,p1wx1},{p1mc,p0wy}},
      {{p0mc,p1wx1},{p0wy,p1mc}},
      {{p1wx1,p0mc},{p0mc,p1wx2},{p1mc,p0wy}},
      {{p1wx1,p0mc},{p0mc,p1wx2},{p0wy,p1mc}},
      {{p1wx2,p0mc},{p0mc,p1mc},{p1mc,p0wy}},
      {{p1wx2,p0mc},{p0mc,p1mc},{p0wy,p1mc}},
      {{p1mc,p0mc}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Full_conflict_6){
  /* This test assumes that memcpy has full memory conflict. If that
   * is no longer the case, replace the call to memcpy with any
   * function that does have a full memory conflict.
   */
  Configuration conf = DPORDriver_test::get_sc_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  call i8* @memcpy(i8* null, i8* null, i32 0)
  store i32 1, i32* @x, align 4
  store i32 1, i32* @y, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@p1, i8* null)
  load i32, i32* @x
  ret i32 0
}

%attr_t = type { i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
declare i8* @memcpy(i8*, i8*, i32) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0; CPid P1 = P0.spawn(0);
  IID<CPid> p1mc(P1,1), p1wx(P1,2), p0rx(P0,2);
  DPORDriver_test::trace_set_spec expected =
    {
      {{p0rx,p1mc}},
      {{p1mc,p0rx},{p0rx,p1wx}},
      {{p1wx,p0rx}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Full_conflict_7){
  /* This test assumes that memcpy has full memory conflict. If that
   * is no longer the case, replace the call to memcpy with any
   * function that does have a full memory conflict.
   */
  Configuration conf = DPORDriver_test::get_sc_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  load i32, i32* @x
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@p1, i8* null)
  call i8* @memcpy(i8* null, i8* null, i32 0)
  store i32 1, i32* @x, align 4
  store i32 1, i32* @y, align 4
  ret i32 0
}

%attr_t = type { i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
declare i8* @memcpy(i8*, i8*, i32) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0; CPid P1 = P0.spawn(0);
  IID<CPid> p0mc(P0,2), p0wx(P0,3), p1rx(P1,1);
  DPORDriver_test::trace_set_spec expected =
    {
      {{p1rx,p0mc}},
      {{p0mc,p1rx},{p1rx,p0wx}},
      {{p0wx,p1rx}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Full_conflict_8){
  /* This test assumes that memcpy has full memory conflict. If that
   * is no longer the case, replace the call to memcpy with any
   * function that does have a full memory conflict.
   */
  Configuration conf = DPORDriver_test::get_sc_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4

define i8* @p1(i8* %arg){
  call i32 @pthread_mutex_lock(i32* @lck)
  call i32 @pthread_mutex_unlock(i32* @lck)
  call i8* @memcpy(i8* null, i8* null, i32 0)
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_mutex_init(i32* @lck, i32* null) nounwind
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@p1, i8* null)
  call i8* @memcpy(i8* null, i8* null, i32 0)
  call i32 @pthread_mutex_lock(i32* @lck)
  call i32 @pthread_mutex_unlock(i32* @lck)
  ret i32 0
}

%attr_t = type { i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
declare i8* @memcpy(i8*, i8*, i32) nounwind
declare i32 @pthread_mutex_init(i32*,i32*) nounwind
declare i32 @pthread_mutex_lock(i32*) nounwind
declare i32 @pthread_mutex_unlock(i32*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0; CPid P1 = P0.spawn(0);
  IID<CPid> p0mc(P0,3), p0lock(P0,4), p0unlock(P0,5),
    p1lock(P1,1), p1unlock(P1,2), p1mc(P1,3);
  DPORDriver_test::trace_set_spec expected =
    {
      {{p0unlock,p1lock}},

      {{p0mc,p1lock},{p1unlock,p0lock},{p1mc,p0lock}},
      {{p1lock,p0mc},{p0mc,p1unlock},{p1unlock,p0lock},{p1mc,p0lock}},
      {{p1unlock,p0mc},{p0mc,p1mc},{p1unlock,p0lock},{p1mc,p0lock}},

      {{p0mc,p1lock},{p1unlock,p0lock},{p0lock,p1mc},{p1mc,p0unlock}},
      {{p1lock,p0mc},{p0mc,p1unlock},{p1unlock,p0lock},{p0lock,p1mc},{p1mc,p0unlock}},
      {{p1unlock,p0mc},{p0mc,p1mc},{p1unlock,p0lock},{p0lock,p1mc},{p1mc,p0unlock}},

      {{p0mc,p1lock},{p1unlock,p0lock},{p0unlock,p1mc}},
      {{p1lock,p0mc},{p0mc,p1unlock},{p1unlock,p0lock},{p0unlock,p1mc}},
      {{p1unlock,p0mc},{p0mc,p1mc},{p1unlock,p0lock},{p0unlock,p1mc}},

      {{p1mc,p0mc},{p1unlock,p0lock}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Full_conflict_9){
  /* This test assumes that memcpy has full memory conflict. If that
   * is no longer the case, replace the call to memcpy with any
   * function that does have a full memory conflict.
   */
  Configuration conf = DPORDriver_test::get_sc_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4

define i8* @p1(i8* %arg){
  call i8* @memcpy(i8* null, i8* null, i32 0)
  call i32 @pthread_mutex_lock(i32* @lck)
  call i32 @pthread_mutex_unlock(i32* @lck)
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_mutex_init(i32* @lck, i32* null) nounwind
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@p1, i8* null)
  call i32 @pthread_mutex_lock(i32* @lck)
  call i32 @pthread_mutex_unlock(i32* @lck)
  call i8* @memcpy(i8* null, i8* null, i32 0)
  ret i32 0
}

%attr_t = type { i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
declare i8* @memcpy(i8*, i8*, i32) nounwind
declare i32 @pthread_mutex_init(i32*,i32*) nounwind
declare i32 @pthread_mutex_lock(i32*) nounwind
declare i32 @pthread_mutex_unlock(i32*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0; CPid P1 = P0.spawn(0);
  IID<CPid> p0mc(P0,5), p0lock(P0,3), p0unlock(P0,4),
    p1lock(P1,2), p1unlock(P1,3), p1mc(P1,1);
  DPORDriver_test::trace_set_spec expected =
    {
      {{p1unlock,p0lock}},

      {{p1mc,p0lock},{p0unlock,p1lock},{p0mc,p1lock}},
      {{p0lock,p1mc},{p1mc,p0unlock},{p0unlock,p1lock},{p0mc,p1lock}},
      {{p0unlock,p1mc},{p1mc,p0mc},{p0unlock,p1lock},{p0mc,p1lock}},

      {{p1mc,p0lock},{p0unlock,p1lock},{p1lock,p0mc},{p0mc,p1unlock}},
      {{p0lock,p1mc},{p1mc,p0unlock},{p0unlock,p1lock},{p1lock,p0mc},{p0mc,p1unlock}},
      {{p0unlock,p1mc},{p1mc,p0mc},{p0unlock,p1lock},{p1lock,p0mc},{p0mc,p1unlock}},

      {{p1mc,p0lock},{p0unlock,p1lock},{p1unlock,p0mc}},
      {{p0lock,p1mc},{p1mc,p0unlock},{p0unlock,p1lock},{p1unlock,p0mc}},
      {{p0unlock,p1mc},{p1mc,p0mc},{p0unlock,p1lock},{p1unlock,p0mc}},

      {{p0mc,p1mc},{p0unlock,p1lock}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Full_conflict_10){
  /* This test assumes that memcpy has full memory conflict. If that
   * is no longer the case, replace the call to memcpy with any
   * function that does have a full memory conflict.
   */
  Configuration conf = DPORDriver_test::get_sc_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4

define i8* @p1(i8* %arg){
  call i32 @pthread_mutex_lock(i32* @lck)
  call i8* @memcpy(i8* null, i8* null, i32 0)
  call i32 @pthread_mutex_unlock(i32* @lck)
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_mutex_init(i32* @lck, i32* null) nounwind
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@p1, i8* null)
  call i8* @memcpy(i8* null, i8* null, i32 0)
  call i32 @pthread_mutex_lock(i32* @lck)
  call i32 @pthread_mutex_unlock(i32* @lck)
  ret i32 0
}

%attr_t = type { i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
declare i8* @memcpy(i8*, i8*, i32) nounwind
declare i32 @pthread_mutex_init(i32*,i32*) nounwind
declare i32 @pthread_mutex_lock(i32*) nounwind
declare i32 @pthread_mutex_unlock(i32*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0; CPid P1 = P0.spawn(0);
  IID<CPid> p0mc(P0,3), p0lock(P0,4), p0unlock(P0,5),
    p1lock(P1,1), p1unlock(P1,3), p1mc(P1,2);
  DPORDriver_test::trace_set_spec expected =
    {
      {{p0unlock,p1lock}},

      {{p0mc,p1lock},{p1unlock,p0lock}},
      {{p1lock,p0mc},{p0mc,p1mc},{p1unlock,p0lock}},
      {{p1mc,p0mc},{p0mc,p1unlock},{p1unlock,p0lock}},
      {{p1unlock,p0mc},{p1unlock,p0lock}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Intrinsic_first){
  Configuration conf = DPORDriver_test::get_sc_conf();
#ifdef LLVM_METADATA_IS_VALUE
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p0(i8* %arg){
  tail call void @llvm.dbg.value(metadata !{i8* %arg}, i64 0, metadata !1)
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@p0, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@p0, i8* null)
  ret i32 0
}

declare void @llvm.dbg.value(metadata, i64, metadata) #1
%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind

!llvm.module.flags = !{!2}
!1 = metadata !{i32 0}
!2 = metadata !{i32 2, metadata !"Debug Info Version", i32 )" LLVM_METADATA_VERSION_NUMBER_STR R"(}
)"),conf);
#else
#ifdef LLVM_DBG_DECLARE_TWO_ARGS
  std::string declarecall = "call void @llvm.dbg.declare(metadata i8* %arg, metadata !1)";
  std::string declaredeclare = "declare void @llvm.dbg.declare(metadata, metadata) nounwind readnone";
#else
  std::string declarecall = "call void @llvm.dbg.declare(metadata i8* %arg, metadata !1, metadata !1)";
  std::string declaredeclare = "declare void @llvm.dbg.declare(metadata, metadata, metadata) nounwind readnone";
#endif
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p0(i8* %arg){
  )" + declarecall + R"(
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@p0, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@p0, i8* null)
  ret i32 0
}

)" + declaredeclare + R"(
%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind

!llvm.module.flags = !{!2}
!1 = !{i32 0}
!2 = !{i32 2, !"Debug Info Version", i32 )" LLVM_METADATA_VERSION_NUMBER_STR R"(}
)"),conf);
#endif

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0, P1 = P0.spawn(0), P2 = P0.spawn(1);
  IID<CPid> wx1(P1,1), wx2(P2,1);
  DPORDriver_test::trace_set_spec expected =
    {{{wx1,wx2}},{{wx2,wx1}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Mutex_trylock_1){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4
@x = global i32 0, align 4

define i8* @p(i8* %arg){
  %lckret = call i32 @pthread_mutex_trylock(i32* @lck)
  %lcksuc = icmp eq i32 %lckret, 0
  br i1 %lcksuc, label %CS, label %exit
CS:
  store i32 1, i32* @x, align 4
  store i32 1, i32* @x, align 4
  store i32 1, i32* @x, align 4
  store i32 1, i32* @x, align 4
  call i32 @pthread_mutex_unlock(i32* @lck)
  br label %exit
exit:
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_mutex_init(i32* @lck, i32* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call i8* @p(i8* null)
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_mutex_init(i32*,i32*) nounwind
declare i32 @pthread_mutex_trylock(i32*) nounwind
declare i32 @pthread_mutex_unlock(i32*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0, P1 = P0.spawn(0);
  IID<CPid> lck0(P0,4), ulck0(P0,11), lck1(P1,1), ulck1(P1,8);
  DPORDriver_test::trace_set_spec expected =
    {{{ulck0,lck1}}, // P0 entirely before P1
     {{lck0,lck1},{lck1,ulck0}}, // P0 first, P1 fails at trylock
     {{ulck1,lck0}}, // P1 entirely before P0
     {{lck1,lck0},{lck0,ulck1}} // P1 first, P0 fails at trylock
    };
  BOOST_CHECK(!res.has_errors());
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Condvar_1){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4
@cnd = global i32 0, align 4
@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  call i32 @pthread_mutex_lock(i32* @lck)
  %x0 = load i32, i32* @x, align 4
  %xcmp0 = icmp ne i32 %x0, 0
  br i1 %xcmp0, label %check, label %wait
wait:
  call i32 @pthread_cond_wait(i32* @cnd, i32* @lck)
  br label %check
check:
  %x1 = load i32, i32* @x, align 4
  %xcmp1 = icmp ne i32 %x1, 0
  br i1 %xcmp1, label %done, label %error
error:
  call void @__assert_fail()
  br label %done
done:
  call i32 @pthread_mutex_unlock(i32* @lck)
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_mutex_init(i32* @lck, i32* null)
  call i32 @pthread_cond_init(i32* @cnd, i32* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)

  call i32 @pthread_mutex_lock(i32* @lck)
  store i32 1, i32* @x, align 4
  call i32 @pthread_cond_broadcast(i32* @cnd)
  call i32 @pthread_mutex_unlock(i32* @lck)

  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_mutex_init(i32*,i32*) nounwind
declare i32 @pthread_mutex_lock(i32*) nounwind
declare i32 @pthread_mutex_unlock(i32*) nounwind
declare i32 @pthread_cond_init(i32*,i32*) nounwind
declare i32 @pthread_cond_broadcast(i32*) nounwind
declare i32 @pthread_cond_wait(i32*,i32*) nounwind
declare void @__assert_fail()
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0, P1 = P0.spawn(0);
  IID<CPid> lck0(P0,4), ulck0(P0,7),
    lck1(P1,1), cndwt(P1,5), lck1b(P1,6);
  DPORDriver_test::trace_set_spec expected =
    {{{ulck0,lck1}}, // No cond wait
     {{cndwt,lck0},{ulck0,lck1b}} // Cond wait
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Condvar_2){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4
@cnd = global i32 0, align 4
@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  call i32 @pthread_cond_wait(i32* @cnd, i32* @lck)
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_mutex_init(i32* @lck, i32* null)
  call i32 @pthread_cond_init(i32* @cnd, i32* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)

  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_mutex_init(i32*,i32*) nounwind
declare i32 @pthread_mutex_lock(i32*) nounwind
declare i32 @pthread_mutex_unlock(i32*) nounwind
declare i32 @pthread_cond_init(i32*,i32*) nounwind
declare i32 @pthread_cond_broadcast(i32*) nounwind
declare i32 @pthread_cond_wait(i32*,i32*) nounwind
declare void @__assert_fail()
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(Condvar_3){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4
@cnd = global i32 0, align 4
@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  call i32 @pthread_mutex_lock(i32* @lck)
  call i32 @pthread_cond_wait(i32* @cnd, i32* @lck)
  call i32 @pthread_mutex_unlock(i32* @lck)
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_mutex_init(i32* @lck, i32* null)
  call i32 @pthread_cond_init(i32* @cnd, i32* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)

  call i32 @pthread_cond_broadcast(i32* @cnd)

  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_mutex_init(i32*,i32*) nounwind
declare i32 @pthread_mutex_lock(i32*) nounwind
declare i32 @pthread_mutex_unlock(i32*) nounwind
declare i32 @pthread_cond_init(i32*,i32*) nounwind
declare i32 @pthread_cond_broadcast(i32*) nounwind
declare i32 @pthread_cond_wait(i32*,i32*) nounwind
declare void @__assert_fail()
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0, P1 = P0.spawn(0);
  IID<CPid> bcast(P0,4), cndwt(P1,2), ulck1(P1,3);
  DPORDriver_test::trace_set_spec expected =
    {{{bcast,cndwt}},
     {{cndwt,bcast},{bcast,ulck1}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Condvar_4){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4
@cnd = global i32 0, align 4
@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  call i32 @pthread_cond_broadcast(i32* @cnd)
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_mutex_init(i32* @lck, i32* null)
  call i32 @pthread_cond_init(i32* @cnd, i32* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)

  call i32 @pthread_mutex_lock(i32* @lck)
  call i32 @pthread_cond_wait(i32* @cnd, i32* @lck)
  call i32 @pthread_mutex_unlock(i32* @lck)

  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_mutex_init(i32*,i32*) nounwind
declare i32 @pthread_mutex_lock(i32*) nounwind
declare i32 @pthread_mutex_unlock(i32*) nounwind
declare i32 @pthread_cond_init(i32*,i32*) nounwind
declare i32 @pthread_cond_broadcast(i32*) nounwind
declare i32 @pthread_cond_wait(i32*,i32*) nounwind
declare void @__assert_fail()
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0, P1 = P0.spawn(0);
  IID<CPid> bcast(P1,1), cndwt(P0,5), ulck0(P0,6);
  DPORDriver_test::trace_set_spec expected =
    {{{bcast,cndwt}},
     {{cndwt,bcast},{bcast,ulck0}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Condvar_5){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4
@cnd = global i32 0, align 4
@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  call i32 @pthread_mutex_lock(i32* @lck)
  store i32 1, i32* @x, align 4
  call i32 @pthread_cond_broadcast(i32* @cnd)
  call i32 @pthread_mutex_unlock(i32* @lck)
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_mutex_init(i32* @lck, i32* null)
  call i32 @pthread_cond_init(i32* @cnd, i32* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)

  call i32 @pthread_mutex_lock(i32* @lck)
  %x0 = load i32, i32* @x, align 4
  %xcmp0 = icmp ne i32 %x0, 0
  br i1 %xcmp0, label %check, label %wait
wait:
  call i32 @pthread_cond_wait(i32* @cnd, i32* @lck)
  br label %check
check:
  %x1 = load i32, i32* @x, align 4
  %xcmp1 = icmp ne i32 %x1, 0
  br i1 %xcmp1, label %done, label %error
error:
  call void @__assert_fail()
  br label %done
done:
  call i32 @pthread_mutex_unlock(i32* @lck)

  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_mutex_init(i32*,i32*) nounwind
declare i32 @pthread_mutex_lock(i32*) nounwind
declare i32 @pthread_mutex_unlock(i32*) nounwind
declare i32 @pthread_cond_init(i32*,i32*) nounwind
declare i32 @pthread_cond_broadcast(i32*) nounwind
declare i32 @pthread_cond_wait(i32*,i32*) nounwind
declare void @__assert_fail()
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0, P1 = P0.spawn(0);
  IID<CPid> lck0(P1,1), ulck0(P1,4),
    lck1(P0,4), cndwt(P0,8), lck1b(P0,9);
  DPORDriver_test::trace_set_spec expected =
    {{{ulck0,lck1}}, // No cond wait
     {{cndwt,lck0},{ulck0,lck1b}} // Cond wait
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Condvar_6){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4
@cnd = global i32 0, align 4
@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  call i32 @pthread_mutex_lock(i32* @lck)
  store i32 1, i32* @x, align 4
  call i32 @pthread_cond_broadcast(i32* @cnd)
  call i32 @pthread_mutex_unlock(i32* @lck)
  ret i8* null
}

define i8* @pwait(i8* %arg){
  call i32 @pthread_mutex_lock(i32* @lck)
  %x0 = load i32, i32* @x, align 4
  %xcmp0 = icmp ne i32 %x0, 0
  br i1 %xcmp0, label %check, label %wait
wait:
  call i32 @pthread_cond_wait(i32* @cnd, i32* @lck)
  br label %check
check:
  %x1 = load i32, i32* @x, align 4
  %xcmp1 = icmp ne i32 %x1, 0
  br i1 %xcmp1, label %done, label %error
error:
  call void @__assert_fail()
  br label %done
done:
  call i32 @pthread_mutex_unlock(i32* @lck)
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_mutex_init(i32* @lck, i32* null)
  call i32 @pthread_cond_init(i32* @cnd, i32* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @pwait, i8* null)

  call i8* @pwait(i8* null)

  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_mutex_init(i32*,i32*) nounwind
declare i32 @pthread_mutex_lock(i32*) nounwind
declare i32 @pthread_mutex_unlock(i32*) nounwind
declare i32 @pthread_cond_init(i32*,i32*) nounwind
declare i32 @pthread_cond_broadcast(i32*) nounwind
declare i32 @pthread_cond_wait(i32*,i32*) nounwind
declare void @__assert_fail()
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0, P1 = P0.spawn(0), P2 = P0.spawn(1);
  IID<CPid> lckB(P1,1), ulckB(P1,4),
    lckW0(P0,6), cndwtW0(P0,10), lckW0b(P0,11), ulckW0a(P0,13), ulckW0b(P0,15),
    lckW1(P2,1), cndwtW1(P2,5), lckW1b(P2,6), ulckW1a(P2,8), ulckW1b(P2,10);
  DPORDriver_test::trace_set_spec expected =
    {{{ulckB,lckW0},{ulckW0a,lckW1}}, // No wait, W0 before W1
     {{ulckB,lckW1},{ulckW1a,lckW0}}, // No wait, W1 before W0
     {{cndwtW0,lckB},{ulckB,lckW0b},{ulckW0b,lckW1}}, // W0 waits, not W1, W0 before W1
     {{cndwtW0,lckB},{ulckB,lckW1},{ulckW1a,lckW0b}}, // W0 waits, not W1, W1 before W0
     {{cndwtW1,lckB},{ulckB,lckW1b},{ulckW1b,lckW0}}, // W1 waits, not W0, W1 before W0
     {{cndwtW1,lckB},{ulckB,lckW0},{ulckW0a,lckW1b}}, // W1 waits, not W0, W0 before W1
     {{cndwtW0,lckW1},{cndwtW1,lckB},{ulckB,lckW0b},{ulckW0b,lckW1b}}, // Both wait, W0 before W1, W0 before W1
     {{cndwtW0,lckW1},{cndwtW1,lckB},{ulckB,lckW1b},{ulckW1b,lckW0b}}, // Both wait, W0 before W1, W1 before W0
     {{cndwtW1,lckW0},{cndwtW0,lckB},{ulckB,lckW0b},{ulckW0b,lckW1b}}, // Both wait, W1 before W0, W0 before W1
     {{cndwtW1,lckW0},{cndwtW0,lckB},{ulckB,lckW1b},{ulckW1b,lckW0b}}  // Both wait, W1 before W0, W1 before W0
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Condvar_7){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4
@cnd = global i32 0, align 4
@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  call i32 @pthread_mutex_lock(i32* @lck)
  %x0 = load i32, i32* @x, align 4
  %xcmp0 = icmp ne i32 %x0, 0
  br i1 %xcmp0, label %check, label %wait
wait:
  call i32 @pthread_cond_wait(i32* @cnd, i32* @lck)
  br label %check
check:
  %x1 = load i32, i32* @x, align 4
  %xcmp1 = icmp ne i32 %x1, 0
  br i1 %xcmp1, label %done, label %error
error:
  call void @__assert_fail()
  br label %done
done:
  call i32 @pthread_mutex_unlock(i32* @lck)
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_mutex_init(i32* @lck, i32* null)
  call i32 @pthread_cond_init(i32* @cnd, i32* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)

  call i32 @pthread_mutex_lock(i32* @lck)
  store i32 1, i32* @x, align 4
  call i32 @pthread_cond_signal(i32* @cnd)
  call i32 @pthread_mutex_unlock(i32* @lck)

  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_mutex_init(i32*,i32*) nounwind
declare i32 @pthread_mutex_lock(i32*) nounwind
declare i32 @pthread_mutex_unlock(i32*) nounwind
declare i32 @pthread_cond_init(i32*,i32*) nounwind
declare i32 @pthread_cond_signal(i32*) nounwind
declare i32 @pthread_cond_wait(i32*,i32*) nounwind
declare void @__assert_fail()
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0, P1 = P0.spawn(0);
  IID<CPid> lck0(P0,4), ulck0(P0,7),
    lck1(P1,1), cndwt(P1,5), lck1b(P1,6);
  DPORDriver_test::trace_set_spec expected =
    {{{ulck0,lck1}}, // No cond wait
     {{cndwt,lck0},{ulck0,lck1b}} // Cond wait
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Condvar_8){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4
@cnd = global i32 0, align 4
@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  call i32 @pthread_mutex_lock(i32* @lck)
  call i32 @pthread_cond_wait(i32* @cnd, i32* @lck)
  call i32 @pthread_mutex_unlock(i32* @lck)
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_mutex_init(i32* @lck, i32* null)
  call i32 @pthread_cond_init(i32* @cnd, i32* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)

  call i32 @pthread_cond_signal(i32* @cnd)

  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_mutex_init(i32*,i32*) nounwind
declare i32 @pthread_mutex_lock(i32*) nounwind
declare i32 @pthread_mutex_unlock(i32*) nounwind
declare i32 @pthread_cond_init(i32*,i32*) nounwind
declare i32 @pthread_cond_signal(i32*) nounwind
declare i32 @pthread_cond_wait(i32*,i32*) nounwind
declare void @__assert_fail()
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0, P1 = P0.spawn(0);
  IID<CPid> bcast(P0,4), cndwt(P1,2), ulck1(P1,3);
  DPORDriver_test::trace_set_spec expected =
    {{{bcast,cndwt}},
     {{cndwt,bcast},{bcast,ulck1}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Condvar_9){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4
@cnd = global i32 0, align 4
@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  call i32 @pthread_cond_signal(i32* @cnd)
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_mutex_init(i32* @lck, i32* null)
  call i32 @pthread_cond_init(i32* @cnd, i32* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)

  call i32 @pthread_mutex_lock(i32* @lck)
  call i32 @pthread_cond_wait(i32* @cnd, i32* @lck)
  call i32 @pthread_mutex_unlock(i32* @lck)

  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_mutex_init(i32*,i32*) nounwind
declare i32 @pthread_mutex_lock(i32*) nounwind
declare i32 @pthread_mutex_unlock(i32*) nounwind
declare i32 @pthread_cond_init(i32*,i32*) nounwind
declare i32 @pthread_cond_signal(i32*) nounwind
declare i32 @pthread_cond_wait(i32*,i32*) nounwind
declare void @__assert_fail()
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0, P1 = P0.spawn(0);
  IID<CPid> bcast(P1,1), cndwt(P0,5), ulck0(P0,6);
  DPORDriver_test::trace_set_spec expected =
    {{{bcast,cndwt}},
     {{cndwt,bcast},{bcast,ulck0}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Condvar_10){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4
@cnd = global i32 0, align 4
@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  call i32 @pthread_mutex_lock(i32* @lck)
  store i32 1, i32* @x, align 4
  call i32 @pthread_cond_signal(i32* @cnd)
  call i32 @pthread_mutex_unlock(i32* @lck)
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_mutex_init(i32* @lck, i32* null)
  call i32 @pthread_cond_init(i32* @cnd, i32* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)

  call i32 @pthread_mutex_lock(i32* @lck)
  %x0 = load i32, i32* @x, align 4
  %xcmp0 = icmp ne i32 %x0, 0
  br i1 %xcmp0, label %check, label %wait
wait:
  call i32 @pthread_cond_wait(i32* @cnd, i32* @lck)
  br label %check
check:
  %x1 = load i32, i32* @x, align 4
  %xcmp1 = icmp ne i32 %x1, 0
  br i1 %xcmp1, label %done, label %error
error:
  call void @__assert_fail()
  br label %done
done:
  call i32 @pthread_mutex_unlock(i32* @lck)

  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_mutex_init(i32*,i32*) nounwind
declare i32 @pthread_mutex_lock(i32*) nounwind
declare i32 @pthread_mutex_unlock(i32*) nounwind
declare i32 @pthread_cond_init(i32*,i32*) nounwind
declare i32 @pthread_cond_signal(i32*) nounwind
declare i32 @pthread_cond_wait(i32*,i32*) nounwind
declare void @__assert_fail()
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0, P1 = P0.spawn(0);
  IID<CPid> lck0(P1,1), ulck0(P1,4),
    lck1(P0,4), cndwt(P0,8), lck1b(P0,9);
  DPORDriver_test::trace_set_spec expected =
    {{{ulck0,lck1}}, // No cond wait
     {{cndwt,lck0},{ulck0,lck1b}} // Cond wait
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Condvar_11){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4
@cnd = global i32 0, align 4
@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  call i32 @pthread_mutex_lock(i32* @lck)
  store i32 1, i32* @x, align 4
  call i32 @pthread_cond_signal(i32* @cnd)
  call i32 @pthread_mutex_unlock(i32* @lck)
  ret i8* null
}

define i8* @pwait(i8* %arg){
  call i32 @pthread_mutex_lock(i32* @lck)
  %x0 = load i32, i32* @x, align 4
  %xcmp0 = icmp ne i32 %x0, 0
  br i1 %xcmp0, label %check, label %wait
wait:
  call i32 @pthread_cond_wait(i32* @cnd, i32* @lck)
  br label %check
check:
  %x1 = load i32, i32* @x, align 4
  %xcmp1 = icmp ne i32 %x1, 0
  br i1 %xcmp1, label %done, label %error
error:
  call void @__assert_fail()
  br label %done
done:
  call i32 @pthread_mutex_unlock(i32* @lck)
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_mutex_init(i32* @lck, i32* null)
  call i32 @pthread_cond_init(i32* @cnd, i32* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @pwait, i8* null)

  call i8* @pwait(i8* null)

  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_mutex_init(i32*,i32*) nounwind
declare i32 @pthread_mutex_lock(i32*) nounwind
declare i32 @pthread_mutex_unlock(i32*) nounwind
declare i32 @pthread_cond_init(i32*,i32*) nounwind
declare i32 @pthread_cond_signal(i32*) nounwind
declare i32 @pthread_cond_wait(i32*,i32*) nounwind
declare void @__assert_fail()
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0, P1 = P0.spawn(0), P2 = P0.spawn(1);
  IID<CPid> lckB(P1,1), ulckB(P1,4),
    lckW0(P0,6), cndwtW0(P0,10), lckW0b(P0,11), ulckW0a(P0,13), ulckW0b(P0,15),
    lckW1(P2,1), cndwtW1(P2,5), lckW1b(P2,6), ulckW1a(P2,8), ulckW1b(P2,10);
  DPORDriver_test::trace_set_spec expected =
    {{{ulckB,lckW0},{ulckW0a,lckW1}}, // No wait, W0 before W1
     {{ulckB,lckW1},{ulckW1a,lckW0}}, // No wait, W1 before W0
     {{cndwtW0,lckB},{ulckB,lckW0b},{ulckW0b,lckW1}}, // W0 waits, not W1, W0 before W1
     {{cndwtW0,lckB},{ulckB,lckW1},{ulckW1a,lckW0b}}, // W0 waits, not W1, W1 before W0
     {{cndwtW1,lckB},{ulckB,lckW1b},{ulckW1b,lckW0}}, // W1 waits, not W0, W1 before W0
     {{cndwtW1,lckB},{ulckB,lckW0},{ulckW0a,lckW1b}}, // W1 waits, not W0, W0 before W1
     {{cndwtW0,lckW1},{cndwtW1,lckB},{ulckB,lckW0b}}, // Both wait, W0 before W1, W0 wakes up
     {{cndwtW0,lckW1},{cndwtW1,lckB},{ulckB,lckW1b}}, // Both wait, W0 before W1, W1 wakes up
     {{cndwtW1,lckW0},{cndwtW0,lckB},{ulckB,lckW0b}}, // Both wait, W1 before W0, W0 wakes up
     {{cndwtW1,lckW0},{cndwtW0,lckB},{ulckB,lckW1b}}  // Both wait, W1 before W0, W1 wakes up
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Condvar_12){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4
@cnd = global i32 0, align 4
@x = global i32 0, align 4
@t = global i64 0, align 8

define i8* @p1(i8* %arg){
  call i32 @pthread_mutex_lock(i32* @lck)
  %x0 = load i32, i32* @x, align 4
  %xcmp0 = icmp ne i32 %x0, 0
  br i1 %xcmp0, label %check, label %wait
wait:
  call i32 @pthread_cond_wait(i32* @cnd, i32* @lck)
  br label %check
check:
  %x1 = load i32, i32* @x, align 4
  %xcmp1 = icmp ne i32 %x1, 0
  br i1 %xcmp1, label %done, label %error
error:
  call void @__assert_fail()
  br label %done
done:
  call i32 @pthread_mutex_unlock(i32* @lck)
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_mutex_init(i32* @lck, i32* null)
  call i32 @pthread_cond_init(i32* @cnd, i32* null)
  call i32 @pthread_create(i64* @t, %attr_t* null, i8*(i8*)* @p1, i8* null)

  call i32 @pthread_mutex_lock(i32* @lck)
  store i32 1, i32* @x, align 4
  call i32 @pthread_cond_broadcast(i32* @cnd)
  call i32 @pthread_mutex_unlock(i32* @lck)

  %t = load i64, i64* @t, align 8
  call i32 @pthread_join(i64 %t, i8** null)

  %drv = call i32 @pthread_cond_destroy(i32* @cnd)
  %drvcmp = icmp ne i32 %drv, 0
  br i1 %drvcmp, label %error, label %exit
error:
  call void @__assert_fail()
  br label %exit
exit:
  call i32 @pthread_cond_init(i32* @cnd, i32* null)
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_join(i64,i8**) nounwind
declare i32 @pthread_mutex_init(i32*,i32*) nounwind
declare i32 @pthread_mutex_lock(i32*) nounwind
declare i32 @pthread_mutex_unlock(i32*) nounwind
declare i32 @pthread_cond_init(i32*,i32*) nounwind
declare i32 @pthread_cond_broadcast(i32*) nounwind
declare i32 @pthread_cond_wait(i32*,i32*) nounwind
declare i32 @pthread_cond_destroy(i32*) nounwind
declare void @__assert_fail()
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0, P1 = P0.spawn(0);
  IID<CPid> lck0(P0,4), ulck0(P0,7),
    lck1(P1,1), cndwt(P1,5), lck1b(P1,6);
  DPORDriver_test::trace_set_spec expected =
    {{{ulck0,lck1}}, // No cond wait
     {{cndwt,lck0},{ulck0,lck1b}} // Cond wait
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Condvar_13){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4
@cnd = global i32 0, align 4
@x = global i32 0, align 4
@t = global i64 0, align 8

define i8* @p1(i8* %arg){
  call i32 @pthread_mutex_lock(i32* @lck)
  %x0 = load i32, i32* @x, align 4
  %xcmp0 = icmp ne i32 %x0, 0
  br i1 %xcmp0, label %check, label %wait
wait:
  call i32 @pthread_cond_wait(i32* @cnd, i32* @lck)
  br label %check
check:
  %x1 = load i32, i32* @x, align 4
  %xcmp1 = icmp ne i32 %x1, 0
  br i1 %xcmp1, label %done, label %error
error:
  call void @__assert_fail()
  br label %done
done:
  call i32 @pthread_mutex_unlock(i32* @lck)
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_mutex_init(i32* @lck, i32* null)
  call i32 @pthread_cond_init(i32* @cnd, i32* null)
  call i32 @pthread_create(i64* @t, %attr_t* null, i8*(i8*)* @p1, i8* null)

  call i32 @pthread_mutex_lock(i32* @lck)
  store i32 1, i32* @x, align 4
  call i32 @pthread_mutex_unlock(i32* @lck)

  %drv = call i32 @pthread_cond_destroy(i32* @cnd)
  %drvcmp = icmp ne i32 %drv, 0
  br i1 %drvcmp, label %error, label %exit
error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_join(i64,i8**) nounwind
declare i32 @pthread_mutex_init(i32*,i32*) nounwind
declare i32 @pthread_mutex_lock(i32*) nounwind
declare i32 @pthread_mutex_unlock(i32*) nounwind
declare i32 @pthread_cond_init(i32*,i32*) nounwind
declare i32 @pthread_cond_broadcast(i32*) nounwind
declare i32 @pthread_cond_wait(i32*,i32*) nounwind
declare i32 @pthread_cond_destroy(i32*) nounwind
declare void @__assert_fail()
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(Condvar_14){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4
@cnd = global i32 0, align 4
@x = global i32 0, align 4
@t = global i64 0, align 8

define i8* @p1(i8* %arg){
  call i32 @pthread_mutex_lock(i32* @lck)
  %x0 = load i32, i32* @x, align 4
  %xcmp0 = icmp ne i32 %x0, 0
  br i1 %xcmp0, label %check, label %wait
wait:
  call i32 @pthread_cond_wait(i32* @cnd, i32* @lck)
  br label %check
check:
  %x1 = load i32, i32* @x, align 4
  %xcmp1 = icmp ne i32 %x1, 0
  br i1 %xcmp1, label %done, label %error
error:
  call void @__assert_fail()
  br label %done
done:
  call i32 @pthread_mutex_unlock(i32* @lck)
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_mutex_init(i32* @lck, i32* null)
  call i32 @pthread_cond_init(i32* @cnd, i32* null)
  call i32 @pthread_create(i64* @t, %attr_t* null, i8*(i8*)* @p1, i8* null)

  %drv = call i32 @pthread_cond_destroy(i32* @cnd)
  %drvcmp = icmp ne i32 %drv, 0
  br i1 %drvcmp, label %error, label %exit
error:
  call void @__assert_fail()
  br label %exit
exit:
  call i32 @pthread_cond_init(i32* @cnd, i32* null)
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_join(i64,i8**) nounwind
declare i32 @pthread_mutex_init(i32*,i32*) nounwind
declare i32 @pthread_mutex_lock(i32*) nounwind
declare i32 @pthread_mutex_unlock(i32*) nounwind
declare i32 @pthread_cond_init(i32*,i32*) nounwind
declare i32 @pthread_cond_broadcast(i32*) nounwind
declare i32 @pthread_cond_wait(i32*,i32*) nounwind
declare i32 @pthread_cond_destroy(i32*) nounwind
declare void @__assert_fail()
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(Free_null_1){
  Configuration conf = DPORDriver_test::get_tso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
define i32 @main(){
  call void @free(i8* null)
  ret i32 0
}

declare i8* @malloc(i32) nounwind
declare void @free(i8*) nounwind
declare void @__assert_fail() noreturn nounwind
)"),conf);
  DPORDriver::Result res = driver->run();
  delete driver;

  /* free(NULL) is a no-op. */
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Double_free_1){
  Configuration conf = DPORDriver_test::get_tso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
define i32 @main(){
  %p = call i8* @malloc(i32 10)
  call void @free(i8* %p)
  call void @free(i8* %p)
  ret i32 0
}

declare i8* @malloc(i32) nounwind
declare void @free(i8*) nounwind
declare void @__assert_fail() noreturn nounwind
)"),conf);
  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(Free_stack_1){
  Configuration conf = DPORDriver_test::get_tso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
define i32 @main(){
  %p = alloca i8
  call void @free(i8* %p)
  ret i32 0
}

declare i8* @malloc(i32) nounwind
declare void @free(i8*) nounwind
declare void @__assert_fail() noreturn nounwind
)"),conf);
  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(Free_global_1){
#ifdef HAVE_VALGRIND_VALGRIND_H
  /* This test will work under valgrind, but will produce a spurious
   * valgrind error. The error should not be suppressed, since it is
   * used to detect the erroneous call to free. To avoid spam when
   * running the test suite under valgrind, this test is then
   * disabled.
   */
  if(!(RUNNING_ON_VALGRIND)){
#endif
    Configuration conf = DPORDriver_test::get_tso_conf();
    DPORDriver *driver =
      DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i8 0

define i32 @main(){
  call void @free(i8* @x)
  ret i32 0
}

declare i8* @malloc(i32) nounwind
declare void @free(i8*) nounwind
declare void @__assert_fail() noreturn nounwind
)"),conf);
    DPORDriver::Result res = driver->run();
    delete driver;

    BOOST_CHECK(res.has_errors());
#ifdef HAVE_VALGRIND_VALGRIND_H
  }
#endif
}

BOOST_AUTO_TEST_CASE(Memory_leak_1){
  /* Allocate memory without freeing it.
   *
   * The memory should be freed by the Interpreter after the end of
   * the execution. If not, it will be detected when running the unit
   * tests under valgrind.
   */
  Configuration conf = DPORDriver_test::get_tso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
define i32 @main(){
  call i8* @malloc(i32 10)
  ret i32 0
}

declare i8* @malloc(i32) nounwind
declare void @free(i8*) nounwind
declare void @__assert_fail() noreturn nounwind
)"),conf);
  DPORDriver::Result res = driver->run();
  delete driver;
}

BOOST_AUTO_TEST_CASE(Atexit_1){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

define void @f(){
  call void @__assert_fail()
  ret void
}

define i32 @main(){
  call i32 @atexit(void()*@f)
  ret i32 0
}

declare i32 @atexit(void()*)
declare void @__assert_fail()
)"),conf);
  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(Atexit_2){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

define void @f(){
  ret void
}

define i32 @main(){
  call i32 @atexit(void()*@f)
  ret i32 0
}

declare i32 @atexit(void()*)
declare void @__assert_fail()
)"),conf);
  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Atexit_3){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

define void @f(){
  call void @__assert_fail()
  ret void
}

define i32 @main(){
  call i32 @atexit(void()*@f)
  call void @__VERIFIER_assume(i32 0)
  ret i32 0
}

declare i32 @atexit(void()*)
declare void @__assert_fail()
declare void @__VERIFIER_assume(i32)
)"),conf);
  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Atexit_4){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0

define void @f(){
  %x = load i32, i32* @x
  %xcmp = icmp eq i32 %x, 0
  br i1 %xcmp, label %ok, label %error
error:
  call void @__assert_fail()
  ret void
ok:
  store i32 1, i32* @x
  ret void
}

define void @g(){
  %x = load i32, i32* @x
  %xcmp = icmp eq i32 %x, 1
  br i1 %xcmp, label %ok, label %error
error:
  call void @__assert_fail()
  ret void
ok:
  store i32 2, i32* @x
  ret void
}

define i32 @main(){
  call i32 @atexit(void()*@g)
  call i32 @atexit(void()*@f)
  ret i32 0
}

declare i32 @atexit(void()*)
declare void @__assert_fail()
)"),conf);
  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Atexit_5){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0

define void @f(){
  %x = load i32, i32* @x
  %xcmp = icmp eq i32 %x, 0
  br i1 %xcmp, label %ok, label %error
error:
  call void @__assert_fail()
  ret void
ok:
  store i32 1, i32* @x
  ret void
}

define void @g(){
  %x = load i32, i32* @x
  %xcmp = icmp eq i32 %x, 1
  br i1 %xcmp, label %ok, label %error
error:
  call void @__assert_fail()
  ret void
ok:
  store i32 2, i32* @x
  ret void
}

define i32 @main(){
  call i32 @atexit(void()*@f)
  call i32 @atexit(void()*@g)
  ret i32 0
}

declare i32 @atexit(void()*)
declare void @__assert_fail()
)"),conf);
  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(Atexit_6){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0

define void @f(){
  %x = load i32, i32* @x
  %xcmp = icmp eq i32 %x, 0
  br i1 %xcmp, label %ok, label %error
error:
  call void @__assert_fail()
  ret void
ok:
  store i32 1, i32* @x
  ret void
}

define void @g(){
  store i32 2, i32* @x
  ret void
}

define i32 @main(){
  call i32 @atexit(void()*@f)
  call i32 @atexit(void()*@g)
  ret i32 0
}

declare i32 @atexit(void()*)
declare void @__assert_fail()
)"),conf);
  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(Atexit_multithreaded){
  Debug::warn("sctestatexitmultithreaded")
    << "WARNING: Missing support for multithreaded atexit.\n";
}

BOOST_AUTO_TEST_SUITE_END()

#endif
