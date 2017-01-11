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

#include "DPORDriver_test.h"
#include "StrModule.h"
#include "Transform.h"

#if defined(HAVE_LLVM_ANALYSIS_VERIFIER_H)
#include <llvm/Analysis/Verifier.h>
#elif defined(HAVE_LLVM_IR_VERIFIER_H)
#include <llvm/IR/Verifier.h>
#endif
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(Unroll_test)

BOOST_AUTO_TEST_CASE(Global_spin_1){
  Configuration conf = DPORDriver_test::get_sc_conf();
  Configuration tconf;
  tconf.transform_loop_unroll = 4;
  DPORDriver *tdriver =
    DPORDriver::parseIR(Transform::transform(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 1, i32* @y, align 4
  load i32, i32* @x, align 4
  store i32 0, i32* @y, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  br label %head

head:
  %y = load i32, i32* @y, align 4
  %yc = icmp ne i32 %y, 0
  br i1 %yc, label %body, label %exit

body:
  store i32 1, i32* @x, align 4
  br label %head

exit:
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)"),tconf),conf);

  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 1, i32* @y, align 4
  load i32, i32* @x, align 4
  store i32 0, i32* @y, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  br label %head0

head0:
  %y0 = load i32, i32* @y, align 4
  %yc0 = icmp ne i32 %y0, 0
  br i1 %yc0, label %body0, label %exit

body0:
  store i32 1, i32* @x, align 4
  br label %head1

head1:
  %y1 = load i32, i32* @y, align 4
  %yc1 = icmp ne i32 %y1, 0
  br i1 %yc1, label %body1, label %exit

body1:
  store i32 1, i32* @x, align 4
  br label %head2

head2:
  %y2 = load i32, i32* @y, align 4
  %yc2 = icmp ne i32 %y2, 0
  br i1 %yc2, label %body2, label %exit

body2:
  store i32 1, i32* @x, align 4
  br label %head3

head3:
  %y3 = load i32, i32* @y, align 4
  %yc3 = icmp ne i32 %y3, 0
  br i1 %yc3, label %body3, label %exit

body3:
  store i32 1, i32* @x, align 4
  br label %diverge

diverge:
  call void @__VERIFIER_assume(i1 false)
  br label %diverge

exit:
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare void @__VERIFIER_assume(i1) nounwind
)"),conf);

  DPORDriver::Result tres = tdriver->run();
  delete tdriver;
  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
  BOOST_CHECK(!tres.has_errors());
  BOOST_CHECK(res.trace_count == tres.trace_count);
}

BOOST_AUTO_TEST_CASE(PHI_counter_1){
  Configuration conf = DPORDriver_test::get_sc_conf();
  Configuration tconf;
  tconf.transform_loop_unroll = 4;
  DPORDriver *tdriver =
    DPORDriver::parseIR(Transform::transform(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  br label %init

init:
  br label %head

head:
  %i = phi i32 [0,%init], [%i2,%tail]
  %ic = icmp slt i32 %i, 10
  br i1 %ic, label %body, label %exit

body:
  store i32 1, i32* @x, align 4
  br label %tail

tail:
  %i2 = add i32 %i, 1
  br label %head

exit:
  call void @__assert_fail() ; Cannot get here with loop bound 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare void @__assert_fail() nounwind
)"),tconf),conf);

  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)

  store i32 1, i32* @x, align 4
  store i32 1, i32* @x, align 4
  store i32 1, i32* @x, align 4
  store i32 1, i32* @x, align 4

  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)"),conf);

  DPORDriver::Result tres = tdriver->run();
  delete tdriver;
  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
  BOOST_CHECK(!tres.has_errors());
  BOOST_CHECK(res.trace_count == tres.trace_count);
}

