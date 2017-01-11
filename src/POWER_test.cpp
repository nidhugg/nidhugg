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

#include "DPORDriver.h"
#include "DPORDriver_test.h"
#include "StrModule.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(POWER_test)

BOOST_AUTO_TEST_CASE(Minimal_computation){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
define i32 @main(){
  ret i32 0
}
)"),conf);

  DPORDriver::Result res = driver->run();

  BOOST_CHECK(res.trace_count == 1);

  delete driver;
}

BOOST_AUTO_TEST_CASE(Reordering_box_1){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i32 @main(){
  %a = add i32 1, 2
  %x = load i32, i32* @x, align 4
  store i32 1, i32* @x, align 4
  %b = add i32 3, %a
  %x1 = add i32 %x, 1
  store i32 %x1, i32* @y, align 4
  ret i32 %b
}
)"),conf);

  DPORDriver::Result res = driver->run();

  BOOST_CHECK(res.trace_count == 1);
  BOOST_CHECK(!res.has_errors());

  delete driver;
}

BOOST_AUTO_TEST_CASE(Reordering_box_2){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

define i32 @main(){
  call void @__assert_fail()
  ret i32 0
}

declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();

  BOOST_CHECK(res.trace_count == 1);
  BOOST_CHECK(res.has_errors());

  delete driver;
}

BOOST_AUTO_TEST_CASE(Reordering_box_3){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

define i32 @main(){
one:
  %a = add i32 1, 1
  br label %two

two:
  %b = add i32 %a, 1
  br label %three

three:
  %c = add i32 %a, %b
  call void @__assert_fail()
  ret i32 0
}

declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();

  BOOST_CHECK(res.trace_count == 1);
  BOOST_CHECK(res.has_errors());

  delete driver;
}

BOOST_AUTO_TEST_CASE(Reordering_box_4){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

define i32 @main(){
one:
  %a = add i32 1, 1
  br label %two

two:
  %b = add i32 %a, 1
  br label %three

three:
  %c = add i32 %a, %b
  %cc = icmp eq i32 %c, 5
  br i1 %cc, label %exit, label %error

error:
  call void @__assert_fail()
  br label %exit

exit:
  ret i32 0
}

declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();

  BOOST_CHECK(res.trace_count == 1);
  BOOST_CHECK(!res.has_errors());

  delete driver;
}

BOOST_AUTO_TEST_CASE(Reordering_box_5){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

define i32 @main(){
one:
  %a = add i32 1, 1
  br label %two

two:
  %b = add i32 %a, 1
  br label %three

three:
  %c = add i32 %a, %b
  %cc = icmp eq i32 %c, 6
  br i1 %cc, label %exit, label %error

error:
  call void @__assert_fail()
  br label %exit

exit:
  ret i32 0
}

declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();

  BOOST_CHECK(res.trace_count == 1);
  BOOST_CHECK(res.has_errors());

  delete driver;
}

BOOST_AUTO_TEST_CASE(Reordering_box_6){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

define i32 @main(){
entry:
  br label %head

head:
  %n = phi i32 [0, %entry], [%nnext, %body]
  %sum = phi i32 [0, %entry], [%sumnext, %body]
  %nc = icmp slt i32 %n, 5
  br i1 %nc, label %body, label %check
body:
  %nnext = add i32 %n, 1
  %sumnext = add i32 %nnext, %sum
  br label %head

check:
  %sc = icmp eq i32 %sum, 15
  br i1 %sc, label %exit, label %error

error:
  call void @__assert_fail()
  br label %exit

exit:
  ret i32 0
}

declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();

  BOOST_CHECK(res.trace_count == 1);
  BOOST_CHECK(!res.has_errors());

  delete driver;
}

BOOST_AUTO_TEST_CASE(Reordering_box_7){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

define i32 @main(){
entry:
  br label %head

head:
  %n = phi i32 [0, %entry], [%nnext, %body]
  %sum = phi i32 [0, %entry], [%sumnext, %body]
  %nc = icmp slt i32 %n, 5
  br i1 %nc, label %body, label %check
body:
  %nnext = add i32 %n, 1
  %sumnext = add i32 %nnext, %sum
  br label %head

check:
  %sc = icmp ne i32 %sum, 15
  br i1 %sc, label %exit, label %error

error:
  call void @__assert_fail()
  br label %exit

exit:
  ret i32 0
}

declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();

  BOOST_CHECK(res.trace_count == 1);
  BOOST_CHECK(res.has_errors());

  delete driver;
}

