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
#include <cstdint>

BOOST_AUTO_TEST_SUITE(PSO_test)

BOOST_AUTO_TEST_CASE(Overlapping_store_1){
  Configuration conf = DPORDriver_test::get_pso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p0(i8* %arg){
  %xb0 = bitcast i32* @x to i8*
  %xb1 = getelementptr i8, i8* %xb0, i32 1
  store i8 1, i8* %xb0
  store i8 1, i8* %xb1
  ret i8* null
}

define i8* @p1(i8* %arg){
  %xb0 = bitcast i32* @x to i8*
  %xb1 = getelementptr i8, i8* %xb0, i32 1
  load i8, i8* %xb1
  load i8, i8* %xb0
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p0, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P, P0 = P.spawn(0), P1 = P.spawn(1),
    U00 = P0.aux(0), U01 = P0.aux(1);
  IID<CPid> u00(U00,1), u01(U01,1), r11(P1,3), r10(P1,4);
  DPORDriver_test::trace_set_spec expected =
    {{{u00,r10},{u01,r11}},
     {{u00,r10},{r11,u01}},
     {{r10,u00},{u01,r11}},
     {{r10,u00},{r11,u01}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Overlapping_store_2){
  Configuration conf = DPORDriver_test::get_pso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p0(i8* %arg){
  %xb0 = bitcast i32* @x to i8*
  %xb1 = getelementptr i8, i8* %xb0, i32 1
  store i32 1, i32* @x
  store i8 1, i8* %xb1
  ret i8* null
}

define i8* @p1(i8* %arg){
  %xb0 = bitcast i32* @x to i8*
  %xb1 = getelementptr i8, i8* %xb0, i32 1
  load i8, i8* %xb1
  load i8, i8* %xb0
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p0, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P, P0 = P.spawn(0), P1 = P.spawn(1),
    U00 = P0.aux(0), U01 = P0.aux(1);
  IID<CPid> u00(U00,1), u01(U01,1), r11(P1,3), r10(P1,4);
  DPORDriver_test::trace_set_spec expected =
    {{{u00,r10},{u01,r11}},
     {{u00,r10},{r11,u01},{u00,r11}},
     {{u00,r10},{r11,u01},{r11,u00}},
     {{r10,u00},{r11,u01},{r11,u00}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Overlapping_store_3){
  /*
   * loc   bytes
   * -----------
   * @x    XXXX
   * %xw0  XX--
   * %xw05 -XX-
   * %xw1  --XX
   * %xb0  X---
   * %xb3  ---X
   *
   * The store to %xw05 enforces order between the stores to %xw0 and
   * %xw1.
   */
  Configuration conf = DPORDriver_test::get_pso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p0(i8* %arg){
  %xw0 = bitcast i32* @x to i16*
  %xw1 = getelementptr i16, i16* %xw0, i32 1
  %xb0 = bitcast i32* @x to i8*
  %xb1 = getelementptr i8, i8* %xb0, i32 1
  %xw05 = bitcast i8* %xb1 to i16*
  store i16 1, i16* %xw0
  store i16 1, i16* %xw05
  store i16 1, i16* %xw1
  ret i8* null
}

define i8* @p1(i8* %arg){
  %xb0 = bitcast i32* @x to i8*
  %xb3 = getelementptr i8, i8* %xb0, i32 3
  load i8, i8* %xb3
  load i8, i8* %xb0
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p0, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P, P0 = P.spawn(0), P1 = P.spawn(1),
    U00 = P0.aux(0), U005 = P0.aux(1), U01 = P0.aux(2);
  IID<CPid> u00(U00,1), u01(U01,1), r13(P1,3), r10(P1,4);
  DPORDriver_test::trace_set_spec expected =
    {{{u00,r10},{u01,r13}},
     {{u00,r10},{r13,u01}},
     {{r10,u00},{r13,u01}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Mutex_trylock_1){
  Configuration conf = DPORDriver_test::get_pso_conf();
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

BOOST_AUTO_TEST_CASE(Mutex_trylock_2){
  Configuration conf = DPORDriver_test::get_pso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 8

define i8* @l(i8*) {
  call i32 @pthread_mutex_lock(i32* @lck)
  call i32 @pthread_mutex_unlock(i32* @lck)
  ret i8* null
}

define i8* @tl(i8*) {
  %lckret = call i32 @pthread_mutex_trylock(i32* @lck)
  %lcksuc = icmp eq i32 %lckret, 0
  br i1 %lcksuc, label %CS, label %exit
CS:
  call i32 @pthread_mutex_unlock(i32* @lck)
  br label %exit
exit:
  ret i8* null
}

define i32 @main() {
  %il1 = alloca i64, align 8
  %il2 = alloca i64, align 8
  %itl = alloca i64, align 8
  call i32 @pthread_mutex_init(i32* @lck, i32* null) #3
  call i32 @pthread_create(i64* %il1, %attr_t* null, i8* (i8*)* @l, i8* null) #3
  call i32 @pthread_create(i64* %itl, %attr_t* null, i8* (i8*)* @tl, i8* null) #3
  call i32 @pthread_create(i64* %il2, %attr_t* null, i8* (i8*)* @l, i8* null) #3
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8* (i8*)*, i8*)
declare i32 @pthread_mutex_lock(i32*)
declare i32 @pthread_mutex_unlock(i32*)
declare i32 @pthread_mutex_trylock(i32*)
declare i32 @pthread_mutex_init(i32*, i32*)
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0, PL1 = P0.spawn(0), PTL = P0.spawn(1), PL2 = P0.spawn(2);
  IID<CPid> lck1(PL1,1), ulck1(PL1,2), lck2(PL2,1), ulck2(PL2,2),
    tlck(PTL,1), tulck(PTL,4);
  DPORDriver_test::trace_set_spec expected =
    {{{ulck1,lck2},{ulck2,tlck}},             // L1, L2,      TL
     {{ulck1,lck2},{lck2,tlck},{tlck,ulck2}}, // L1, L2,      TL-fail
     {{ulck1,tlck},{tulck,lck2}},             // L1, TL,      L2
     {{lck1,tlck},{tlck,ulck1},{ulck1,lck2}}, // L1, TL-fail, L2
     {{tulck,lck1},{ulck1,lck2}},             // TL, L1,      L2
     {{tulck,lck2},{ulck2,lck1}},             // TL, L2,      L1
     {{ulck2,lck1},{ulck1,tlck}},             // L2, L1,      TL
     {{ulck2,lck1},{lck1,tlck},{tlck,ulck1}}, // L2, L1,      TL-fail
     {{ulck2,tlck},{tulck,lck1}},             // L2, TL,      L1
     {{lck2,tlck},{tlck,ulck2},{ulck2,lck1}}, // L2, TL-fail, L1
    };
  BOOST_CHECK(!res.has_errors());
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Condvar_1){
  Configuration conf = DPORDriver_test::get_pso_conf();
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
  Configuration conf = DPORDriver_test::get_pso_conf();
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
  Configuration conf = DPORDriver_test::get_pso_conf();
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
  Configuration conf = DPORDriver_test::get_pso_conf();
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
  Configuration conf = DPORDriver_test::get_pso_conf();
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
  Configuration conf = DPORDriver_test::get_pso_conf();
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
  Configuration conf = DPORDriver_test::get_pso_conf();
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
  Configuration conf = DPORDriver_test::get_pso_conf();
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
  Configuration conf = DPORDriver_test::get_pso_conf();
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
  Configuration conf = DPORDriver_test::get_pso_conf();
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
  Configuration conf = DPORDriver_test::get_pso_conf();
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
  Configuration conf = DPORDriver_test::get_pso_conf();
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
  Configuration conf = DPORDriver_test::get_pso_conf();
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
  Configuration conf = DPORDriver_test::get_pso_conf();
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

BOOST_AUTO_TEST_CASE(Compiler_fence_mp){
  /* MP with compiler fence */
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  %y = load i32, i32* @y, align 4
  call void asm sideeffect "", "~{memory}"()
  %x = load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  store i32 1, i32* @x, align 4
  call void asm sideeffect "", "~{memory}"()
  store i32 1, i32* @y, align 4
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0; CPid U0x = P0.aux(0); CPid U0y = P0.aux(1);
  CPid P1 = P0.spawn(0);
  IID<CPid>
    ux0(U0x,1), uy0(U0y,1),
    ry1(P1,1), rx1(P1,3);
  DPORDriver_test::trace_set_spec expected =
    {{{ux0,rx1},{uy0,ry1}},
     {{ux0,rx1},{ry1,uy0}},
     {{rx1,ux0},{uy0,ry1}},
     {{rx1,ux0},{ry1,uy0}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Nondeterminism_detection){
  /* Simulate thread-wise nondeterminism, and check that it is
   * detected.
   *
   * Here we use a memory location _changing_ which is declared
   * *outside* of the test code. This is in order to ensure that its
   * value is not reinitialized when the test is restarted (as a
   * static variable inside the test code would be).
   *
   * The code in main will run twice. (This is ensured by the race on
   * x.)
   *
   * The code in main branches on the value of _changing_, and changes
   * its value. Therefore the branch will go in different directions
   * in the first and the second computation. Since the branch is
   * evaluated in the portion of the code that is replayed, this
   * apparent non-determinism should be detected and trigger an error.
   *
   * All accesses to _changing_ are done through memcpy() in order to
   * mask the fact that it is not a valid memory location, so that this
   * test will exercise the nondeterminism detection, and not other
   * mechanisms that detect invalid input.
   */
  Configuration conf = DPORDriver_test::get_pso_conf();
  char changing = 0;
  std::stringstream ss;
  ss << "inttoptr(i64 " << (uintptr_t)(&changing) << " to i8*)";
  std::string changingAddr = ss.str();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 1, i32* @x
  ret i8* null
}

define i32 @main(){
  %tp = alloca i8
  call i8* @memcpy(i8* %tp, i8* )"+changingAddr+R"(, i64 1)
  %v = load i8, i8* %tp
  %c = icmp eq i8 %v, 0
  br i1 %c, label %zero, label %nonzero
zero:
  store i8 1, i8* %tp
  call i8* @memcpy(i8* )"+changingAddr+R"(, i8* %tp, i64 1)
  br label %nonzero
nonzero:
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  load i32, i32* @x
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare void @__assert_fail()
declare i8* @memcpy(i8*, i8*, i64)
)"),conf);

  DPORDriver::Result res = driver->run();

  BOOST_CHECK(changing == 1);
  BOOST_CHECK(res.has_errors());

  delete driver;
}

BOOST_AUTO_TEST_CASE(Nondeterminism_without_branch){
  /* Mechanism is the same as in Nonteterminsim_detection (cf for
   * detailed description of the test)
   * Here, instead having a branch that changes direction, we have a
   * load that changes address
   */
  Configuration conf = DPORDriver_test::get_sc_conf();
  char changing = 0;
  std::stringstream ss;
  ss << "inttoptr(i64 " << (uintptr_t)(&changing) << " to i8*)";
  std::string changingAddr = ss.str();
  std::string module = StrModule::portasm(R"(
@x = global i32 0, align 4
@arr = constant [2 x i32] [i32 42, i32 999]

define i8* @p1(i8* %arg){
  store i32 1, i32* @x
  ret i8* null
}

define i32 @main(){
  %tp = alloca i8
  call i8* @memcpy(i8* %tp, i8* )"+changingAddr+R"(, i64 1)
  %v = load i8, i8* %tp
  %v64 = zext i8 %v to i64
  %p = getelementptr [2 x i32], [2 x i32]* @arr, i64 0, i64 %v64
  %xxx = load i32, i32* %p
  store i8 1, i8* %tp
  call i8* @memcpy(i8* )"+changingAddr+R"(, i8* %tp, i64 1)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  load i32, i32* @x
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare void @__assert_fail()
declare i8* @memcpy(i8*, i8*, i64)
)");

  DPORDriver *driver = DPORDriver::parseIR(module, conf);
  DPORDriver::Result res = driver->run();

  BOOST_CHECK(changing == 1);
  BOOST_CHECK(res.has_errors());
  delete driver;
}

BOOST_AUTO_TEST_SUITE_END()

#endif
