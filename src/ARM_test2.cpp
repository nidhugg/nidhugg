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

#include "Debug.h"
#include "DPORDriver.h"
#include "DPORDriver_test.h"
#include "StrModule.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(ARM_test)

BOOST_AUTO_TEST_CASE(TWO_2W_overlapping_1){
  Configuration conf = DPORDriver_test::get_arm_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p(i8* %arg){
  store i32 4294967295, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  %x0 = getelementptr i8, i8* bitcast (i32* @x to i8*), i32 0
  %x1 = getelementptr i8, i8* bitcast (i32* @x to i8*), i32 1
  store i8 1, i8* %x0, align 4
  call void asm sideeffect "lwsync", ""()
  store i8 1, i8* %x1, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 3);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(TWO_2W_overlapping_2){
  Configuration conf = DPORDriver_test::get_arm_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p(i8* %arg){
  store i32 4294967295, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  %x0 = getelementptr i8, i8* bitcast (i32* @x to i8*), i32 1
  %x1 = getelementptr i8, i8* bitcast (i32* @x to i8*), i32 0
  store i8 1, i8* %x0, align 4
  call void asm sideeffect "lwsync", ""()
  store i8 1, i8* %x1, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 3);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(TWO_2W_overlapping_3){
  Configuration conf = DPORDriver_test::get_arm_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p(i8* %arg){
  %x0 = getelementptr i8, i8* bitcast (i32* @x to i8*), i32 1
  %x1 = getelementptr i8, i8* bitcast (i32* @x to i8*), i32 0
  store i8 1, i8* %x0, align 4
  call void asm sideeffect "lwsync", ""()
  store i8 1, i8* %x1, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  store i32 4294967295, i32* @x, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 3);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(TWO_2W_overlapping_4){
  Configuration conf = DPORDriver_test::get_arm_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p(i8* %arg){
  %x0 = getelementptr i8, i8* bitcast (i32* @x to i8*), i32 0
  %x1 = getelementptr i8, i8* bitcast (i32* @x to i8*), i32 1
  store i8 1, i8* %x0, align 4
  call void asm sideeffect "lwsync", ""()
  store i8 1, i8* %x1, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  store i32 4294967295, i32* @x, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 3);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(TWO_W){
  Configuration conf = DPORDriver_test::get_arm_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p(i8* %arg){
  %x1 = getelementptr i8, i8* bitcast (i32* @x to i8*), i32 1
  store i8 2, i8* %x1
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  %x0 = getelementptr i8, i8* bitcast (i32* @x to i8*), i32 0
  store i8 1, i8* %x0
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 1);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(TWO_W_2R){
  /* a:wx    b:wy    c:r{x,y}    d:r{x,y}
   *
   * Forbidden: a -rf-> c -fr-> b -rf-> d -fr-> a
   * Forbidden: b -rf-> c -fr-> a -rf-> d -fr-> b
   *
   * Both are forbidden due to a cycle in com.
   *
   * (Although the behaviour seems to make sense when you think of an
   * operational semantics where stores are propagated independently
   * to different threads...)
   */
  Configuration conf = DPORDriver_test::get_arm_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p(i8* %arg){
  %x1 = getelementptr i8, i8* bitcast (i32* @x to i8*), i32 1
  store i8 2, i8* %x1
  ret i8* null
}

define i8* @q(i8* %arg){
  load i32, i32* @x
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @q, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @q, i8* null)
  %x0 = getelementptr i8, i8* bitcast (i32* @x to i8*), i32 0
  store i8 1, i8* %x0
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 14);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(MP_lwsync_overlap){
  Configuration conf = DPORDriver_test::get_arm_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p(i8* %arg){
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  %x0 = getelementptr i8, i8* bitcast (i32* @x to i8*), i32 0
  %x1 = getelementptr i8, i8* bitcast (i32* @x to i8*), i32 1
  store i8 1, i8* %x0, align 4
  call void asm sideeffect "lwsync", ""()
  store i8 1, i8* %x1, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 3);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(MP_lwsync_overlap_2){
  Configuration conf = DPORDriver_test::get_arm_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p(i8* %arg){
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  %x0 = getelementptr i8, i8* bitcast (i32* @x to i8*), i32 0
  %x1 = getelementptr i8, i8* bitcast (i32* @x to i8*), i32 1
  store i8 1, i8* %x1, align 4
  call void asm sideeffect "lwsync", ""()
  store i8 1, i8* %x0, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 3);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Create_id_1){
  Configuration conf = DPORDriver_test::get_arm_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
define i8* @p(i8* %arg){
  ret i8* null
}

define i32 @main(){
  %t0 = alloca i64
  %t1 = alloca i64
  call i32 @pthread_create(i64* %t0, %attr_t* null, i8*(i8*)* @p, i8* null)
  call i32 @pthread_create(i64* %t1, %attr_t* null, i8*(i8*)* @p, i8* null)
  %t0v = load i64, i64* %t0
  %t1v = load i64, i64* %t1
  %eq = icmp eq i64 %t0v, %t1v
  br i1 %eq, label %error, label %exit
error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 1);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(LB_polocrfiaddr_lwsync){
  /* Note: this case shows a difference between POWER and ARM. */
  Configuration conf = DPORDriver_test::get_arm_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0
@y = global i32 0

define i8* @p(i8* %arg){
  load i32, i32* @y
  call void asm sideeffect "lwsync", ""()
  store i32 2, i32* @x
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  load i32, i32* @x
  store i32 1, i32* @x
  %x = load i32, i32* @x
  store i32 %x, i32* @y
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 7); // Only 6 under POWER
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(LB_datapoloc_lwsync){
  Configuration conf = DPORDriver_test::get_arm_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i8 0
@y = global i32 0

define i8* @p(i8* %arg){
  %y0addr = bitcast i32* @y to i8*
  load i8, i8* %y0addr
  call void asm sideeffect "lwsync", ""()
  store i8 1, i8* @x
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  %x = load i8, i8* @x
  %y1addr = getelementptr i8, i8* bitcast (i32* @y to i8*), i32 1
  store i8 %x, i8* %y1addr
  store i32 1, i32* @y
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  /* The fourth trace is forbidden by NO THIN AIR and PROPAGATION
   * under POWER. It is forbidden only by PROPAGATION under ARM.
   */
  BOOST_CHECK(res.trace_count == 3);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(MP_double_fence){
  /* Regression test for a bug caused by an off by one error in the
   * handling of fences.
   */
  Configuration conf = DPORDriver_test::get_arm_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0
@y = global i32 0

define i8* @p(i8* %arg){
  call void asm sideeffect "lwsync", ""()
  load i32, i32* @y
  call void asm sideeffect "lwsync", ""()
  load i32, i32* @x
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call void asm sideeffect "lwsync", ""()
  store i32 1, i32* @x
  call void asm sideeffect "lwsync", ""()
  store i32 1, i32* @y
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 3);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Data_rfi_sleepset){
  Configuration conf = DPORDriver_test::get_arm_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0
@y = global i32 0

define i8* @p(i8* %arg){
  store i32 1, i32* @x
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  %x = load i32, i32* @x
  store i32 %x, i32* @y
  load i32, i32* @y
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 2);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Pthread_create_arg_1){
  Configuration conf = DPORDriver_test::get_arm_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
define i8* @p(i8* %arg){
  %i = ptrtoint i8* %arg to i64
  %ic = icmp eq i64 %i, 42
  br i1 %ic, label %exit, label %error
error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i8* null
}

define i32 @main(){
  %i = inttoptr i64 42 to i8*
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* %i)
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare i32 @pthread_join(i64,i8**)
declare void @__assert_fail()
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 1);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Pthread_create_arg_2){
  Configuration conf = DPORDriver_test::get_arm_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
define i8* @p(i8* %arg){
  %i = ptrtoint i8* %arg to i64
  %ic = icmp eq i64 %i, 42
  br i1 %ic, label %error, label %exit
error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i8* null
}

define i32 @main(){
  %i = inttoptr i64 42 to i8*
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* %i)
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare i32 @pthread_join(i64,i8**)
declare void @__assert_fail()
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 1);
  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(Fun_arg_1){
  Configuration conf = DPORDriver_test::get_arm_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0
@y = global i32 0

define i8* @p(i8* %arg){
  %y = load i32, i32* @y
  %y1 = add i32 %y, 1
  store i32 %y1, i32* @x
  ret i8* null
}

define void @f(i32 %x){
  %x1 = add i32 %x, 1
  store i32 1, i32* @y
  ret void
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  %x = load i32, i32* @x
  call void @f(i32 %x)
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare i32 @pthread_join(i64,i8**)
declare void @__assert_fail()
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 4);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Fun_arg_2){
  Configuration conf = DPORDriver_test::get_arm_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0
@y = global i32 0

define i8* @p(i8* %arg){
  %y = load i32, i32* @y
  %y1 = add i32 %y, 1
  store i32 %y1, i32* @x
  ret i8* null
}

define void @f(i32 %x){
  %x1 = add i32 %x, 1
  store i32 %x1, i32* @y
  ret void
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  %x = load i32, i32* @x
  call void @f(i32 %x)
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare i32 @pthread_join(i64,i8**)
declare void @__assert_fail()
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 3);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Mutex_1){
  Configuration conf = DPORDriver_test::get_arm_conf();
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
  BOOST_CHECK(res.trace_count == 2);
}

BOOST_AUTO_TEST_CASE(Mutex_3){
  Configuration conf = DPORDriver_test::get_arm_conf();
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
  BOOST_CHECK(res.trace_count == 3);
}

BOOST_AUTO_TEST_CASE(Mutex_6){
  /* Make sure that we do not miss computations even when some process
   * refuses to release a lock. */
  Configuration conf = DPORDriver_test::get_arm_conf();
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

  BOOST_CHECK(res.trace_count + res.sleepset_blocked_trace_count == 3);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Mutex_7){
  /* Typical use of pthread_mutex_destroy */
  Configuration conf = DPORDriver_test::get_arm_conf();
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
  BOOST_CHECK(res.trace_count == 2);
}

BOOST_AUTO_TEST_CASE(Mutex_9){
  /* Mutex destruction and flag */
  Configuration conf = DPORDriver_test::get_arm_conf();
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
  BOOST_CHECK(res.trace_count == 4);
}

BOOST_AUTO_TEST_CASE(Mutex_10){
  /* Check that critical sections locked with different locks can
   * overlap. */
  Configuration conf = DPORDriver_test::get_arm_conf();
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
  Configuration conf = DPORDriver_test::get_arm_conf();
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
  Configuration conf = DPORDriver_test::get_arm_conf();
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
  BOOST_CHECK(res.trace_count == 6);
}

BOOST_AUTO_TEST_CASE(Pthread_exit_1){
  Configuration conf = DPORDriver_test::get_arm_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

define i8* @p1(i8* %arg){
  call void @pthread_exit(i8* null)
  ret i8* null
}

define i32 @main(){
  %tp = alloca i64
  call i32 @pthread_create(i64* %tp, %attr_t* null, i8*(i8*)* @p1, i8* null)
  %t = load i64, i64* %tp
  call i32 @pthread_join(i64 %t, i8** null)
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_join(i64,i8**) nounwind
declare void @pthread_exit(i8*)
declare void @__assert_fail() noreturn nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
  BOOST_CHECK(res.trace_count == 1);
}

BOOST_AUTO_TEST_CASE(Pthread_exit_2){
  Configuration conf = DPORDriver_test::get_arm_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

@x = global i32 0

define void @f(){
  store i32 1, i32* @x
  call void @pthread_exit(i8* null)
  ret void
}

define i8* @p1(i8* %arg){
  call void @f()
  ret i8* null
}

define i32 @main(){
  %tp = alloca i64
  call i32 @pthread_create(i64* %tp, %attr_t* null, i8*(i8*)* @p1, i8* null)
  load i32, i32* @x
  %t = load i64, i64* %tp
  call i32 @pthread_join(i64 %t, i8** null)
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_join(i64,i8**) nounwind
declare void @pthread_exit(i8*)
declare void @__assert_fail() noreturn nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
  BOOST_CHECK(res.trace_count == 2);
}

BOOST_AUTO_TEST_CASE(Pthread_exit_3){
  Configuration conf = DPORDriver_test::get_arm_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

@x = global i32 0

define void @f(){
  call void @pthread_exit(i8* null)
  store i32 1, i32* @x
  ret void
}

define i8* @p1(i8* %arg){
  call void @f()
  ret i8* null
}

define i32 @main(){
  %tp = alloca i64
  call i32 @pthread_create(i64* %tp, %attr_t* null, i8*(i8*)* @p1, i8* null)
  load i32, i32* @x
  %t = load i64, i64* %tp
  call i32 @pthread_join(i64 %t, i8** null)
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_join(i64,i8**) nounwind
declare void @pthread_exit(i8*)
declare void @__assert_fail() noreturn nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
  BOOST_CHECK(res.trace_count == 1);
}

BOOST_AUTO_TEST_CASE(Pthread_exit_4){
  Configuration conf = DPORDriver_test::get_arm_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

@x = global i32 0

define void @f(){
  call void @pthread_exit(i8* null)
  ret void
}

define i8* @p1(i8* %arg){
  call void @f()
  store i32 1, i32* @x
  ret i8* null
}

define i32 @main(){
  %tp = alloca i64
  call i32 @pthread_create(i64* %tp, %attr_t* null, i8*(i8*)* @p1, i8* null)
  load i32, i32* @x
  %t = load i64, i64* %tp
  call i32 @pthread_join(i64 %t, i8** null)
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_join(i64,i8**) nounwind
declare void @pthread_exit(i8*)
declare void @__assert_fail() noreturn nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
  BOOST_CHECK(res.trace_count == 1);
}

BOOST_AUTO_TEST_CASE(Malloc_free_1){
  Configuration conf = DPORDriver_test::get_arm_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@p = global i8* null

define i8* @p1(i8* %arg){
  %p = load i8*, i8** @p
  store i8 1, i8* %p
  ret i8* null
}

define i32 @main(){
  %p = call i8* @malloc(i32 10)
  store i8* %p, i8** @p
  %tp = alloca i64
  call i32 @pthread_create(i64* %tp, %attr_t* null, i8*(i8*)* @p1, i8* null)
  load i8, i8* %p
  %t = load i64, i64* %tp
  call i32 @pthread_join(i64 %t, i8** null)
  call void @free(i8* %p)
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare i32 @pthread_join(i64,i8**) nounwind
declare void @pthread_exit(i8*)
declare void @__assert_fail() noreturn nounwind
declare i8* @malloc(i32)
declare void @free(i8*)
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
  BOOST_CHECK(res.trace_count == 2);
}

BOOST_AUTO_TEST_CASE(Normalise_1){
  /* Regression test:
   *
   * The nudging of store parameters was done incorrectly in
   * normalise_branch.
   */
  Configuration conf = DPORDriver_test::get_arm_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0
@y = global i32 0

define i8* @p(i8* %_arg){
  store i32 1, i32* @x
  call void asm sideeffect "dmb", ""()
  %x = load i32, i32* @x
  store i32 %x, i32* @y
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  load i32, i32* @y
  load i32, i32* @x
  store i32 2, i32* @x
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
  BOOST_CHECK(res.trace_count == 10);
}

BOOST_AUTO_TEST_CASE(Compiler_fence_SB){
  Configuration conf = DPORDriver_test::get_arm_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  store i32 1, i32* @y, align 4
  call void asm sideeffect "", "~{memory}"()
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  store i32 1, i32* @x, align 4
  call void asm sideeffect "", "~{memory}"()
  load i32, i32* @y, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 4);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_SUITE_END()

#endif
