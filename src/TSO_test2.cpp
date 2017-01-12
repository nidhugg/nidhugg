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

BOOST_AUTO_TEST_SUITE(TSO_test)

BOOST_AUTO_TEST_CASE(Mutex_trylock_1){
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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

BOOST_AUTO_TEST_CASE(Compiler_fenced_dekker){
  /* Dekker with compiler fence (). */
  Configuration conf = DPORDriver_test::get_tso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p0(i8* %arg){
  store i32 1, i32* @x, align 4
  call void asm sideeffect "", "~{memory}"()
  %1 = load i32, i32* @y, align 4
  ret i8* null
}

define i8* @p1(i8* %arg){
  store i32 1, i32* @y, align 4
  call void asm sideeffect "", "~{memory}"()
  %1 = load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %union.pthread_attr_t* null, i8*(i8*)* @p1,i8* null)
  call i8* @p0(i8* null)
  ret i32 0
}

%union.pthread_attr_t = type { i64, [48 x i8] }

declare i32 @pthread_create(i64*,%union.pthread_attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();

  CPid P0;
  CPid U0 = P0.aux(0);
  CPid P1 = P0.spawn(0);
  CPid U1 = P1.aux(0);

  IID<CPid>
    ux0(U0,1),
    ry0(P0,5),
    uy1(U1,1),
    rx1(P1,3);

  DPORDriver_test::trace_set_spec spec =
    {{{ux0,rx1},{uy1,ry0}},
     {{ux0,rx1},{ry0,uy1}},
     {{rx1,ux0},{uy1,ry0}},
     {{rx1,ux0},{ry0,uy1}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,spec,conf));

  delete driver;
}

BOOST_AUTO_TEST_CASE(Mutex_13_no_req_init){
  /* Mutex lock without init with mutex_require_init==false. */
  Configuration conf = DPORDriver_test::get_tso_conf();
  conf.mutex_require_init = false;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4

define i32 @main(){
  call i32 @pthread_mutex_lock(i32* @lck)
  ret i32 0
}

declare i32 @pthread_mutex_lock(i32*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Mutex_14_no_req_init){
  /* Mutex trylock without init with mutex_require_init==false. */
  Configuration conf = DPORDriver_test::get_tso_conf();
  conf.mutex_require_init = false;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4

define i32 @main(){
  call i32 @pthread_mutex_trylock(i32* @lck)
  ret i32 0
}

declare i32 @pthread_mutex_trylock(i32*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Mutex_15_no_req_init){
  /* Mutex destroy without init with mutex_require_init==false. */
  Configuration conf = DPORDriver_test::get_tso_conf();
  conf.mutex_require_init = false;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4

define i32 @main(){
  call i32 @pthread_mutex_destroy(i32* @lck)
  ret i32 0
}

declare i32 @pthread_mutex_destroy(i32*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Mutex_16_no_req_init){
  /* Race between mutex lock and init. This is still an error with
   * mutex_require_init==false. That is because if the mutex is
   * assumed to be initialized before the lock, then a later init will
   * be a double initialization.
   */
  Configuration conf = DPORDriver_test::get_tso_conf();
  conf.mutex_require_init = false;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4
@c = global i32 0, align 4

define i8* @p1(i8* %arg){
  call i32 @pthread_mutex_lock(i32* @lck)
  %c = load i32, i32* @c, align 4
  %ccmp = icmp ne i32 %c, 0
  br i1 %ccmp, label %error, label %cs
error:
  call void @__assert_fail()
  br label %cs
cs:
  store i32 1, i32* @c, align 4
  store i32 0, i32* @c, align 4
  call i32 @pthread_mutex_unlock(i32* @lck)
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call i32 @pthread_mutex_init(i32* @lck, i32* null)
  call i8* @p1(i8* null)
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_mutex_init(i32*,i32*) nounwind
declare i32 @pthread_mutex_lock(i32*) nounwind
declare i32 @pthread_mutex_unlock(i32*) nounwind
declare void @__assert_fail() noreturn nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(Mutex_17_req_init){
  /* cond_wait without init with mutex_require_init==true. */
  Configuration conf = DPORDriver_test::get_tso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@cnd = global i32 0, align 4
@lck = global i32 0, align 4

define i32 @main(){
  call i32 @pthread_cond_init(i32* @cnd, i32* null)
  call i32 @pthread_mutex_lock(i32* @lck)
  call i32 @pthread_cond_wait(i32* @cnd, i32* @lck)
  ret i32 0
}

declare i32 @pthread_mutex_lock(i32*) nounwind
declare i32 @pthread_cond_init(i32*,i32*) nounwind
declare i32 @pthread_cond_wait(i32*,i32*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(Mutex_18_no_req_init){
  /* cond_wait without init with mutex_require_init==false. */
  Configuration conf = DPORDriver_test::get_tso_conf();
  conf.mutex_require_init = false;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@cnd = global i32 0, align 4
@lck = global i32 0, align 4

define i32 @main(){
  call i32 @pthread_cond_init(i32* @cnd, i32* null)
  call i32 @pthread_mutex_lock(i32* @lck)
  call i32 @pthread_cond_wait(i32* @cnd, i32* @lck)
  ret i32 0
}

declare i32 @pthread_mutex_lock(i32*) nounwind
declare i32 @pthread_cond_init(i32*,i32*) nounwind
declare i32 @pthread_cond_wait(i32*,i32*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_SUITE_END()

#endif