BOOST_AUTO_TEST_CASE(Reordering_box_8){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

define i32 @fib(i32 %n){
  %term = icmp slt i32 %n, 2
  br i1 %term, label %ret1, label %rec

rec:
  %n1 = sub i32 %n, 1
  %n2 = sub i32 %n, 2
  %f1 = call i32 @fib(i32 %n1)
  %f2 = call i32 @fib(i32 %n2)
  %f = add i32 %f1, %f2
  ret i32 %f

ret1:
  ret i32 1
}

define i32 @main(){
  %f = call i32 @fib(i32 5)
  %fc = icmp eq i32 %f, 8
  br i1 %fc, label %exit, label %error
error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i32 0
}

declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();

  BOOST_CHECK(res.trace_count == 1);
  BOOST_CHECK(!res.has_errors());

  delete driver;
}

BOOST_AUTO_TEST_CASE(Reordering_box_9){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

define i32 @fib(i32 %n){
  %term = icmp slt i32 %n, 2
  br i1 %term, label %ret1, label %rec

rec:
  %n1 = sub i32 %n, 1
  %n2 = sub i32 %n, 2
  %f1 = call i32 @fib(i32 %n1)
  %f2 = call i32 @fib(i32 %n2)
  %f = add i32 %f1, %f2
  ret i32 %f

ret1:
  ret i32 1
}

define i32 @main(){
  %f = call i32 @fib(i32 5)
  %fc = icmp ne i32 %f, 8
  br i1 %fc, label %exit, label %error
error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i32 0
}

declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();

  BOOST_CHECK(res.trace_count == 1);
  BOOST_CHECK(res.has_errors());

  delete driver;
}

BOOST_AUTO_TEST_CASE(Reordering_box_10){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

define i32 @main(){
  %a = add i32 1, 1
  switch i32 %a, label %def [i32 1, label %one i32 2, label %two i32 3, label %three]

def:
  br label %error
one:
  br label %error
three:
  br label %error
error:
  call void @__assert_fail()
  br label %two
two:
  ret i32 0
}

declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();

  BOOST_CHECK(res.trace_count == 1);
  BOOST_CHECK(!res.has_errors());

  delete driver;
}

BOOST_AUTO_TEST_CASE(Reordering_box_11){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

define i32 @main(){
  %a = add i32 1, 2
  switch i32 %a, label %def [i32 1, label %one i32 2, label %two i32 3, label %three]

def:
  br label %error
one:
  br label %error
three:
  br label %error
error:
  call void @__assert_fail()
  br label %two
two:
  ret i32 0
}

declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();

  BOOST_CHECK(res.trace_count == 1);
  BOOST_CHECK(res.has_errors());

  delete driver;
}

BOOST_AUTO_TEST_CASE(Reordering_box_12){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

define i32 @main(){
  %a = getelementptr i64, i64* null, i64 1
  %b = getelementptr i64, i64* null, i64 2
  %ai = ptrtoint i64* %a to i64
  %bi = ptrtoint i64* %b to i64
  %d = sub i64 %bi, %ai
  %dc = icmp eq i64 %d, 8
  br i1 %dc, label %cexpr, label %error

cexpr:
  %d2 = sub i64 ptrtoint (i64* getelementptr (i64, i64* null, i64 3) to i64), ptrtoint (i64* getelementptr (i64, i64* null, i64 1) to i64)
  %dc2 = icmp eq i64 %d2, 16
  br i1 %dc2, label %exit, label %error

error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i32 0
}

declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();

  BOOST_CHECK(res.trace_count == 1);
  BOOST_CHECK(!res.has_errors());

  delete driver;
}

BOOST_AUTO_TEST_CASE(Reordering_box_13){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

define i32 @main(){
  %a = getelementptr i64, i64* null, i64 1
  %b = getelementptr i64, i64* null, i64 2
  %ai = ptrtoint i64* %a to i64
  %bi = ptrtoint i64* %b to i64
  %d = sub i64 %bi, %ai
  %dc = icmp eq i64 %d, 8
  br i1 %dc, label %cexpr, label %error

cexpr:
  %d2 = sub i64 ptrtoint (i64* getelementptr (i64, i64* null, i64 3) to i64), ptrtoint (i64* getelementptr (i64, i64* null, i64 1) to i64)
  %dc2 = icmp eq i64 %d2, 17
  br i1 %dc2, label %exit, label %error

error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i32 0
}

declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();

  BOOST_CHECK(res.trace_count == 1);
  BOOST_CHECK(res.has_errors());

  delete driver;
}

BOOST_AUTO_TEST_CASE(Reordering_box_14){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

define i32 @main(){
  %a = getelementptr i64, i64* null, i64 1
  %b = getelementptr i64, i64* null, i64 2
  %ai = ptrtoint i64* %a to i64
  %bi = ptrtoint i64* %b to i64
  %d = sub i64 %bi, %ai
  %dc = icmp eq i64 %d, 16
  br i1 %dc, label %cexpr, label %error

cexpr:
  %d2 = sub i64 ptrtoint (i64* getelementptr (i64, i64* null, i64 3) to i64), ptrtoint (i64* getelementptr (i64, i64* null, i64 1) to i64)
  %dc2 = icmp eq i64 %d2, 16
  br i1 %dc2, label %exit, label %error

error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i32 0
}

declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();

  BOOST_CHECK(res.trace_count == 1);
  BOOST_CHECK(res.has_errors());

  delete driver;
}