BOOST_AUTO_TEST_CASE(Termination_1){
  Configuration conf = DPORDriver_test::get_sc_conf();
  Configuration tconf;
  tconf.transform_loop_unroll = 4;
  DPORDriver *tdriver =
    DPORDriver::parseIR(Transform::transform(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  br label %init

init:
  br label %head

head:
  %i = phi i32 [0,%init], [%i2,%tail]
  %ic = icmp slt i32 %i, 4
  br i1 %ic, label %body, label %exit

body:
  store i32 1, i32* @x, align 4
  br label %tail

tail:
  %i2 = add i32 %i, 1
  br label %head

exit:
  call void @__assert_fail() ; Cannot get here with loop bound 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare void @__assert_fail() nounwind
)"),tconf),conf);

  DPORDriver::Result tres = tdriver->run();
  delete tdriver;

  BOOST_CHECK(!tres.has_errors());
}

BOOST_AUTO_TEST_CASE(Termination_2){
  Configuration conf = DPORDriver_test::get_sc_conf();
  Configuration tconf;
  tconf.transform_loop_unroll = 4;
  DPORDriver *tdriver =
    DPORDriver::parseIR(Transform::transform(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  br label %init

init:
  br label %head

head:
  %i = phi i32 [0,%init], [%i2,%tail]
  %ic = icmp slt i32 %i, 3
  br i1 %ic, label %body, label %exit

body:
  store i32 1, i32* @x, align 4
  br label %tail

tail:
  %i2 = add i32 %i, 1
  br label %head

exit:
  call void @__assert_fail()
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare void @__assert_fail() nounwind
)"),tconf),conf);

  DPORDriver::Result tres = tdriver->run();
  delete tdriver;

  BOOST_CHECK(tres.has_errors());
}

BOOST_AUTO_TEST_CASE(Unroll_4){
  Configuration conf = DPORDriver_test::get_sc_conf();
  Configuration tconf;
  tconf.transform_loop_unroll = 4;
  DPORDriver *tdriver =
    DPORDriver::parseIR(Transform::transform(R"(
@x = global i32 0, align 4

define i8* @f(i8* %arg) {
entry:
  br label %do.body

do.body:
  br label %do.cond

do.cond:
  br i1 1, label %land.rhs, label %land.end

land.rhs:
  store i32 1, i32* @x, align 4
  br label %land.end

land.end:
  %E = phi i1 [ false, %do.cond ], [ true, %land.rhs ]
  br i1 %E, label %do.body, label %do.end

do.end:
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @f, i8* null)
  store i32 1, i32* @x, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)",tconf),conf);

  DPORDriver *driver =
    DPORDriver::parseIR(R"(
@x = global i32 0, align 4

define i8* @f(i8* %arg){
  store i32 1, i32* @x, align 4
  store i32 1, i32* @x, align 4
  store i32 1, i32* @x, align 4
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @f, i8* null)
  store i32 1, i32* @x, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)",conf);

  DPORDriver::Result tres = tdriver->run();
  DPORDriver::Result res = driver->run();
  delete tdriver;
  delete driver;

  BOOST_CHECK(tres.has_errors() == res.has_errors());
  BOOST_CHECK(tres.trace_count == res.trace_count);
}

BOOST_AUTO_TEST_CASE(PHI_exit){
  Configuration tconf;
  tconf.transform_loop_unroll = 4;
  llvm::Module *mod =
    StrModule::read_module_src(R"(
@x = global i32 0

define i32 @main(){
entry:
  %rv0 = add i32 1, 1
  br i1 1, label %done, label %header
header:
  %rvloop = add i32 2, 2
  br i1 1, label %body, label %done
body:
  store i32 1, i32* @x
  br label %header
done:
  %rv = phi i32 [%rv0 ,%entry], [%rvloop, %header]
  ret i32 %rv
}
)");
  BOOST_CHECK(Transform::transform(*mod,tconf));
  BOOST_CHECK(!llvm::verifyModule(*mod));

  delete mod;
}

BOOST_AUTO_TEST_CASE(PHI_exit_2){
  Configuration tconf;
  tconf.transform_loop_unroll = 4;
  llvm::Module *mod =
    StrModule::read_module_src(R"(
@x = global i32 0

define i32 @main(){
entry:
  %a = add i32 1, 1
  %b = add i32 2, 2
  br i1 1, label %done, label %header

header:
  br i1 1, label %done, label %body

body:
  store i32 1, i32* @x
  br label %header

done:
  %c = phi i32 [%a, %entry], [%b, %header]
  ret i32 0
}
)");
  BOOST_CHECK(Transform::transform(*mod,tconf));
  BOOST_CHECK(!llvm::verifyModule(*mod));

  delete mod;
}

BOOST_AUTO_TEST_CASE(PHI_exit_3){
  Configuration tconf;
  tconf.transform_loop_unroll = 4;
  llvm::Module *mod =
    StrModule::read_module_src(R"(
@x = global i32 0

define i32 @main(){
entry:
  %a = add i32 1, 1
  %b = add i32 2, 1
  %c = add i32 3, 1
  br i1 1, label %header, label %done

header:
  br i1 1, label %body, label %done

body:
  store i32 1, i32* @x
  br i1 1, label %header, label %done

done:
  %rv = phi i32 [%a,%entry], [%b,%header], [%c,%body]
  ret i32 0
}
)");
  BOOST_CHECK(Transform::transform(*mod,tconf));
  BOOST_CHECK(!llvm::verifyModule(*mod));

  delete mod;
}

BOOST_AUTO_TEST_SUITE_END()

#endif
