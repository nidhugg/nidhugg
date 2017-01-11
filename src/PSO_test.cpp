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

BOOST_AUTO_TEST_SUITE(PSO_test)

BOOST_AUTO_TEST_CASE(minimal){
  Configuration conf = DPORDriver_test::get_pso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i32 @main(){
  store i32 1, i32* @x, align 4
  store i32 1, i32* @y, align 4
  store i32 1, i32* @x, align 4
  ret i32 0
}
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(DPORDriver_test::check_all_traces(res,{{}},conf));
}

BOOST_AUTO_TEST_CASE(mp){
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  %y = load i32, i32* @y, align 4
  %x = load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  store i32 1, i32* @x, align 4
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
    ry1(P1,1), rx1(P1,2);
  DPORDriver_test::trace_set_spec expected =
    {{{ux0,rx1},{uy0,ry1}},
     {{ux0,rx1},{ry1,uy0}},
     {{rx1,ux0},{uy0,ry1}},
     {{rx1,ux0},{ry1,uy0}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(mp_2){
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 1, i32* @x, align 4
  store i32 1, i32* @y, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  %y = load i32, i32* @y, align 4
  %x = load i32, i32* @x, align 4
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0;
  CPid P1 = P0.spawn(0);
  CPid U1x = P1.aux(0); CPid U1y = P1.aux(1);
  IID<CPid>
    ry0(P0,2), rx0(P0,3),
    ux1(U1x,1), uy1(U1y,1);
  DPORDriver_test::trace_set_spec expected =
    {{{ux1,rx0},{uy1,ry0}},
     {{ux1,rx0},{ry0,uy1}},
     {{rx0,ux1},{uy1,ry0}},
     {{rx0,ux1},{ry0,uy1}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(mp_3){
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  %y = load i32, i32* @y, align 4
  %x = load i32, i32* @x, align 4
  %xycmp = icmp sge i32 %x, %y
  br i1 %xycmp, label %exit, label %error
error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  store i32 1, i32* @x, align 4
  fence seq_cst
  store i32 1, i32* @y, align 4
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0; CPid U0x = P0.aux(0); CPid U0y = P0.aux(1);
  CPid P1 = P0.spawn(0);
  IID<CPid>
    ux0(U0x,1), uy0(U0y,1),
    ry1(P1,1), rx1(P1,2);
  DPORDriver_test::trace_set_spec expected =
    {{{ux0,rx1},{uy0,ry1}},
     {{ux0,rx1},{ry1,uy0}},
     {{rx1,ux0},{ry1,uy0}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Small_rowe){
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  store i32 2, i32* @x, align 4
  %x = load i32, i32* @x, align 4
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0; CPid U0x = P0.aux(0);
  CPid P1 = P0.spawn(0); CPid U1x = P1.aux(0);
  IID<CPid>
    ux0(U0x,1), rx0(P0,3), ux1(U1x,1);
  DPORDriver_test::trace_set_spec expected =
    {{{ux0,ux1},{rx0,ux1}},
     {{ux0,ux1},{ux1,rx0}},
     {{ux1,ux0}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Small_rowe_2){
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 1, i32* @x, align 4
  %x = load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  store i32 2, i32* @x, align 4
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0; CPid U0x = P0.aux(0);
  CPid P1 = P0.spawn(0); CPid U1x = P1.aux(0);
  IID<CPid>
    ux0(U0x,1), rx1(P1,2), ux1(U1x,1);
  DPORDriver_test::trace_set_spec expected =
    {{{ux0,ux1}},
     {{ux1,ux0},{rx1,ux0}},
     {{ux1,ux0},{ux0,rx1}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Join_1){
  /* Check propagation of arguments through pthread_create, and return
   * values through pthread_join.
   */
  Configuration conf = DPORDriver_test::get_pso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
define i8* @p1(i8* %arg){
  %iarg = ptrtoint i8* %arg to i32
  %rv = mul i32 %iarg, %iarg
  %retval = inttoptr i32 %rv to i8*
  ret i8* %retval
}


define i32 @main(){
  %thread = alloca i64
  %rv = alloca i8*
  %arg = inttoptr i32 4 to i8*
  call i32 @pthread_create(i64* %thread, %attr_t* null, i8*(i8*)* @p1, i8* %arg)
  %thr = load i64, i64* %thread
  call i32 @pthread_join(i64 %thr,i8** %rv)
  %rvv = load i8*, i8** %rv
  %retval = ptrtoint i8* %rvv to i32
  %rvcmp = icmp ne i32 %retval, 16
  br i1 %rvcmp, label %error, label %exit
error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_join(i64, i8**) nounwind
declare void @__assert_fail() noreturn nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Join_2){
  Configuration conf = DPORDriver_test::get_pso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  %thread = alloca i64
  call i32 @pthread_create(i64* %thread, %attr_t* null, i8*(i8*)* @p1, i8* null)
  %thr = load i64, i64* %thread
  call i32 @pthread_join(i64 %thr,i8** null)
  store i32 1, i32* @x, align 4
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_join(i64, i8**) nounwind
declare void @__assert_fail() noreturn nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0; CPid U0 = P0.aux(0);
  CPid P1 = P0.spawn(0);
  IID<CPid>
    ux0(U0,1), rx1(P1,1);
  DPORDriver_test::trace_set_spec expected =
    {{{rx1,ux0}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Join_3){
  Configuration conf = DPORDriver_test::get_pso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 42, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  %thread = alloca i64
  call i32 @pthread_create(i64* %thread, %attr_t* null, i8*(i8*)* @p1, i8* null)
  %thr = load i64, i64* %thread
  call i32 @pthread_join(i64 %thr,i8** null)
  %x = load i32, i32* @x, align 4
  %xcmp = icmp ne i32 %x, 42
  br i1 %xcmp, label %error, label %exit
error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_join(i64, i8**) nounwind
declare void @__assert_fail() noreturn nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0; CPid U0 = P0.aux(0);
  CPid P1 = P0.spawn(0); CPid U1 = P1.aux(0);
  IID<CPid>
    ux1(U1,1), rx0(P0,5);
  DPORDriver_test::trace_set_spec expected =
    {{{ux1,rx0}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Mutex_1){
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.explore_all_traces = true;
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
  call i32 @pthread_mutex_init(i32* @lck, i32* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
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

  BOOST_CHECK(!res.has_errors());

  CPid P0; CPid P1 = P0.spawn(0);
  IID<CPid>
    lock0(P0,4), unlock0(P0,10),
    lock1(P1,1), unlock1(P1,7);
  DPORDriver_test::trace_set_spec expected =
    {{{unlock0,lock1}},
     {{unlock1,lock0}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Mutex_2){
  /* Possible access to mutex before initialization. */
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.explore_all_traces = true;
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

BOOST_AUTO_TEST_CASE(Mutex_3){
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 1, i32* @x, align 4
  call i32 @pthread_mutex_lock(i32* @lck)
  store i32 1, i32* @y, align 4
  call i32 @pthread_mutex_unlock(i32* @lck)
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_mutex_init(i32* @lck, i32* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call i32 @pthread_mutex_lock(i32* @lck)
  %y = load i32, i32* @y, align 4
  call i32 @pthread_mutex_unlock(i32* @lck)
  %x = load i32, i32* @x, align 4
  %xycmp = icmp sgt i32 %y, %x
  br i1 %xycmp, label %error, label %exit
error:
  call void @__assert_fail()
  br label %exit
exit:
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

  BOOST_CHECK(!res.has_errors());

  CPid P0;
  CPid P1 = P0.spawn(0); CPid U1x = P1.aux(0); CPid U1y = P1.aux(1);
  IID<CPid>
    ux1(U1x,1), uy1(U1y,1), lock1(P1,2), unlock1(P1,4),
    ry0(P0,4), rx0(P0,6), lock0(P0,3), unlock0(P0,5);
  DPORDriver_test::trace_set_spec expected =
    {{{unlock0,lock1},{ry0,uy1},{rx0,ux1}},
     {{unlock0,lock1},{ry0,uy1},{ux1,rx0}},
     {{unlock1,lock0},{uy1,ry0},{ux1,rx0}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Mutex_4){
  /* Unlock without lock */
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4

define i32 @main(){
  call i32 @pthread_mutex_init(i32* @lck, i32* null)
  call i32 @pthread_mutex_unlock(i32* @lck)
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_mutex_init(i32*,i32*) nounwind
declare i32 @pthread_mutex_unlock(i32*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(Mutex_5){
  /* Unlock mutex when locked by other process. */
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  call i32 @pthread_mutex_unlock(i32* @lck)
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_mutex_init(i32* @lck, i32* null)
  call i32 @pthread_mutex_lock(i32* @lck)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
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

BOOST_AUTO_TEST_CASE(Mutex_6){
  /* Make sure that we do not miss computations even when some process
   * refuses to release a lock. */
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4
@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  call i32 @pthread_mutex_lock(i32* @lck)
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  store i32 1, i32* @x, align 4
  call i32 @pthread_mutex_init(i32* @lck, i32* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call i32 @pthread_mutex_lock(i32* @lck)
  load i32, i32* @x, align 4
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

  CPid P0; CPid U0x = P0.aux(0);
  CPid P1 = P0.spawn(0); CPid P2 = P0.spawn(1);
  IID<CPid>
    ux0(U0x,1), rx0(P0,6), rx1(P1,2), rx2(P2,2);
  DPORDriver_test::trace_set_spec expected =
    {{{ux0,rx0}},
     {{ux0,rx1}},
     {{ux0,rx2}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Mutex_7){
  /* Typical use of pthread_mutex_destroy */
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4

define i8* @p1(i8* %arg){
  call i32 @pthread_mutex_lock(i32* @lck)
  call i32 @pthread_mutex_unlock(i32* @lck)
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_mutex_init(i32* @lck, i32* null)
  %thread = alloca i64, align 4
  call i32 @pthread_create(i64* %thread, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call i32 @pthread_mutex_lock(i32* @lck)
  call i32 @pthread_mutex_unlock(i32* @lck)
  %thr = load i64, i64* %thread, align 4
  call i32 @pthread_join(i64 %thr, i8** null)
  call i32 @pthread_mutex_destroy(i32* @lck)
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_join(i64, i8**) nounwind
declare i32 @pthread_mutex_init(i32*,i32*) nounwind
declare i32 @pthread_mutex_destroy(i32*) nounwind
declare i32 @pthread_mutex_lock(i32*) nounwind
declare i32 @pthread_mutex_unlock(i32*) nounwind
declare void @__assert_fail() noreturn nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0; CPid P1 = P0.spawn(0);
  IID<CPid>
    lock0(P0,4), unlock0(P0,5), destroy0(P0,8),
    lock1(P1,1), unlock1(P1,2);
  DPORDriver_test::trace_set_spec expected =
    {{{unlock0,lock1},{unlock1,destroy0}},
     {{unlock1,lock0},{unlock0,destroy0}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Mutex_8){
  /* Too early destruction of mutex. */
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4

define i8* @p1(i8* %arg){
  call i32 @pthread_mutex_lock(i32* @lck)
  call i32 @pthread_mutex_unlock(i32* @lck)
  call i32 @pthread_mutex_destroy(i32* @lck)
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_mutex_init(i32* @lck, i32* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call i32 @pthread_mutex_lock(i32* @lck)
  call i32 @pthread_mutex_unlock(i32* @lck)
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_join(i64, i8**) nounwind
declare i32 @pthread_mutex_init(i32*,i32*) nounwind
declare i32 @pthread_mutex_destroy(i32*) nounwind
declare i32 @pthread_mutex_lock(i32*) nounwind
declare i32 @pthread_mutex_unlock(i32*) nounwind
declare void @__assert_fail() noreturn nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(Mutex_9){
  /* Mutex destruction and flag */
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4
@flag = global i32 0, align 4

define i8* @p1(i8* %arg){
  call i32 @pthread_mutex_lock(i32* @lck)
  call i32 @pthread_mutex_unlock(i32* @lck)
  %flag = load i32, i32* @flag, align 4
  %fcmp = icmp eq i32 %flag, 1
  br i1 %fcmp, label %destroy, label %exit
destroy:
  call i32 @pthread_mutex_destroy(i32* @lck)
  br label %exit
exit:
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_mutex_init(i32* @lck, i32* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call i32 @pthread_mutex_lock(i32* @lck)
  call i32 @pthread_mutex_unlock(i32* @lck)
  store i32 1, i32* @flag, align 4
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_join(i64, i8**) nounwind
declare i32 @pthread_mutex_init(i32*,i32*) nounwind
declare i32 @pthread_mutex_destroy(i32*) nounwind
declare i32 @pthread_mutex_lock(i32*) nounwind
declare i32 @pthread_mutex_unlock(i32*) nounwind
declare void @__assert_fail() noreturn nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0; CPid U0f = P0.aux(0);
  CPid P1 = P0.spawn(0);
  IID<CPid>
    lock0(P0,3), unlock0(P0,4), uf0(U0f,1),
    lock1(P1,1), unlock1(P1,2), rf1(P1,3), destroy1(P1,6);
  DPORDriver_test::trace_set_spec expected =
    {{{unlock0,lock1},{uf0,rf1},{unlock0,destroy1}},
     {{unlock0,lock1},{rf1,uf0}},
     {{unlock1,lock0},{uf0,rf1},{unlock0,destroy1}},
     {{unlock1,lock0},{rf1,uf0}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Mutex_10){
  /* Check that critical sections locked with different locks can
   * overlap. */
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck0 = global i32 0, align 4
@lck1 = global i32 0, align 4
@c = global i32 0, align 4

define i8* @p0(i8* %arg){
  call i32 @pthread_mutex_lock(i32* @lck0)
  %c = load i32, i32* @c, align 4
  %ccmp = icmp ne i32 %c, 0
  br i1 %ccmp, label %error, label %cs
error:
  call void @__assert_fail()
  br label %cs
cs:
  store i32 1, i32* @c, align 4
  store i32 0, i32* @c, align 4
  call i32 @pthread_mutex_unlock(i32* @lck0)
  ret i8* null
}

define i8* @p1(i8* %arg){
  call i32 @pthread_mutex_lock(i32* @lck1)
  %c = load i32, i32* @c, align 4
  %ccmp = icmp ne i32 %c, 0
  br i1 %ccmp, label %error, label %cs
error:
  call void @__assert_fail()
  br label %cs
cs:
  store i32 1, i32* @c, align 4
  store i32 0, i32* @c, align 4
  call i32 @pthread_mutex_unlock(i32* @lck1)
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_mutex_init(i32* @lck0, i32* null)
  call i32 @pthread_mutex_init(i32* @lck1, i32* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call i8* @p0(i8* null)
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

BOOST_AUTO_TEST_CASE(Mutex_11){
  /* Check return values from calls to pthread_mutex_* */
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4
@c = global i32 0, align 4

define i8* @p1(i8* %arg){
  %i0 = call i32 @pthread_mutex_lock(i32* @lck)
  %c = load i32, i32* @c, align 4
  %ccmp = icmp ne i32 %c, 0
  br i1 %ccmp, label %error, label %cs
error:
  call void @__assert_fail()
  br label %cs
cs:
  store i32 1, i32* @c, align 4
  store i32 0, i32* @c, align 4
  %i1 = call i32 @pthread_mutex_unlock(i32* @lck)
  %icmp0 = icmp ne i32 %i0, 0
  %icmp1 = icmp ne i32 %i1, 0
  %icmp = or i1 %icmp0, %icmp1
  br i1 %icmp, label %error, label %exit
exit:
  ret i8* null
}

define i32 @main(){
  %i0 = call i32 @pthread_mutex_init(i32* @lck, i32* null)
  %icmp = icmp ne i32 %i0, 0
  br i1 %icmp, label %error, label %create
error:
  call void @__assert_fail()
  br label %create
create:
  %thread = alloca i64, align 4
  call i32 @pthread_create(i64* %thread, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call i8* @p1(i8* null)
  %thr = load i64, i64* %thread, align 4
  call i32 @pthread_join(i64 %thr, i8** null)
  %i1 = call i32 @pthread_mutex_destroy(i32* @lck)
  %icmp1 = icmp ne i32 %i1, 0
  br i1 %icmp1, label %error, label %exit
exit:
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_join(i64,i8**) nounwind
declare i32 @pthread_mutex_init(i32*,i32*) nounwind
declare i32 @pthread_mutex_destroy(i32*) nounwind
declare i32 @pthread_mutex_lock(i32*) nounwind
declare i32 @pthread_mutex_unlock(i32*) nounwind
declare void @__assert_fail() noreturn nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Mutex_12){
  /* Regression test: The set of processes waiting for a mutex to be
   * unlocked would not be cleared when they were awoken.
   */
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@lck = global i32 0, align 4

define i8* @p1(i8* %arg){
  call i32 @pthread_mutex_lock(i32* @lck)
  call i32 @pthread_mutex_unlock(i32* @lck)
  call i32 @pthread_mutex_lock(i32* @lck)
  call i32 @pthread_mutex_unlock(i32* @lck)
  ret i8* null
}

define i32 @main(){
  %i0 = call i32 @pthread_mutex_init(i32* @lck, i32* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call i8* @p1(i8* null)
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_join(i64,i8**) nounwind
declare i32 @pthread_mutex_init(i32*,i32*) nounwind
declare i32 @pthread_mutex_destroy(i32*) nounwind
declare i32 @pthread_mutex_lock(i32*) nounwind
declare i32 @pthread_mutex_unlock(i32*) nounwind
declare void @__assert_fail() noreturn nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0; CPid P1 = P0.spawn(0);
  IID<CPid>
    lock0a(P0,4), unlock0a(P0,5), lock0b(P0,6), unlock0b(P0,7),
    lock1a(P1,1), unlock1a(P1,2), lock1b(P1,3), unlock1b(P1,4);
  DPORDriver_test::trace_set_spec expected =
    {{{unlock0b,lock1a}}, // 0011
     {{unlock0a,lock1a},{unlock1a,lock0b},{unlock0b,lock1b}}, // 0101
     {{unlock0a,lock1a},{unlock1b,lock0b}}, // 0110
     {{unlock1a,lock0a},{unlock0a,lock1b},{unlock1b,lock0b}}, // 1010
     {{unlock1b,lock0a}}, // 1100
     {{unlock1a,lock0a},{unlock0b,lock1b}} // 1001
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Atomic_store_1){
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@c = global i32 0, align 4
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  store atomic i32 1, i32* @y seq_cst, align 4
  %x = load i32, i32* @x, align 4
  %xcmp = icmp eq i32 %x, 0
  br i1 %xcmp, label %cs, label %exit

cs:
  %c = load i32, i32* @c, align 4
  %ccmp = icmp eq i32 %c, 1
  br i1 %ccmp, label %error, label %cs2

cs2:
  store i32 1, i32* @c, align 4
  store i32 0, i32* @c, align 4
  br label %exit

error:
  call void @__assert_fail()
  br label %exit

exit:
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  store atomic i32 1, i32* @x seq_cst, align 4
  %y = load i32, i32* @y, align 4
  %ycmp = icmp eq i32 %y, 0
  br i1 %ycmp, label %cs, label %exit

cs:
  %c = load i32, i32* @c, align 4
  %ccmp = icmp eq i32 %c, 1
  br i1 %ccmp, label %error, label %cs2

cs2:
  store i32 1, i32* @c, align 4
  store i32 0, i32* @c, align 4
  br label %exit

error:
  call void @__assert_fail()
  br label %exit

exit:
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare void @__assert_fail() noreturn nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0;
  CPid P1 = P0.spawn(0);
  IID<CPid>
    ux0(P0,2), ry0(P0,3),
    uy1(P1,1), rx1(P1,2);
  DPORDriver_test::trace_set_spec expected =
    {{{ux0,rx1},{ry0,uy1}},
     {{ux0,rx1},{uy1,ry0}},
     {{rx1,ux0},{uy1,ry0}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Atomic_store_2){
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@c = global i32 0, align 4
@x = global i32 0, align 4
@y = global i32 0, align 4
@f0 = global i32 0, align 4
@f1 = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 1, i32* @y, align 4
  store atomic i32 1, i32* @f1 seq_cst, align 4
  %x = load i32, i32* @x, align 4
  %xcmp = icmp eq i32 %x, 0
  br i1 %xcmp, label %cs, label %exit

cs:
  %c = load i32, i32* @c, align 4
  %ccmp = icmp eq i32 %c, 1
  br i1 %ccmp, label %error, label %cs2

cs2:
  store i32 1, i32* @c, align 4
  store i32 0, i32* @c, align 4
  br label %exit

error:
  call void @__assert_fail()
  br label %exit

exit:
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  store i32 1, i32* @x, align 4
  store atomic i32 1, i32* @f0 seq_cst, align 4
  %y = load i32, i32* @y, align 4
  %ycmp = icmp eq i32 %y, 0
  br i1 %ycmp, label %cs, label %exit

cs:
  %c = load i32, i32* @c, align 4
  %ccmp = icmp eq i32 %c, 1
  br i1 %ccmp, label %error, label %cs2

cs2:
  store i32 1, i32* @c, align 4
  store i32 0, i32* @c, align 4
  br label %exit

error:
  call void @__assert_fail()
  br label %exit

exit:
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare void @__assert_fail() noreturn nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0; CPid U0x = P0.aux(0);
  CPid P1 = P0.spawn(0); CPid U1y = P1.aux(0);
  IID<CPid>
    ux0(U0x,1), ry0(P0,4),
    uy1(U1y,1), rx1(P1,3);
  DPORDriver_test::trace_set_spec expected =
    {{{ux0,rx1},{ry0,uy1}},
     {{ux0,rx1},{uy1,ry0}},
     {{rx1,ux0},{uy1,ry0}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Atomic_store_3){
  Configuration conf = DPORDriver_test::get_pso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4
@z = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 1, i32* @y, align 4
  store atomic i32 1, i32* @z seq_cst, align 4
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  store i32 1, i32* @x, align 4
  store atomic i32 1, i32* @z seq_cst, align 4
  load i32, i32* @y, align 4
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0, P1 = P0.spawn(0),
    U0 = P0.aux(0), U1 = P1.aux(0);
  IID<CPid> ux(U0,1), uy(U1,1), uz0(P0,3), uz1(P1,2),
    ry(P0,4), rx(P1,3);
  DPORDriver_test::trace_set_spec expected =
    {{{ux,rx},{uz0,uz1},{ry,uy}},
     {{ux,rx},{uz0,uz1},{uy,ry}},
     {{ux,rx},{uz1,uz0},{uy,ry}},
     {{rx,ux},{uz1,uz0},{uy,ry}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(CMPXCHG_1){
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@c = global i32 0, align 4
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){)"
#if defined(LLVM_CMPXCHG_SEPARATE_SUCCESS_FAILURE_ORDERING)
R"(
  cmpxchg volatile i32* @y, i32 0, i32 1 seq_cst seq_cst)"
#else
R"(
  cmpxchg volatile i32* @y, i32 0, i32 1 seq_cst)"
#endif
R"(
  %x = load i32, i32* @x, align 4
  %xcmp = icmp eq i32 %x, 0
  br i1 %xcmp, label %cs, label %exit

cs:
  %c = load i32, i32* @c, align 4
  %ccmp = icmp eq i32 %c, 1
  br i1 %ccmp, label %error, label %cs2

cs2:
  store i32 1, i32* @c, align 4
  store i32 0, i32* @c, align 4
  br label %exit

error:
  call void @__assert_fail()
  br label %exit

exit:
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null))"
#if defined(LLVM_CMPXCHG_SEPARATE_SUCCESS_FAILURE_ORDERING)
R"(
  cmpxchg volatile i32* @x, i32 0, i32 1 seq_cst seq_cst)"
#else
R"(
  cmpxchg volatile i32* @x, i32 0, i32 1 seq_cst)"
#endif
R"(
  %y = load i32, i32* @y, align 4
  %ycmp = icmp eq i32 %y, 0
  br i1 %ycmp, label %cs, label %exit

cs:
  %c = load i32, i32* @c, align 4
  %ccmp = icmp eq i32 %c, 1
  br i1 %ccmp, label %error, label %cs2

cs2:
  store i32 1, i32* @c, align 4
  store i32 0, i32* @c, align 4
  br label %exit

error:
  call void @__assert_fail()
  br label %exit

exit:
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare void @__assert_fail() noreturn nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0;
  CPid P1 = P0.spawn(0);
  IID<CPid>
    ux0(P0,2), ry0(P0,3),
    uy1(P1,1), rx1(P1,2);
  DPORDriver_test::trace_set_spec expected =
    {{{ux0,rx1},{ry0,uy1}},
     {{ux0,rx1},{uy1,ry0}},
     {{rx1,ux0},{uy1,ry0}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(CMPXCHG_2){
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@c = global i32 0, align 4
@x = global i32 0, align 4
@y = global i32 0, align 4
@f0 = global i32 0, align 4
@f1 = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 1, i32* @y, align 4)"
#if defined(LLVM_CMPXCHG_SEPARATE_SUCCESS_FAILURE_ORDERING)
R"(
  cmpxchg volatile i32* @f1, i32 0, i32 1 seq_cst seq_cst)"
#else
R"(
  cmpxchg volatile i32* @f1, i32 0, i32 1 seq_cst)"
#endif
R"(
  %x = load i32, i32* @x, align 4
  %xcmp = icmp eq i32 %x, 0
  br i1 %xcmp, label %cs, label %exit

cs:
  %c = load i32, i32* @c, align 4
  %ccmp = icmp eq i32 %c, 1
  br i1 %ccmp, label %error, label %cs2

cs2:
  store i32 1, i32* @c, align 4
  store i32 0, i32* @c, align 4
  br label %exit

error:
  call void @__assert_fail()
  br label %exit

exit:
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  store i32 1, i32* @x, align 4)"
#if defined(LLVM_CMPXCHG_SEPARATE_SUCCESS_FAILURE_ORDERING)
R"(
  cmpxchg volatile i32* @f0, i32 0, i32 1 seq_cst seq_cst)"
#else
R"(
  cmpxchg volatile i32* @f0, i32 0, i32 1 seq_cst)"
#endif
R"(
  %y = load i32, i32* @y, align 4
  %ycmp = icmp eq i32 %y, 0
  br i1 %ycmp, label %cs, label %exit

cs:
  %c = load i32, i32* @c, align 4
  %ccmp = icmp eq i32 %c, 1
  br i1 %ccmp, label %error, label %cs2

cs2:
  store i32 1, i32* @c, align 4
  store i32 0, i32* @c, align 4
  br label %exit

error:
  call void @__assert_fail()
  br label %exit

exit:
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare void @__assert_fail() noreturn nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0; CPid U0x = P0.aux(0);
  CPid P1 = P0.spawn(0); CPid U1y = P1.aux(0);
  IID<CPid>
    ux0(U0x,1), ry0(P0,4),
    uy1(U1y,1), rx1(P1,3);
  DPORDriver_test::trace_set_spec expected =
    {{{ux0,rx1},{ry0,uy1}},
     {{ux0,rx1},{uy1,ry0}},
     {{rx1,ux0},{uy1,ry0}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(CMPXCHG_3){
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@c = global i32 0, align 4
@l = global i32 0, align 4

define i8* @p1(i8* %arg){)"
#if defined(LLVM_CMPXCHG_SEPARATE_SUCCESS_FAILURE_ORDERING)
R"(
  %old = cmpxchg volatile i32* @l, i32 0, i32 1 seq_cst seq_cst
  %ocmp = extractvalue {i32,i1} %old, 1)"
#else
R"(
  %old = cmpxchg volatile i32* @l, i32 0, i32 1 seq_cst
  %ocmp = icmp eq i32 %old, 0)"
#endif
R"(
  br i1 %ocmp, label %cs, label %exit
cs:
  store i32 1, i32* @c, align 4
  store i32 0, i32* @l, align 4
  br label %exit
exit:
  ret i8* null
}


define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null))"
#if defined(LLVM_CMPXCHG_SEPARATE_SUCCESS_FAILURE_ORDERING)
R"(
  %old = cmpxchg volatile i32* @l, i32 0, i32 1 seq_cst seq_cst
  %ocmp = extractvalue {i32,i1} %old, 1)"
#else
R"(
  %old = cmpxchg volatile i32* @l, i32 0, i32 1 seq_cst
  %ocmp = icmp eq i32 %old, 0)"
#endif
R"(
  br i1 %ocmp, label %cs, label %exit
cs:
  %c = load i32, i32* @c, align 4
  %ccmp = icmp eq i32 %c, 1
  br i1 %ccmp, label %cone, label %unlock
cone:
  call void @__assert_fail()
  br label %unlock
unlock:
  store i32 0, i32* @l, align 4
  br label %exit
exit:
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare void @__assert_fail() noreturn nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(CMPXCHG_4){
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@c = global i32 0, align 4
@l = global i32 0, align 4

define i8* @p1(i8* %arg){)"
#if defined(LLVM_CMPXCHG_SEPARATE_SUCCESS_FAILURE_ORDERING)
R"(
  %old = cmpxchg volatile i32* @l, i32 0, i32 1 seq_cst seq_cst
  %ocmp = extractvalue {i32,i1} %old, 1)"
#else
R"(
  %old = cmpxchg volatile i32* @l, i32 0, i32 1 seq_cst
  %ocmp = icmp eq i32 %old, 0)"
#endif
R"(
  br i1 %ocmp, label %cs, label %exit
cs:
  store i32 1, i32* @c, align 4
  store i32 0, i32* @c, align 4
  store i32 0, i32* @l, align 4
  br label %exit
exit:
  ret i8* null
}


define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null))"
#if defined(LLVM_CMPXCHG_SEPARATE_SUCCESS_FAILURE_ORDERING)
R"(
  %old = cmpxchg volatile i32* @l, i32 0, i32 1 seq_cst seq_cst
  %ocmp = extractvalue {i32,i1} %old, 1)"
#else
R"(
  %old = cmpxchg volatile i32* @l, i32 0, i32 1 seq_cst
  %ocmp = icmp eq i32 %old, 0)"
#endif
R"(
  br i1 %ocmp, label %cs, label %exit
cs:
  %c = load i32, i32* @c, align 4
  %ccmp = icmp eq i32 %c, 1
  br i1 %ccmp, label %cone, label %unlock
cone:
  call void @__assert_fail()
  br label %unlock
unlock:
  store i32 0, i32* @l, align 4
  br label %exit
exit:
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare void @__assert_fail() noreturn nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(CMPXCHG_5){
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@c = global i32 0, align 4
@l = global i32 0, align 4

define i8* @p1(i8* %arg){)"
#if defined(LLVM_CMPXCHG_SEPARATE_SUCCESS_FAILURE_ORDERING)
R"(
  %old = cmpxchg volatile i32* @l, i32 0, i32 1 seq_cst seq_cst
  %ocmp = extractvalue {i32,i1} %old, 1)"
#else
R"(
  %old = cmpxchg volatile i32* @l, i32 0, i32 1 seq_cst
  %ocmp = icmp eq i32 %old, 0)"
#endif
R"(
  br i1 %ocmp, label %cs, label %exit
cs:
  store i32 1, i32* @c, align 4
  store i32 0, i32* @c, align 4
  fence seq_cst
  store i32 0, i32* @l, align 4
  br label %exit
exit:
  ret i8* null
}


define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null))"
#if defined(LLVM_CMPXCHG_SEPARATE_SUCCESS_FAILURE_ORDERING)
R"(
  %old = cmpxchg volatile i32* @l, i32 0, i32 1 seq_cst seq_cst
  %ocmp = extractvalue {i32,i1} %old, 1)"
#else
R"(
  %old = cmpxchg volatile i32* @l, i32 0, i32 1 seq_cst
  %ocmp = icmp eq i32 %old, 0)"
#endif
R"(
  br i1 %ocmp, label %cs, label %exit
cs:
  %c = load i32, i32* @c, align 4
  %ccmp = icmp eq i32 %c, 1
  br i1 %ccmp, label %cone, label %unlock
cone:
  call void @__assert_fail()
  br label %unlock
unlock:
  store i32 0, i32* @l, align 4
  br label %exit
exit:
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare void @__assert_fail() noreturn nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Malloc_1){
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.malloc_may_fail = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
define i32 @main(){
  %p = call i8* @malloc(i32 10)
  %pcmp = icmp eq i8* %p, null
  br i1 %pcmp, label %error, label %free
error:
  call void @__assert_fail()
  br label %exit
free:
  call void @free(i8* %p)
  br label %exit
exit:
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

BOOST_AUTO_TEST_CASE(Malloc_2){
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.malloc_may_fail = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
define i32 @main(){
  %p = call i8* @malloc(i32 10)
  %pcmp = icmp ne i8* %p, null
  br i1 %pcmp, label %error, label %exit
error:
  call void @free(i8* %p)
  call void @__assert_fail()
  br label %exit
exit:
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

BOOST_AUTO_TEST_CASE(Peterson){
  Configuration conf = DPORDriver_test::get_pso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4
@z = global i32 0, align 4

define i8* @p0(i8* %arg){
  store i32 1, i32* @x, align 4
  store i32 1, i32* @z, align 4
  load i32, i32* @z, align 4
  load i32, i32* @y, align 4
  ret i8* null
}

define i8* @p1(i8* %arg){
  store i32 1, i32* @y, align 4
  store i32 1, i32* @z, align 4
  load i32, i32* @z, align 4
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@p1, i8* null)
  call i8* @p0(i8* null)
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);
  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0; CPid U0x = P0.aux(0), U0z = P0.aux(1);
  CPid P1 = P0.spawn(0); CPid U1y = P1.aux(0), U1z = P1.aux(1);
  IID<CPid>
    ux0(U0x,1), uz0(U0z,1), rz0(P0,5), ry0(P0,6),
    uy1(U1y,1), uz1(U1z,1), rz1(P1,3), rx1(P1,4);
  DPORDriver_test::trace_set_spec expected =
    {{{ux0,rx1},{uz0,uz1},{rz0,uz1},{ry0,uy1}},
     {{ux0,rx1},{uz0,uz1},{uz1,rz0},{ry0,uy1}},
     {{ux0,rx1},{uz1,uz0},{rz1,uz0},{ry0,uy1}},
     {{ux0,rx1},{uz1,uz0},{uz0,rz1},{ry0,uy1}},

     {{ux0,rx1},{uz0,uz1},{rz0,uz1},{uy1,ry0}},
     {{ux0,rx1},{uz0,uz1},{uz1,rz0},{uy1,ry0}},
     {{ux0,rx1},{uz1,uz0},{rz1,uz0},{uy1,ry0}},
     {{ux0,rx1},{uz1,uz0},{uz0,rz1},{uy1,ry0}},

     {{rx1,ux0},{uz0,uz1},{rz0,uz1},{ry0,uy1}},
     {{rx1,ux0},{uz0,uz1},{uz1,rz0},{ry0,uy1}},
     {{rx1,ux0},{uz1,uz0},{rz1,uz0},{ry0,uy1}},
     {{rx1,ux0},{uz1,uz0},{uz0,rz1},{ry0,uy1}},

     {{rx1,ux0},{uz0,uz1},{rz0,uz1},{uy1,ry0}},
     {{rx1,ux0},{uz0,uz1},{uz1,rz0},{uy1,ry0}},
     {{rx1,ux0},{uz1,uz0},{rz1,uz0},{uy1,ry0}},
     {{rx1,ux0},{uz1,uz0},{uz0,rz1},{uy1,ry0}},
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Peterson_nop){
  Configuration conf = DPORDriver_test::get_pso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4
@z = global i32 0, align 4

define i8* @p0(i8* %arg){
  store i32 1, i32* @x, align 4
  store i32 1, i32* @z, align 4
  load i32, i32* @z, align 4
  load i32, i32* @y, align 4
  ret i8* null
}

define i8* @p1(i8* %arg){
  %foo = add i32 0, 0
  store i32 1, i32* @y, align 4
  store i32 1, i32* @z, align 4
  load i32, i32* @z, align 4
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@p1, i8* null)
  call i8* @p0(i8* null)
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);
  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0; CPid U0x = P0.aux(0), U0z = P0.aux(1);
  CPid P1 = P0.spawn(0); CPid U1y = P1.aux(0), U1z = P1.aux(1);
  IID<CPid>
    ux0(U0x,1), uz0(U0z,1), rz0(P0,5), ry0(P0,6),
    uy1(U1y,1), uz1(U1z,1), rz1(P1,4), rx1(P1,5);
  DPORDriver_test::trace_set_spec expected =
    {{{ux0,rx1},{uz0,uz1},{rz0,uz1},{ry0,uy1}},
     {{ux0,rx1},{uz0,uz1},{uz1,rz0},{ry0,uy1}},
     {{ux0,rx1},{uz1,uz0},{rz1,uz0},{ry0,uy1}},
     {{ux0,rx1},{uz1,uz0},{uz0,rz1},{ry0,uy1}},

     {{ux0,rx1},{uz0,uz1},{rz0,uz1},{uy1,ry0}},
     {{ux0,rx1},{uz0,uz1},{uz1,rz0},{uy1,ry0}},
     {{ux0,rx1},{uz1,uz0},{rz1,uz0},{uy1,ry0}},
     {{ux0,rx1},{uz1,uz0},{uz0,rz1},{uy1,ry0}},

     {{rx1,ux0},{uz0,uz1},{rz0,uz1},{ry0,uy1}},
     {{rx1,ux0},{uz0,uz1},{uz1,rz0},{ry0,uy1}},
     {{rx1,ux0},{uz1,uz0},{rz1,uz0},{ry0,uy1}},
     {{rx1,ux0},{uz1,uz0},{uz0,rz1},{ry0,uy1}},

     {{rx1,ux0},{uz0,uz1},{rz0,uz1},{uy1,ry0}},
     {{rx1,ux0},{uz0,uz1},{uz1,rz0},{uy1,ry0}},
     {{rx1,ux0},{uz1,uz0},{rz1,uz0},{uy1,ry0}},
     {{rx1,ux0},{uz1,uz0},{uz0,rz1},{uy1,ry0}},
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Atomic_1){
  Configuration conf = DPORDriver_test::get_pso_conf();
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

BOOST_AUTO_TEST_CASE(Atomic_4){
  /* Atomic functions act as full fences */
  Configuration conf = DPORDriver_test::get_pso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 1, i32* @y, align 4
  call void @__VERIFIER_atomic_fence()
  load i32, i32* @x, align 4
  ret i8* null
}

define void @__VERIFIER_atomic_fence(){
  ret void
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  store i32 1, i32* @x, align 4
  call void @__VERIFIER_atomic_fence()
  load i32, i32* @y, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare void @__assert_fail()
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0; CPid U0 = P0.aux(0);
  CPid P1 = P0.spawn(0); CPid U1 = P1.aux(0);
  IID<CPid>
    ux0(U0,1), ry0(P0,4),
    uy1(U1,1), rx1(P1,3);
  DPORDriver_test::trace_set_spec expected =
    {{{ux0,rx1},{ry0,uy1}},
     {{ux0,rx1},{uy1,ry0}},
     {{rx1,ux0},{uy1,ry0}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Atomic_8){
  /* Read depending on read depending on store, all inside atomic
   * function. */
  Configuration conf = DPORDriver_test::get_pso_conf();
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

  CPid P0; CPid P1 = P0.spawn(0); CPid U1x = P1.aux(0);
  IID<CPid> x0(P0,2), x1(U1x,1);
  DPORDriver_test::trace_set_spec expected =
    {{{x0,x1}},{{x1,x0}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Atomic_9){
  Configuration conf;
  conf.memory_model = Configuration::SC;
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

BOOST_AUTO_TEST_CASE(Atomic_and_nonatomic_writes){
  Configuration conf = DPORDriver_test::get_pso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @ax(i8* %arg){
  store atomic i32 1, i32* @x seq_cst, align 4
  ret i8* null
}

define i8* @awx(i8* %arg){
  store atomic i32 1, i32* @x seq_cst, align 4
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @awx, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @awx, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @ax, i8* null)
  ret i32 0
}

%attr_t = type{i64,[48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());

  CPid P0;
  CPid P1 = P0.spawn(0); CPid U1 = P1.aux(0);
  CPid P2 = P0.spawn(1); CPid U2 = P2.aux(0);
  CPid P3 = P0.spawn(2);
  IID<CPid> ax1(P1,1), ux1(U1,1),
    ax2(P2,1), ux2(U2,1),
    ax3(P3,1);
  DPORDriver_test::trace_set_spec expected =
    {{{ux1,ax2},{ux2,ax3}},
     {{ax1,ax2},{ax2,ux1},{ux1,ux2},{ux2,ax3}},
     {{ax1,ax2},{ux2,ux1},{ux1,ax3}},
     {{ax2,ax1},{ux1,ux2},{ux2,ax3}},
     {{ax2,ax1},{ax1,ux2},{ux2,ux1},{ux1,ax3}},
     {{ux2,ax1},{ux1,ax3}},

     {{ux1,ax2},{ax2,ax3},{ax3,ux2}},
     {{ax1,ax2},{ax2,ux1},{ux1,ux2},{ux1,ax3},{ax3,ux2}},
     {{ax1,ax2},{ux2,ux1},{ux2,ax3},{ax3,ux1}},
     {{ax2,ax1},{ux1,ux2},{ux1,ax3},{ax3,ux2}},
     {{ax2,ax1},{ax1,ux2},{ux2,ux1},{ux2,ax3},{ax3,ux1}},
     {{ux2,ax1},{ax1,ax3},{ax3,ux1}},

     {{ux1,ax2},{ux1,ax3},{ax3,ax2}},
     {{ax1,ax2},{ax2,ux1},{ux1,ux2},{ax2,ax3},{ax3,ux1}},
     {{ax1,ax2},{ux2,ux1},{ax2,ax3},{ax3,ux2}},
     {{ax2,ax1},{ux1,ux2},{ax1,ax3},{ax3,ux1}},
     {{ax2,ax1},{ax1,ux2},{ux2,ux1},{ax1,ax3},{ax3,ux2}},
     {{ux2,ax1},{ux2,ax3},{ax3,ax1}},

     {{ux1,ax2},{ax1,ax3},{ax3,ux1}},
     {{ax1,ax2},{ax2,ux1},{ux1,ux2},{ax1,ax3},{ax3,ax2}},
     {{ax1,ax2},{ux2,ux1},{ax1,ax3},{ax3,ax2}},
     {{ax2,ax1},{ux1,ux2},{ax2,ax3},{ax3,ax1}},
     {{ax2,ax1},{ax1,ux2},{ux2,ux1},{ax2,ax3},{ax3,ax1}},
     {{ux2,ax1},{ax2,ax3},{ax3,ux2}},

     {{ux1,ax2},{ax3,ax1}},
     {{ax1,ax2},{ax2,ux1},{ux1,ux2},{ax3,ax1}},
     {{ax1,ax2},{ux2,ux1},{ax3,ax1}},
     {{ax2,ax1},{ux1,ux2},{ax3,ax2}},
     {{ax2,ax1},{ax1,ux2},{ux2,ux1},{ax3,ax2}},
     {{ux2,ax1},{ax3,ax2}},
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(fib_simple){
  Configuration conf = DPORDriver_test::get_pso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@i = global i32 1, align 4
@j = global i32 1, align 4

define i8* @p1(i8* %arg){
  %1 = load i32, i32* @i, align 4
  %2 = load i32, i32* @j, align 4
  %3 = add i32 %1, %2
  store i32 %3, i32* @i, align 4
  ret i8* null
}

define i8* @p2(i8* %arg){
  %1 = load i32, i32* @i, align 4
  %2 = load i32, i32* @j, align 4
  %3 = add i32 %1, %2
  store i32 %3, i32* @j, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p2, i8* null)

  %3 = load i32, i32* @i, align 4
  %4 = load i32, i32* @j, align 4

  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0;
  CPid P1 = P0.spawn(0); CPid U1 = P1.aux(0);
  CPid P2 = P0.spawn(1); CPid U2 = P2.aux(0);

  IID<CPid>
    ri0(P0,3), rj0(P0,4),
    ri1(P1,1), rj1(P1,2), ui1(U1,1),
    ri2(P2,1), rj2(P2,2), uj2(U2,1);

  DPORDriver_test::trace_set_spec expected =
    {{{rj1,uj2},{ui1,ri2},{ri0,ui1},{rj0,uj2}},
     {{rj1,uj2},{ui1,ri2},{ri0,ui1},{uj2,rj0}},
     {{rj1,uj2},{ui1,ri2},{ui1,ri0},{rj0,uj2}},
     {{rj1,uj2},{ui1,ri2},{ui1,ri0},{uj2,rj0}},
     {{rj1,uj2},{ri2,ui1},{ri0,ui1},{rj0,uj2}},
     {{rj1,uj2},{ri2,ui1},{ri0,ui1},{uj2,rj0}},
     {{rj1,uj2},{ri2,ui1},{ui1,ri0},{rj0,uj2}},
     {{rj1,uj2},{ri2,ui1},{ui1,ri0},{uj2,rj0}},
     /* Impossible (uj2 < rj1 < ui1 < ri2 < uj2)
     {{uj2,rj1},{ui1,ri2},{ri0,ui1},{rj0,uj2}},
     {{uj2,rj1},{ui1,ri2},{ri0,ui1},{uj2,rj0}},
     {{uj2,rj1},{ui1,ri2},{ui1,ri0},{rj0,uj2}},
     {{uj2,rj1},{ui1,ri2},{ui1,ri0},{uj2,rj0}},
     */
     {{uj2,rj1},{ri2,ui1},{ri0,ui1},{rj0,uj2}},
     {{uj2,rj1},{ri2,ui1},{ri0,ui1},{uj2,rj0}},
     /* Impossible (ui1 < ri0 < rj0 < uj2 < rj1 < ui1)
     {{uj2,rj1},{ri2,ui1},{ui1,ri0},{rj0,uj2}},
     */
     {{uj2,rj1},{ri2,ui1},{ui1,ri0},{uj2,rj0}},
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(fib_fragment){
  Configuration conf = DPORDriver_test::get_pso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@i = global i32 0, align 4
@j = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 1, i32* @i, align 4
  ret i8* null
}

define i8* @p2(i8* %arg){
  store i32 1, i32* @j, align 4
  load i32, i32* @j, align 4
  load i32, i32* @i, align 4
  store i32 1, i32* @j, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64*null,%attr_t*null,i8*(i8*)*@p1,i8*null)
  call i32 @pthread_create(i64*null,%attr_t*null,i8*(i8*)*@p2,i8*null)

  load i32, i32* @i, align 4
  load i32, i32* @j, align 4

  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0;
  CPid P1 = P0.spawn(0); CPid U1 = P1.aux(0);
  CPid P2 = P0.spawn(1); CPid U2 = P2.aux(0);
  IID<CPid>
    ri0(P0,3), rj0(P0,4),
    wi1(U1,1),
    wj2a(U2,1), wj2b(U2,2),
    ri2(P2,3);
  DPORDriver_test::trace_set_spec expected =
    {{{rj0,wj2a}, {ri0,wi1}, {ri2,wi1}},
     {{rj0,wj2a}, {ri0,wi1}, {wi1,ri2}},
     {{rj0,wj2a}, {wi1,ri0}, {ri2,wi1}},
     {{rj0,wj2a}, {wi1,ri0}, {wi1,ri2}},

     {{wj2a,rj0},{rj0,wj2b}, {ri0,wi1}, {ri2,wi1}},
     {{wj2a,rj0},{rj0,wj2b}, {ri0,wi1}, {wi1,ri2}},
     {{wj2a,rj0},{rj0,wj2b}, {wi1,ri0}, {ri2,wi1}},
     {{wj2a,rj0},{rj0,wj2b}, {wi1,ri0}, {wi1,ri2}},

     {{wj2b,rj0}, {ri0,wi1}, {ri2,wi1}},
     {{wj2b,rj0}, {ri0,wi1}, {wi1,ri2}},
     {{wj2b,rj0}, {wi1,ri0}, {ri2,wi1}},
     {{wj2b,rj0}, {wi1,ri0}, {wi1,ri2}}
    };

  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Wakeup_bug_1){
  /* Regression test for a bug where wakeup would insert erroneous
   * pids for auxiliary threads.
   */
  Configuration conf = DPORDriver_test::get_pso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@flag1 = global i32 0, align 4
@flag2 = global i32 0, align 4
@turn = global i32 0, align 4
@x = common global i32 0, align 4

define noalias i8* @thr1(i8* nocapture %arg) nounwind uwtable {
  store volatile i32 1, i32* @flag1, align 4
  fence seq_cst
  store volatile i32 0, i32* @x, align 4
  store volatile i32 1, i32* @turn, align 4
  store volatile i32 0, i32* @flag1, align 4
  ret i8* null
}

define i32 @main() nounwind uwtable {
  call i32 @pthread_create(i64*null, %attr_t* null, i8* (i8*)* @thr1, i8* null) nounwind
  %f1 = load volatile i32, i32* @flag1, align 4
  %cf1 = icmp sgt i32 %f1, 0
  br i1 %cf1, label %A, label %B

A:
  load volatile i32, i32* @turn, align 4
  br label %B

B:
  load volatile i32, i32* @flag1, align 4
  store volatile i32 1, i32* @turn, align 4
  store volatile i32 0, i32* @flag2, align 4
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8* (i8*)*, i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0; CPid P0turn = P0.aux(0);
  CPid P1 = P0.spawn(0);
  CPid P1flag1 = P1.aux(0); CPid P1turn = P1.aux(2);

  IID<CPid>
    p0rf0(P0,2), p0rt(P0,5), p0rf1a(P0,7), p0rf1b(P0,5),
    p0ut(P0turn,1),
    p1uf0(P1flag1,1), p1uf1(P1flag1,2), p1ut(P1turn,1);
  DPORDriver_test::trace_set_spec expected =
    {
      /* Skip block A */
      {{p0rf0,p1uf0},{p0rf1b,p1uf0},{p0ut,p1ut}},
      {{p0rf0,p1uf0},{p0rf1b,p1uf0},{p1ut,p0ut}},
      {{p0rf0,p1uf0},{p1uf0,p0rf1b},{p0rf1b,p1uf1},{p0ut,p1ut}},
      {{p0rf0,p1uf0},{p1uf0,p0rf1b},{p0rf1b,p1uf1},{p1ut,p0ut}},
      {{p0rf0,p1uf0},{p1uf1,p0rf1b},{p0ut,p1ut}},
      {{p0rf0,p1uf0},{p1uf1,p0rf1b},{p1ut,p0ut}},

      /* Skip block A */
      {{p1uf1,p0rf0},{p0ut,p1ut}},
      {{p1uf1,p0rf0},{p1ut,p0ut}},

      /* Enter block A */
      {{p1uf0,p0rf0},{p0rf0,p1uf1},{p0rf1a,p1uf1},{p1ut,p0rt}},
      {{p1uf0,p0rf0},{p0rf0,p1uf1},{p1uf1,p0rf1a},{p1ut,p0rt}},
      {{p1uf0,p0rf0},{p0rf0,p1uf1},{p0rf1a,p1uf1},{p0rt,p1ut},{p1ut,p0ut}},
      {{p1uf0,p0rf0},{p0rf0,p1uf1},{p1uf1,p0rf1a},{p0rt,p1ut},{p1ut,p0ut}},
      {{p1uf0,p0rf0},{p0rf0,p1uf1},{p0rf1a,p1uf1},{p0ut,p1ut}},
      {{p1uf0,p0rf0},{p0rf0,p1uf1},{p1uf1,p0rf1a},{p0ut,p1ut}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Wakeup_bug_2){
  /* A variation on Wakeup_bug_1. */
  Configuration conf = DPORDriver_test::get_pso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@flag1 = global i32 0, align 4
@flag2 = global i32 0, align 4
@turn = global i32 0, align 4
@x = common global i32 0, align 4

define noalias i8* @thr1(i8* nocapture %arg) nounwind uwtable {
  store volatile i32 1, i32* @flag1, align 4
  fence seq_cst
  store volatile i32 0, i32* @x, align 4
  store volatile i32 1, i32* @turn, align 4
  store volatile i32 0, i32* @flag1, align 4
  ret i8* null
}

define i32 @main() nounwind uwtable {
  call i32 @pthread_create(i64*null, %attr_t* null, i8* (i8*)* @thr1, i8* null) nounwind
  load volatile i32, i32* @flag1, align 4
  %t = load volatile i32, i32* @turn, align 4
  %ct = icmp eq i32 %t, 1
  br i1 %ct, label %A, label %thr2.exit

A:
  %f = load volatile i32, i32* @flag1, align 4
  %cf = icmp sgt i32 %f, 0
  br i1 %cf, label %thr2.exit, label %B

B:
  store volatile i32 1, i32* @turn, align 4
  store volatile i32 0, i32* @flag2, align 4
  br label %thr2.exit

thr2.exit:
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8* (i8*)*, i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0; CPid P0turn = P0.aux(0);
  CPid P1 = P0.spawn(0), P1flag1 = P1.aux(0), P1turn = P1.aux(2);
  IID<CPid> p0rf0(P0,2), p0rt(P0,3), p0rf1(P0,6), p0ut(P0turn,1),
    p1uf0(P1flag1,1), p1uf1(P1flag1,2), p1ut(P1turn,1);

  DPORDriver_test::trace_set_spec expected =
    {
      /* Skip A block */
      {{p0rt,p1ut},{p0rf0,p1uf0}},
      {{p0rt,p1ut},{p1uf0,p0rf0},{p0rf0,p1uf1}},
      {{p0rt,p1ut},{p1uf1,p0rf0}},

      /* Enter A block, skip B block */
      {{p1ut,p0rt},{p0rf1,p1uf1},{p0rf0,p1uf0}},
      {{p1ut,p0rt},{p0rf1,p1uf1},{p1uf0,p0rf0}},

      /* Enter A block, enter B block */
      {{p1ut,p0rt},{p1uf1,p0rf1},{p0rf0,p1uf0}},
      {{p1ut,p0rt},{p1uf1,p0rf1},{p1uf0,p0rf0},{p0rf0,p1uf1}},
      {{p1ut,p0rt},{p1uf1,p0rf0}}
    };

  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Dealloc_buffer_id){
  Configuration conf = DPORDriver_test::get_pso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define void @foo(){
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %c = alloca i32, align 4
  store i32 1, i32* %a, align 4
  store i32 1, i32* %b, align 4
  store i32 1, i32* %c, align 4
  ret void
}

define i8* @p1(i8* %arg){
  store atomic i32 1, i32* @x seq_cst, align 4
  call void @foo()
  store atomic i32 1, i32* @x seq_cst, align 4
  call void @foo()
  store atomic i32 1, i32* @x seq_cst, align 4
  call void @foo()
  store atomic i32 1, i32* @x seq_cst, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64*null, %attr_t*null, i8*(i8*)*@p1, i8*null)
  store atomic i32 1, i32* @x seq_cst, align 4
  call void @foo()
  store atomic i32 1, i32* @x seq_cst, align 4
  call void @foo()
  store atomic i32 1, i32* @x seq_cst, align 4
  call void @foo()
  store atomic i32 1, i32* @x seq_cst, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;
}

BOOST_AUTO_TEST_CASE(Bitcast_rowe_1){
  Configuration conf = DPORDriver_test::get_pso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@p = global i32* null, align 8
@i = global i32 0, align 4

; These variables are only to force ROWE
@x = global i32 0, align 4
@y = global i32 0, align 4

; This thread exists only to force ROWE in main function
define i8* @p1(i8* %arg){
  store atomic i32 1, i32* @y seq_cst, align 4
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@p1, i8* null)
  store i32 1, i32* @x, align 4
  store i32* @i, i32** @p, align 8
  %pp64 = bitcast i32** @p to i64* ; Assume that pointers are 64 bit
  %p64 = load i64, i64* %pp64, align 8 ; ROWE
  load i32, i32* @y, align 4

  %pi64 = ptrtoint i32* @i to i64
  %pcmp = icmp eq i64 %pi64, %p64
  br i1 %pcmp, label %exit, label %error

error:
  call void @__assert_fail()
  br label %exit

exit:
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Bitcast_rowe_2){
  Configuration conf = DPORDriver_test::get_pso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@p = global i32* null, align 8
@i = global i32 0, align 4
@j = global i32 0, align 4

; This variable is only to force ROWE
@y = global i32 0, align 4

; This thread exists only to force ROWE in main function
define i8* @p1(i8* %arg){
  store atomic i32 1, i32* @y seq_cst, align 4
  load i32*, i32** @p, align 8
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@p1, i8* null)
  store i32* @i, i32** @p, align 8
  %pp64 = bitcast i32** @p to i64* ; Assume that pointers are 64 bit
  %p64 = load i64, i64* %pp64, align 8 ; ROWE
  load i32, i32* @y, align 4

  %pi64 = ptrtoint i32* @i to i64
  %pcmp = icmp eq i64 %pi64, %p64
  br i1 %pcmp, label %exit, label %error

error:
  call void @__assert_fail()
  br label %exit

exit:
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Full_conflict_1){
  /* This test assumes that memcpy has full memory conflict. If that
   * is no longer the case, replace the call to memcpy with any
   * function that does have a full memory conflict.
   */
  Configuration conf = DPORDriver_test::get_pso_conf();
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
  Configuration conf = DPORDriver_test::get_pso_conf();
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
  Configuration conf = DPORDriver_test::get_pso_conf();
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
  Configuration conf = DPORDriver_test::get_pso_conf();
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

  CPid P0; CPid P0y = P0.aux(0);
  CPid P1 = P0.spawn(0); CPid P1x = P1.aux(0);
  IID<CPid> p1ux1(P1x,1), p1ux2(P1x,2), p1mc(P1,3),
    p0mc(P0,2), p0uy(P0y,1);
  DPORDriver_test::trace_set_spec expected =
    {
      {{p0mc,p1ux1},{p1mc,p0uy}},
      {{p0mc,p1ux1},{p0uy,p1mc}},
      {{p1ux1,p0mc},{p0mc,p1ux2},{p1mc,p0uy}},
      {{p1ux1,p0mc},{p0mc,p1ux2},{p0uy,p1mc}},
      {{p1ux2,p0mc},{p0mc,p1mc},{p1mc,p0uy}},
      {{p1ux2,p0mc},{p0mc,p1mc},{p0uy,p1mc}},
      {{p1mc,p0mc}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Full_conflict_6){
  /* This test assumes that memcpy has full memory conflict. If that
   * is no longer the case, replace the call to memcpy with any
   * function that does have a full memory conflict.
   */
  Configuration conf = DPORDriver_test::get_pso_conf();
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
  CPid P1x = P1.aux(0);
  IID<CPid> p1mc(P1,1), p1ux(P1x,1), p0rx(P0,2);
  DPORDriver_test::trace_set_spec expected =
    {
      {{p0rx,p1mc}},
      {{p1mc,p0rx},{p0rx,p1ux}},
      {{p1ux,p0rx}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Full_conflict_7){
  /* This test assumes that memcpy has full memory conflict. If that
   * is no longer the case, replace the call to memcpy with any
   * function that does have a full memory conflict.
   */
  Configuration conf = DPORDriver_test::get_pso_conf();
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
  CPid P0x = P0.aux(0);
  IID<CPid> p0mc(P0,2), p0ux(P0x,1), p1rx(P1,1);
  DPORDriver_test::trace_set_spec expected =
    {
      {{p1rx,p0mc}},
      {{p0mc,p1rx},{p1rx,p0ux}},
      {{p0ux,p1rx}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Full_conflict_8){
  /* This test assumes that memcpy has full memory conflict. If that
   * is no longer the case, replace the call to memcpy with any
   * function that does have a full memory conflict.
   */
  Configuration conf = DPORDriver_test::get_pso_conf();
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
  Configuration conf = DPORDriver_test::get_pso_conf();
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
  Configuration conf = DPORDriver_test::get_pso_conf();
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

BOOST_AUTO_TEST_CASE(Overlapping_ROWE_1){
  /* A load l of bytes B(l) where the latest preceding store which
   , l of bytes B(l) where the latest preceding store which
   * accesses some bytes in B(l) does not access precisely the bytes
   * in B(l), will block until that store has updated to memory. This
   * is intended to be a safe under-approximation of PSO behaviour.
   */
  Configuration conf = DPORDriver_test::get_pso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p0(i8* %arg){
  store i32 1, i32* @x, align 4
  %xb = bitcast i32* @x to i8*
  load i8, i8* %xb
  load i32, i32* @y, align 4
  ret i8* null
}

define i8* @p1(i8* %arg){
  store i32 1, i32* @y, align 4
  %yb = bitcast i32* @y to i8*
  load i8, i8* %yb
  load i32, i32* @x, align 4
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

  CPid P0, P1 = P0.spawn(0), P2 = P0.spawn(1);
  CPid U1 = P1.aux(0), U2 = P2.aux(0);
  IID<CPid> ux1(U1,1), uy2(U2,1), ry1(P1,4), rx2(P2,4);
  DPORDriver_test::trace_set_spec expected =
    {{{ux1,rx2},{ry1,uy2}},
     {{ux1,rx2},{uy2,ry1}},
     {{rx2,ux1},{uy2,ry1}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Overlapping_ROWE_2){
  /* A load l of bytes B(l) where the latest preceding store which
   , l of bytes B(l) where the latest preceding store which
   * accesses some bytes in B(l) does not access precisely the bytes
   * in B(l), will block until that store has updated to memory. This
   * is intended to be a safe under-approximation of PSO behaviour.
   */
  Configuration conf = DPORDriver_test::get_pso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p0(i8* %arg){
  store i32 1, i32* @x, align 4
  store i32 2, i32* @x, align 4
  %xb = bitcast i32* @x to i8*
  load i8, i8* %xb
  load i32, i32* @y, align 4
  ret i8* null
}

define i8* @p1(i8* %arg){
  store i32 1, i32* @y, align 4
  store i32 2, i32* @y, align 4
  %yb = bitcast i32* @y to i8*
  load i8, i8* %yb
  load i32, i32* @x, align 4
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

  CPid P0, P1 = P0.spawn(0), P2 = P0.spawn(1);
  CPid U1 = P1.aux(0), U2 = P2.aux(0);
  IID<CPid> ux1a(U1,1), ux1b(U1,2), uy2a(U2,1), uy2b(U2,2),
    ry1(P1,5), rx2(P2,5);
  DPORDriver_test::trace_set_spec expected =
    {{{ux1b,rx2},{ry1,uy2a}},
     {{ux1b,rx2},{uy2a,ry1},{ry1,uy2b}},
     {{ux1b,rx2},{uy2b,ry1}},

     {{ux1a,rx2},{rx2,ux1b},{uy2b,ry1}},
     {{rx2,ux1a},{uy2b,ry1}},
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Overlapping_ROWE_3){
  /* A load l of bytes B(l) where the latest preceding store which
   , l of bytes B(l) where the latest preceding store which
   * accesses some bytes in B(l) does not access precisely the bytes
   * in B(l), will block until that store has updated to memory. This
   * is intended to be a safe under-approximation of PSO behaviour.
   */
  Configuration conf = DPORDriver_test::get_pso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p0(i8* %arg){
  store i32 1, i32* @x, align 4
  %xb = bitcast i32* @x to i8*
  store i8 2, i8* %xb
  load i8, i8* %xb
  load i32, i32* @y, align 4
  ret i8* null
}

define i8* @p1(i8* %arg){
  store i32 1, i32* @y, align 4
  %yb = bitcast i32* @y to i8*
  store i8 2, i8* %yb
  load i8, i8* %yb
  load i32, i32* @x, align 4
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

  CPid P0, P1 = P0.spawn(0), P2 = P0.spawn(1);
  CPid U1 = P1.aux(0), U2 = P2.aux(0);
  IID<CPid> ux1a(U1,1), ux1b(U1,2), uy2a(U2,1), uy2b(U2,2),
    ry1(P1,5), rx2(P2,5);
  DPORDriver_test::trace_set_spec expected =
    {{{ux1b,rx2},{ry1,uy2a}},
     {{ux1b,rx2},{uy2a,ry1},{ry1,uy2b}},
     {{ux1b,rx2},{uy2b,ry1}},

     {{ux1a,rx2},{rx2,ux1b},{ry1,uy2a}},
     {{ux1a,rx2},{rx2,ux1b},{uy2a,ry1},{ry1,uy2b}},
     {{ux1a,rx2},{rx2,ux1b},{uy2b,ry1}},

     {{rx2,ux1a},{ry1,uy2a}},
     {{rx2,ux1a},{uy2a,ry1},{ry1,uy2b}},
     {{rx2,ux1a},{uy2b,ry1}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Overlapping_ROWE_4){
  /* A load l of bytes B(l) where the latest preceding store which
   , l of bytes B(l) where the latest preceding store which
   * accesses some bytes in B(l) does not access precisely the bytes
   * in B(l), will block until that store has updated to memory. This
   * is intended to be a safe under-approximation of PSO behaviour.
   */
  Configuration conf = DPORDriver_test::get_pso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p0(i8* %arg){
  %xb = bitcast i32* @x to i8*
  %xb1 = getelementptr i8, i8* %xb, i32 1
  store i8 2, i8* %xb1
  load i32, i32* @x, align 4
  load i32, i32* @y, align 4
  ret i8* null
}

define i8* @p1(i8* %arg){
  %yb = bitcast i32* @y to i8*
  %yb1 = getelementptr i8, i8* %yb, i32 1
  store i8 1, i8* %yb1
  load i32, i32* @y, align 4
  load i32, i32* @x, align 4
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

  CPid P0, P1 = P0.spawn(0), P2 = P0.spawn(1);
  CPid U1 = P1.aux(0), U2 = P2.aux(0);
  IID<CPid> ux1(U1,1), uy2(U2,1),
    ry1(P1,5), rx2(P2,5);
  DPORDriver_test::trace_set_spec expected =
    {{{ux1,rx2},{ry1,uy2}},
     {{ux1,rx2},{uy2,ry1}},
     {{rx2,ux1},{uy2,ry1}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Overlapping_ROWE_5){
  /* A load l of bytes B(l) where the latest preceding store which
   , l of bytes B(l) where the latest preceding store which
   * accesses some bytes in B(l) does not access precisely the bytes
   * in B(l), will block until that store has updated to memory. This
   * is intended to be a safe under-approximation of PSO behaviour.
   */
  Configuration conf = DPORDriver_test::get_pso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p0(i8* %arg){
  %xb = bitcast i32* @x to i8*
  %xb1 = getelementptr i8, i8* %xb, i32 1
  store i8 2, i8* %xb1
  load i8, i8* %xb
  load i8, i8* %xb1
  load i32, i32* @y, align 4
  ret i8* null
}

define i8* @p1(i8* %arg){
  %yb = bitcast i32* @y to i8*
  %yb1 = getelementptr i8, i8* %yb, i32 1
  store i8 1, i8* %yb1
  load i8, i8* %yb
  load i8, i8* %yb1
  load i32, i32* @x, align 4
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

  CPid P0, P1 = P0.spawn(0), P2 = P0.spawn(1);
  CPid U1 = P1.aux(0), U2 = P2.aux(0);
  IID<CPid> ux1(U1,1), uy2(U2,1),
    ry1(P1,6), rx2(P2,6);
  DPORDriver_test::trace_set_spec expected =
    {{{ux1,rx2},{ry1,uy2}},
     {{ux1,rx2},{uy2,ry1}},
     {{rx2,ux1},{uy2,ry1}},
     {{rx2,ux1},{ry1,uy2}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_SUITE_END()

#endif