BOOST_AUTO_TEST_CASE(Reordering_box_15){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

define i8* @p(i8* %arg){
  ret i8* null
}

define i32 @main(){
  %rv1 = call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  %rv2 = call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  %crv1 = icmp eq i32 %rv1, 0
  br i1 %crv1, label %cmp2, label %error
cmp2:
  %crv2 = icmp eq i32 %rv2, 0
  br i1 %crv2, label %exit, label %error
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

  BOOST_CHECK(res.trace_count == 1);
  BOOST_CHECK(!res.has_errors());

  delete driver;
}

BOOST_AUTO_TEST_CASE(Reordering_box_16){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

define i8* @p(i8* %arg){
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call void @__assert_fail()
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();

  BOOST_CHECK(res.trace_count == 1);
  BOOST_CHECK(res.has_errors());

  delete driver;
}

BOOST_AUTO_TEST_CASE(Reordering_box_17){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

define i8* @p(i8* %arg){
  call void @__assert_fail()
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();

  BOOST_CHECK(res.trace_count == 1);
  BOOST_CHECK(res.has_errors());

  delete driver;
}

BOOST_AUTO_TEST_CASE(Reordering_box_18){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

define i8* @p(i8* %arg){
  %i = ptrtoint i8* %arg to i32
  %ic = icmp eq i32 %i, 42
  br i1 %ic, label %exit, label %error
error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* inttoptr (i32 42 to i8*))
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();

  BOOST_CHECK(res.trace_count == 1);
  BOOST_CHECK(!res.has_errors());

  delete driver;
}

BOOST_AUTO_TEST_CASE(Reordering_box_19){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(

define i8* @p(i8* %arg){
  %i = ptrtoint i8* %arg to i32
  %ic = icmp ne i32 %i, 42
  br i1 %ic, label %exit, label %error
error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* inttoptr (i32 42 to i8*))
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();

  BOOST_CHECK(res.trace_count == 1);
  BOOST_CHECK(res.has_errors());

  delete driver;
}

BOOST_AUTO_TEST_CASE(Store_load_1){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 1111, align 4

define i8* @l(i8* %arg){
  %x = load i32, i32* @x, align 4
  %xc1 = icmp eq i32 %x, 1111
  br i1 %xc1, label %exit, label %cmp2
cmp2:
  %xc2 = icmp eq i32 %x, 2222
  br i1 %xc2, label %exit, label %error
error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @l, i8* null)
  store i32 2222, i32* @x, align 4
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

BOOST_AUTO_TEST_CASE(Store_load_2){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 1111, align 4

define i8* @l(i8* %arg){
  %x = load i32, i32* @x, align 4
  %xc1 = icmp eq i32 %x, 1111
  br i1 %xc1, label %error, label %exit
error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @l, i8* null)
  store i32 2222, i32* @x, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);
  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(Store_load_3){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 1111, align 4

define i8* @l(i8* %arg){
  %x = load i32, i32* @x, align 4
  %xc = icmp eq i32 %x, 2222
  br i1 %xc, label %error, label %exit
error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @l, i8* null)
  store i32 2222, i32* @x, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);
  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(Load_store_1){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 1111, align 4

define i8* @l(i8* %arg){
  store i32 2222, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @l, i8* null)
  %x = load i32, i32* @x, align 4
  %xc1 = icmp eq i32 %x, 1111
  br i1 %xc1, label %exit, label %cmp2
cmp2:
  %xc2 = icmp eq i32 %x, 2222
  br i1 %xc2, label %exit, label %error
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

  BOOST_CHECK(res.trace_count == 2);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Load_store_2){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 1111, align 4

define i8* @l(i8* %arg){
  store i32 2222, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @l, i8* null)
  %x = load i32, i32* @x, align 4
  %xc1 = icmp eq i32 %x, 1111
  br i1 %xc1, label %error, label %exit
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

  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(Load_store_3){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 1111, align 4

define i8* @l(i8* %arg){
  store i32 2222, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @l, i8* null)
  %x = load i32, i32* @x, align 4
  %xc = icmp eq i32 %x, 2222
  br i1 %xc, label %error, label %exit
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

  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(Store_store_1){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 1111, align 4

define i8* @p(i8* %arg){
  store i32 2222, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  store i32 3333, i32* @x, align 4
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

BOOST_AUTO_TEST_CASE(Store_store_2){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 1111, align 4

define i8* @p(i8* %arg){
  store i32 2222, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  store i32 3333, i32* @x, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);
  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 6);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Store_store_3){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 1111, align 4

define i8* @p(i8* %arg){
  store i32 2222, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  store i32 3333, i32* @x, align 4
  store i32 4444, i32* @x, align 4
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

BOOST_AUTO_TEST_CASE(MP_addr_1){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  %y = load i32, i32* @y, align 4
  %z = xor i32 %y, %y
  %xaddr = getelementptr i32, i32* @x, i32 %z
  load i32, i32* %xaddr
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  store i32 1, i32* @x, align 4
  store i32 1, i32* @y, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);
  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 4);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(LB_1){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  load i32, i32* @x, align 4
  store i32 1, i32* @y, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  load i32, i32* @y, align 4
  store i32 1, i32* @x, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);
  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 4);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(LB_data_1){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  %x = load i32, i32* @x, align 4
  %v = add i32 %x, 1
  store i32 %v, i32* @y, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  load i32, i32* @y, align 4
  store i32 1, i32* @x, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);
  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 4);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(LB_data_2){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  load i32, i32* @y, align 4
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  %x = load i32, i32* @x, align 4
  %v = add i32 %x, 1
  store i32 %v, i32* @y, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);
  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 4);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(LB_datas){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  %y = load i32, i32* @y, align 4
  %v = add i32 %y, 1
  store i32 %v, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  %x = load i32, i32* @x, align 4
  %v = add i32 %x, 1
  store i32 %v, i32* @y, align 4
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

BOOST_AUTO_TEST_CASE(LB_addrpos){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4
@z = global i32 0, align 4

define i8* @p(i8* %arg){
  %y = load i32, i32* @y, align 4
  %zero = xor i32 %y, %y
  %zaddr = getelementptr i32,  i32* @z, i32 %zero
  load i32, i32* %zaddr
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  %x = load i32, i32* @x, align 4
  %zero = xor i32 %x, %x
  %zaddr = getelementptr i32, i32* @z, i32 %zero
  load i32, i32* %zaddr
  store i32 1, i32* @y, align 4
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

BOOST_AUTO_TEST_CASE(RRW_1){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p(i8* %arg){
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  load i32, i32* @x, align 4
  load i32, i32* @x, align 4
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

BOOST_AUTO_TEST_CASE(CoWW_1){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p(i8* %arg){
  store i32 1, i32* @x, align 4
  store i32 2, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  %x1 = load i32, i32* @x, align 4
  %x2 = load i32, i32* @x, align 4
  %xc = icmp slt i32 %x2, %x1
  br i1 %xc, label %error, label %exit
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

  BOOST_CHECK(res.trace_count == 6);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(CoWW_2){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p(i8* %arg){
  %x1 = load i32, i32* @x, align 4
  %x2 = load i32, i32* @x, align 4
  %xc = icmp slt i32 %x2, %x1
  br i1 %xc, label %error, label %exit
error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  store i32 1, i32* @x, align 4
  store i32 2, i32* @x, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 6);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(CoWR_1){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 2, i32* @x, align 4
  ret i8* null
}

define i8* @p2(i8* %arg){
  %x1 = load i32, i32* @x, align 4
  %x2 = load i32, i32* @x, align 4
  %y = load i32, i32* @y, align 4
  %xc1 = icmp eq i32 %x1, 2
  %xc2 = icmp eq i32 %x2, 1
  %yc  = icmp eq i32 %y, 1
  %c1 = and i1 %xc1, %xc2
  %c2 = and i1 %c1, %yc
  br i1 %c2, label %error, label %exit
error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p2, i8* null)
  store i32 1, i32* @x, align 4
  %x = load i32, i32* @x, align 4
  %xc = icmp eq i32 %x, 2
  %v = zext i1 %xc to i32
  store i32 %v, i32* @y
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 36);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(CoWR_2){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 1, i32* @x, align 4
  %x = load i32, i32* @x, align 4
  %xc = icmp eq i32 %x, 2
  %v = zext i1 %xc to i32
  store i32 %v, i32* @y
  ret i8* null
}

define i8* @p2(i8* %arg){
  %x1 = load i32, i32* @x, align 4
  %x2 = load i32, i32* @x, align 4
  %y = load i32, i32* @y, align 4
  %xc1 = icmp eq i32 %x1, 2
  %xc2 = icmp eq i32 %x2, 1
  %yc  = icmp eq i32 %y, 1
  %c1 = and i1 %xc1, %xc2
  %c2 = and i1 %c1, %yc
  br i1 %c2, label %error, label %exit
error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p2, i8* null)
  store i32 2, i32* @x, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 36);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(CoRW_1){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 2, i32* @x, align 4
  ret i8* null
}

define i8* @p2(i8* %arg){
  %x1 = load i32, i32* @x, align 4
  %x2 = load i32, i32* @x, align 4
  %y = load i32, i32* @y, align 4
  %xc1 = icmp eq i32 %x1, 1
  %xc2 = icmp eq i32 %x2, 2
  %yc  = icmp eq i32 %y, 1
  %c1 = and i1 %xc1, %xc2
  %c2 = and i1 %c1, %yc
  br i1 %c2, label %error, label %exit
error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p2, i8* null)
  %x = load i32, i32* @x, align 4
  store i32 1, i32* @x
  %xc = icmp eq i32 %x, 2
  %v = zext i1 %xc to i32
  store i32 %v, i32* @y
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 36);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(CoRW_2){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  %x = load i32, i32* @x, align 4
  store i32 1, i32* @x
  %xc = icmp eq i32 %x, 2
  %v = zext i1 %xc to i32
  store i32 %v, i32* @y
  ret i8* null
}

define i8* @p2(i8* %arg){
  %x1 = load i32, i32* @x, align 4
  %x2 = load i32, i32* @x, align 4
  %y = load i32, i32* @y, align 4
  %xc1 = icmp eq i32 %x1, 1
  %xc2 = icmp eq i32 %x2, 2
  %yc  = icmp eq i32 %y, 1
  %c1 = and i1 %xc1, %xc2
  %c2 = and i1 %c1, %yc
  br i1 %c2, label %error, label %exit
error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p2, i8* null)
  store i32 2, i32* @x, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 36);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Ctrldep_1){ // LB+ctrls
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  %x = load i32, i32* @x, align 4
  %one = icmp eq i32 %x, %x
  br i1 %one, label %st, label %exit
st:
  store i32 1, i32* @y, align 4
  br label %exit
exit:
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  %y = load i32, i32* @y, align 4
  %one = icmp eq i32 %y, %y
  br i1 %one, label %st, label %exit
st:
  store i32 1, i32* @x, align 4
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

  BOOST_CHECK(res.trace_count == 3);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Ctrldep_2){ // LB+ctrl+(ctrl without dep)
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  %x = load i32, i32* @x, align 4
  %one = icmp eq i32 1, 1
  br i1 %one, label %st, label %exit
st:
  store i32 1, i32* @y, align 4
  br label %exit
exit:
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  %y = load i32, i32* @y, align 4
  %one = icmp eq i32 %y, %y
  br i1 %one, label %st, label %exit
st:
  store i32 1, i32* @x, align 4
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

  BOOST_CHECK(res.trace_count == 4);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(LB_syncs){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  load i32, i32* @x, align 4
  call void asm sideeffect "sync", ""()
  store i32 1, i32* @y, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  load i32, i32* @y, align 4
  call void asm sideeffect "sync", ""()
  store i32 1, i32* @x, align 4
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

BOOST_AUTO_TEST_CASE(LB_lwsyncs){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  load i32, i32* @x, align 4
  call void asm sideeffect "lwsync", ""()
  store i32 1, i32* @y, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  load i32, i32* @y, align 4
  call void asm sideeffect "lwsync", ""()
  store i32 1, i32* @x, align 4
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

BOOST_AUTO_TEST_CASE(LB_isyncs){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  load i32, i32* @x, align 4
  call void asm sideeffect "isync", ""()
  store i32 1, i32* @y, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  load i32, i32* @y, align 4
  call void asm sideeffect "isync", ""()
  store i32 1, i32* @x, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 4);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(MP_lwsync_addr){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  %y = load i32, i32* @y, align 4
  %zero = xor i32 %y, %y
  %xaddr = getelementptr i32, i32* @x, i32 %zero
  load i32, i32* %xaddr, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  store i32 1, i32* @x, align 4
  call void asm sideeffect "lwsync", ""()
  store i32 1, i32* @y, align 4
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

BOOST_AUTO_TEST_CASE(MP_addr){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  %y = load i32, i32* @y, align 4
  %zero = xor i32 %y, %y
  %xaddr = getelementptr i32, i32* @x, i32 %zero
  load i32, i32* %xaddr, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  store i32 1, i32* @x, align 4
  store i32 1, i32* @y, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 4);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(MP_lwsyncs){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  load i32, i32* @y, align 4
  call void asm sideeffect "lwsync", ""()
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  store i32 1, i32* @x, align 4
  call void asm sideeffect "lwsync", ""()
  store i32 1, i32* @y, align 4
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

BOOST_AUTO_TEST_CASE(MP_lwsync_datarfiaddr){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4
@z = global i32 0, align 4

define i8* @p(i8* %arg){
  %y = load i32, i32* @y, align 4
  store i32 %y, i32* @z, align 4
  %z = load i32, i32* @z, align 4
  %zero = xor i32 %z, %z
  %xaddr = getelementptr i32, i32* @x, i32 %zero
  load i32, i32* %xaddr, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  store i32 1, i32* @x, align 4
  call void asm sideeffect "lwsync", ""()
  store i32 1, i32* @y, align 4
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

BOOST_AUTO_TEST_CASE(MP_lwsync_addrpo){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4
@z = global i32 0, align 4

define i8* @p(i8* %arg){
  %y = load i32, i32* @y, align 4
  %zero = xor i32 %y, %y
  %zaddr = getelementptr i32, i32* @z, i32 %zero
  %z = load i32, i32* @z, align 4
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  store i32 1, i32* @x, align 4
  call void asm sideeffect "lwsync", ""()
  store i32 1, i32* @y, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 4); // addr;po does not order RR
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(MP_lwsync_ctrl){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4
@z = global i32 0, align 4

define i8* @p(i8* %arg){
  %y = load i32, i32* @y, align 4
  %cnd = icmp eq i32 %y, 1
  br i1 %cnd, label %LBL, label %LBL
LBL:
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  store i32 1, i32* @x, align 4
  call void asm sideeffect "lwsync", ""()
  store i32 1, i32* @y, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 4); // ctrl does not order RR
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(MP_lwsync_isync){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4
@z = global i32 0, align 4

define i8* @p(i8* %arg){
  %y = load i32, i32* @y, align 4
  call void asm sideeffect "isync", ""()
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  store i32 1, i32* @x, align 4
  call void asm sideeffect "lwsync", ""()
  store i32 1, i32* @y, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 4); // isync without ctrl does not order RR
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(MP_lwsync_ctrlisync){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4
@z = global i32 0, align 4

define i8* @p(i8* %arg){
  %y = load i32, i32* @y, align 4
  %cnd = icmp eq i32 %y, 1
  br i1 %cnd, label %LBL, label %LBL
LBL:
  call void asm sideeffect "isync", ""()
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  store i32 1, i32* @x, align 4
  call void asm sideeffect "lwsync", ""()
  store i32 1, i32* @y, align 4
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

BOOST_AUTO_TEST_CASE(MP_lwsync_isyncctrl){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4
@z = global i32 0, align 4

define i8* @p(i8* %arg){
  %y = load i32, i32* @y, align 4
  %cnd = icmp eq i32 %y, 1
  call void asm sideeffect "isync", ""()
  br i1 %cnd, label %LBL, label %LBL
LBL:
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  store i32 1, i32* @x, align 4
  call void asm sideeffect "lwsync", ""()
  store i32 1, i32* @y, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 4); // Wrong order between branch and isync
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(MP3_lwsync_addrpo_addr){
  /* Checks that addr;po is counted as part of cc in hb in
   * OBSERVATION.
   */
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@w = global i32 0, align 4
@x = global i32 0, align 4
@y = global i32 0, align 4
@z = global i32 0, align 4

define i8* @p(i8* %arg){
  %y = load i32, i32* @y, align 4
  %zero = xor i32 %y, %y
  %zaddr = getelementptr i32, i32* @z, i32 %zero
  %z = load i32, i32* %zaddr, align 4
  store i32 1, i32* @w, align 4
  ret i8* null
}

define i8* @q(i8* %arg){
  %w = load i32, i32* @w, align 4
  %zero = xor i32 %w, %w
  %xaddr = getelementptr i32, i32* @x, i32 %zero
  load i32, i32* %xaddr, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @q, i8* null)
  store i32 1, i32* @x, align 4
  call void asm sideeffect "lwsync", ""()
  store i32 1, i32* @y, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 7);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(MP3_lwsync_ctrl_addr){
  /* Checks that ctrl is counted as part of cc in hb in
   * OBSERVATION.
   */
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@w = global i32 0, align 4
@x = global i32 0, align 4
@y = global i32 0, align 4
@z = global i32 0, align 4

define i8* @p(i8* %arg){
  %y = load i32, i32* @y, align 4
  %cnd = icmp eq i32 %y, 1
  br i1 %cnd, label %LBL, label %LBL
LBL:
  store i32 1, i32* @w, align 4
  ret i8* null
}

define i8* @q(i8* %arg){
  %w = load i32, i32* @w, align 4
  %zero = xor i32 %w, %w
  %xaddr = getelementptr i32, i32* @x, i32 %zero
  load i32, i32* %xaddr, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @q, i8* null)
  store i32 1, i32* @x, align 4
  call void asm sideeffect "lwsync", ""()
  store i32 1, i32* @y, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 7);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(TWO_2W){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  store i32 1, i32* @y, align 4
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  store i32 1, i32* @x, align 4
  store i32 1, i32* @y, align 4
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

BOOST_AUTO_TEST_CASE(TWO_2W_lwsync){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  store i32 1, i32* @y, align 4
  call void asm sideeffect "lwsync", ""()
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  store i32 1, i32* @x, align 4
  store i32 1, i32* @y, align 4
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

BOOST_AUTO_TEST_CASE(TWO_2W_lwsync_2){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  store i32 1, i32* @y, align 4
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  store i32 1, i32* @x, align 4
  call void asm sideeffect "lwsync", ""()
  store i32 1, i32* @y, align 4
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

BOOST_AUTO_TEST_CASE(TWO_2W_lwsyncs){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  store i32 1, i32* @y, align 4
  call void asm sideeffect "lwsync", ""()
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  store i32 1, i32* @x, align 4
  call void asm sideeffect "lwsync", ""()
  store i32 1, i32* @y, align 4
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

BOOST_AUTO_TEST_CASE(W_RW_2W){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  load i32, i32* @x, align 4
  store i32 1, i32* @y, align 4
  ret i8* null
}

define i8* @q(i8* %arg){
  store i32 2, i32* @y, align 4
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @q, i8* null)
  store i32 2, i32* @x, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 12);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(W_RW_2W_lwsyncs){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  load i32, i32* @x, align 4
  call void asm sideeffect "lwsync", ""()
  store i32 1, i32* @y, align 4
  ret i8* null
}

define i8* @q(i8* %arg){
  store i32 2, i32* @y, align 4
  call void asm sideeffect "lwsync", ""()
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @q, i8* null)
  store i32 2, i32* @x, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 9);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(SB){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  store i32 1, i32* @y, align 4
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  store i32 1, i32* @x, align 4
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

BOOST_AUTO_TEST_CASE(SB_lwsyncs){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  store i32 1, i32* @y, align 4
  call void asm sideeffect "lwsync", ""()
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  store i32 1, i32* @x, align 4
  call void asm sideeffect "lwsync", ""()
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

BOOST_AUTO_TEST_CASE(SB_syncs){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  store i32 1, i32* @y, align 4
  call void asm sideeffect "sync", ""()
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  store i32 1, i32* @x, align 4
  call void asm sideeffect "sync", ""()
  load i32, i32* @y, align 4
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

BOOST_AUTO_TEST_CASE(IRIW){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  store i32 1, i32* @y, align 4
  ret i8* null
}

define i8* @q(i8* %arg){
  load i32, i32* @x, align 4
  load i32, i32* @y, align 4
  ret i8* null
}

define i8* @r(i8* %arg){
  load i32, i32* @y, align 4
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @q, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @r, i8* null)
  store i32 1, i32* @x, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 16);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(IRIW_addrs){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  store i32 1, i32* @y, align 4
  ret i8* null
}

define i8* @q(i8* %arg){
  %x = load i32, i32* @x, align 4
  %zero = xor i32 %x, %x
  %yaddr = getelementptr i32, i32* @y, i32 %zero
  load i32, i32* %yaddr, align 4
  ret i8* null
}

define i8* @r(i8* %arg){
  %y = load i32, i32* @y, align 4
  %zero = xor i32 %y, %y
  %xaddr = getelementptr i32, i32* @x, i32 %zero
  load i32, i32* %xaddr, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @q, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @r, i8* null)
  store i32 1, i32* @x, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 16);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(IRIW_lwsyncs){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  store i32 1, i32* @y, align 4
  ret i8* null
}

define i8* @q(i8* %arg){
  load i32, i32* @x, align 4
  call void asm sideeffect "lwsync", ""()
  load i32, i32* @y, align 4
  ret i8* null
}

define i8* @r(i8* %arg){
  load i32, i32* @y, align 4
  call void asm sideeffect "lwsync", ""()
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @q, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @r, i8* null)
  store i32 1, i32* @x, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 16);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(IRIW_syncs){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  store i32 1, i32* @y, align 4
  ret i8* null
}

define i8* @q(i8* %arg){
  load i32, i32* @x, align 4
  call void asm sideeffect "sync", ""()
  load i32, i32* @y, align 4
  ret i8* null
}

define i8* @r(i8* %arg){
  load i32, i32* @y, align 4
  call void asm sideeffect "sync", ""()
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @q, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @r, i8* null)
  store i32 1, i32* @x, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 15);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(THREE_2W){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4
@z = global i32 0, align 4

define i8* @p(i8* %arg){
  store i32 2, i32* @y, align 4
  store i32 1, i32* @z, align 4
  ret i8* null
}

define i8* @q(i8* %arg){
  store i32 2, i32* @z, align 4
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @q, i8* null)
  store i32 2, i32* @x, align 4
  store i32 1, i32* @y, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 8);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(THREE_2W_lwsyncs){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4
@z = global i32 0, align 4

define i8* @p(i8* %arg){
  store i32 2, i32* @y, align 4
  call void asm sideeffect "lwsync", ""()
  store i32 1, i32* @z, align 4
  ret i8* null
}

define i8* @q(i8* %arg){
  store i32 2, i32* @z, align 4
  call void asm sideeffect "lwsync", ""()
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @q, i8* null)
  store i32 2, i32* @x, align 4
  call void asm sideeffect "lwsync", ""()
  store i32 1, i32* @y, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 7);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Commit_fences_1){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i1 1, align 4

define i32 @main(){
  %x = load i1, i1* @x
  br i1 %x, label %lbl1, label %lbl1
lbl1:
  call void asm sideeffect "lwsync", ""()
  %x2 = load i1, i1* @x
  br i1 %x2, label %lbl2, label %lbl2
lbl2:
  call void @__assert_fail()
  ret i32 0
}

declare void @__assert_fail()
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 1);
  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(Commit_fences_2){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i1 0
@y = global i1 0
@z = global i1 0

define i8* @p(i8* %arg){
  store i1 1, i1* @y
  call void asm sideeffect "lwsync", ""()
  store i1 1, i1* @x
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  %x = load i1, i1* @x
  load i1, i1* @z
  call void asm sideeffect "lwsync", ""()
  %y = load i1, i1* @y
  %y0 = icmp eq i1 %y, 0
  %cnd = and i1 %x, %y0
  br i1 %cnd, label %error, label %exit
error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
declare void @__assert_fail()
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 3);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Disappearing_sleep_1){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0
@y = global i32 0

define i8* @p(i8* %arg){
  %x = load i32, i32* @x
  %xc = icmp eq i32 %x, 1
  br i1 %xc, label %cont, label %exit
cont:
  load i32, i32* @y
  br label %exit
exit:
  ret i8* null
}

define i8* @q(i8* %arg){
  store i32 1, i32* @y
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @q, i8* null)
  store i32 1, i32* @x
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

BOOST_AUTO_TEST_CASE(TWO_2W_rfi_datas_noise){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4
@p_wait = global i1 0
@q_wait = global i1 0

define i8* @p(i8* %arg){
  store i32 1, i32* @y, align 4
  %y = load i32, i32* @y
  store i32 %y, i32* @x, align 4
  store i1 1, i1* @p_wait
  ret i8* null
}

define i8* @q(i8* %arg){
  store i32 1, i32* @x, align 4
  %x = load i32, i32* @x
  store i32 %x, i32* @y, align 4
  store i1 1, i1* @q_wait
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @q, i8* null)
  load i1, i1* @p_wait
  load i1, i1* @q_wait
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 32);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(MPW_lwsyncs){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  store i32 1, i32* @x
  call void asm sideeffect "lwsync", ""()
  store i32 1, i32* @y
  ret i8* null
}

define i8* @q(i8* %arg){
  store i32 1, i32* @x
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @q, i8* null)
  load i32, i32* @y
  call void asm sideeffect "lwsync", ""()
  load i32, i32* @x
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 9);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(MPWW_lwsyncs){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  store i32 1, i32* @x
  call void asm sideeffect "lwsync", ""()
  store i32 1, i32* @y
  ret i8* null
}

define i8* @q(i8* %arg){
  store i32 1, i32* @x
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @q, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @q, i8* null)
  load i32, i32* @y
  call void asm sideeffect "lwsync", ""()
  load i32, i32* @x
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 36);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(PHI_delayed){
  Configuration conf = DPORDriver_test::get_power_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p(i8* %arg){
  store i32 1, i32* @x
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  br i1 0, label %A, label %B
A:
  br label %next
B:
  %x = load i32, i32* @x
  br label %next
next:
  %x2 = phi i32 [0, %A], [%x, %B]
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 2);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(LB_assume_1){
  Configuration conf = DPORDriver_test::get_power_conf();
  conf.malloc_may_fail = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0
@y = global i32 0

define i8* @p(i8* %arg){
  %x = load i32, i32* @x
  call void @__VERIFIER_assume(i32 %x)
  store i32 1, i32* @y
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  load i32 , i32 *@y
  store i32 1, i32* @x
  ret i32 0
}

declare void @__VERIFIER_assume(i32)
%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
)"),conf);
  DPORDriver::Result res = driver->run();
  delete driver;
  BOOST_CHECK(!res.has_errors());
  BOOST_CHECK(res.trace_count == 3);
}

BOOST_AUTO_TEST_CASE(LB_assume_2){
  Configuration conf = DPORDriver_test::get_power_conf();
  conf.malloc_may_fail = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0
@y = global i32 0

define i8* @p(i8* %arg){
  %x = load i32, i32* @x
  %t = add i32 1, %x
  call void @__VERIFIER_assume(i32 %t)
  store i32 1, i32* @y
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  load i32 , i32 *@y
  store i32 1, i32* @x
  ret i32 0
}

declare void @__VERIFIER_assume(i32)
%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
)"),conf);
  DPORDriver::Result res = driver->run();
  delete driver;
  BOOST_CHECK(!res.has_errors());
  BOOST_CHECK(res.trace_count == 4);
}

BOOST_AUTO_TEST_CASE(LB_assume_3){
  Configuration conf = DPORDriver_test::get_power_conf();
  conf.malloc_may_fail = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0
@y = global i32 0

define i8* @p(i8* %arg){
  %x = load i32, i32* @x
  call void @__VERIFIER_assume(i32 %x)
  store i32 1, i32* @y
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  %y = load i32 , i32 *@y
  call void @__VERIFIER_assume(i32 %y)
  store i32 1, i32* @x
  ret i32 0
}

declare void @__VERIFIER_assume(i32)
%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*)
)"),conf);
  DPORDriver::Result res = driver->run();
  delete driver;
  BOOST_CHECK(!res.has_errors());
  BOOST_CHECK(res.trace_count == 1);
}

BOOST_AUTO_TEST_SUITE_END()

#endif

