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

#include "vecset.h"

#include "DPORDriver.h"
#include "StrModule.h"

#include <llvm/Support/Debug.h>

#include <boost/test/unit_test.hpp>

#include <sstream>

BOOST_AUTO_TEST_SUITE(DryRun_test)

static VecSet<Configuration::MemoryModel> MMs =
{Configuration::SC, Configuration::TSO, Configuration::PSO};

static void check(Configuration::MemoryModel MM, const std::string &prog){
  Configuration conf;
  conf.debug_collect_all_traces = true;
  conf.memory_model = MM;
  DPORDriver *driver =
    DPORDriver::parseIR(prog,conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
}

static void check_all(const std::string &prog_template){
  for(auto MM : MMs){
    check(MM,prog_template);
  }
}

static std::string &format(std::string &s, const std::vector<int> &substs){
  for(unsigned i = 0; i < substs.size(); ++i){
    std::stringstream ss_fmt, ss_subst;
    ss_fmt << "{" << i << "}";
    ss_subst << substs[i];
    std::size_t pos = 0;
    while((pos = s.find(ss_fmt.str(),pos)) != std::string::npos){
      s.replace(pos,ss_fmt.str().size(),ss_subst.str());
    }
  }
  return s;
}

BOOST_AUTO_TEST_CASE(DryRunMem_1){
  check_all(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  call void @__VERIFIER_atomic_f();
  ret i8* null
}

define void @__VERIFIER_atomic_f(){
  store i32 1, i32* @x, align 4
  store i32 1, i32* @y, align 4
  %y = load i32, i32* @y, align 4
  %cmp0 = icmp ne i32 %y, 1
  br i1 %cmp0, label %error, label %exit
error:
  load i32, i32* null, align 4 ; Cause segmentation fault
  br label %exit
exit:
  ret void
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call void @__VERIFIER_atomic_f()
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)"));
}

BOOST_AUTO_TEST_CASE(DryRunMem_2){
  check_all(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  call void @__VERIFIER_atomic_f();
  ret i8* null
}

define void @__VERIFIER_atomic_f(){
  store i32 1, i32* @x, align 4
  store i32 1, i32* @y, align 4
  store i32 2, i32* @y, align 4
  store i32 3, i32* @y, align 4
  %y = load i32, i32* @y, align 4
  %cmp0 = icmp ne i32 %y, 3
  br i1 %cmp0, label %error, label %exit
error:
  load i32, i32* null, align 4 ; Cause segmentation fault
  br label %exit
exit:
  ret void
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call void @__VERIFIER_atomic_f()
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)"));
}

BOOST_AUTO_TEST_CASE(DryRunMem_3){
  std::string prog = R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  call void @__VERIFIER_atomic_f();
  ret i8* null
}

define void @__VERIFIER_atomic_f(){
  store i32 1, i32* @x, align 4
  %yc0 = bitcast i32* @y to i8*
  store i8 1, i8* %yc0
  %yc1 = getelementptr i8, i8* %yc0, i32 1
  store i8 2, i8* %yc1
  %yc2 = getelementptr i8, i8* %yc0, i32 2
  store i8 3, i8* %yc2
  %yc3 = getelementptr i8, i8* %yc0, i32 3
  store i8 4, i8* %yc3
  %y = load i32, i32* @y, align 4
  %cmp0 = icmp ne i32 %y, {0}
  br i1 %cmp0, label %error, label %exit
error:
  load i32, i32* null, align 4 ; Cause segmentation fault
  br label %exit
exit:
  ret void
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call void @__VERIFIER_atomic_f()
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)";

  /* Figure out what the result value should be on this machine. */
  int32_t i = 0;
  ((char*)&i)[0] = 1;
  ((char*)&i)[1] = 2;
  ((char*)&i)[2] = 3;
  ((char*)&i)[3] = 4;

  format(prog,{i});

  check_all(StrModule::portasm(prog));
}

BOOST_AUTO_TEST_CASE(DryRunMem_4){
  std::string prog = R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p(i8* %arg){
  call void @__VERIFIER_atomic_f();
  ret i8* null
}

define void @__VERIFIER_atomic_f(){
  store i32 1, i32* @x, align 4
  store i32 {0}, i32* @y
  %yc0 = bitcast i32* @y to i8*
  %yc1 = getelementptr i8, i8* %yc0, i32 1
  %yc2 = getelementptr i8, i8* %yc0, i32 2
  %yc3 = getelementptr i8, i8* %yc0, i32 3
  %y0 = load i8, i8* %yc0
  %y1 = load i8, i8* %yc1
  %y2 = load i8, i8* %yc2
  %y3 = load i8, i8* %yc3
  %cmp0 = icmp ne i8 %y0, 1
  %cmp1 = icmp ne i8 %y1, 2
  %cmp2 = icmp ne i8 %y2, 3
  %cmp3 = icmp ne i8 %y3, 4
  %cmp01 = or i1 %cmp0, %cmp1
  %cmp23 = or i1 %cmp2, %cmp3
  %cmp = or i1 %cmp01, %cmp23
  br i1 %cmp, label %error, label %exit
error:
  load i32, i32* null, align 4 ; Cause segmentation fault
  br label %exit
exit:
  ret void
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call void @__VERIFIER_atomic_f()
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)";

  /* Figure out what the result value should be on this machine. */
  int32_t i = 0;
  ((char*)&i)[0] = 1;
  ((char*)&i)[1] = 2;
  ((char*)&i)[2] = 3;
  ((char*)&i)[3] = 4;

  format(prog,{i});

  check_all(StrModule::portasm(prog));
}

BOOST_AUTO_TEST_CASE(DryRunMem_5){
  std::string prog = R"(
@x = global i32 0, align 4
@y = global i32 {0}, align 4

define i8* @p(i8* %arg){
  call void @__VERIFIER_atomic_f();
  ret i8* null
}

define void @__VERIFIER_atomic_f(){
  store i32 1, i32* @x, align 4
  %yc0 = bitcast i32* @y to i8*
  %yc1 = getelementptr i8, i8* %yc0, i32 1
  %yc2 = getelementptr i8, i8* %yc0, i32 2
  %yc3 = getelementptr i8, i8* %yc0, i32 3
  store i8 42, i8* %yc1
  %y0 = load i8, i8* %yc0
  %y1 = load i8, i8* %yc1
  %y2 = load i8, i8* %yc2
  %y3 = load i8, i8* %yc3
  %cmp0 = icmp ne i8 %y0, 1
  %cmp1 = icmp ne i8 %y1, 42
  %cmp2 = icmp ne i8 %y2, 3
  %cmp3 = icmp ne i8 %y3, 4
  %cmp01 = or i1 %cmp0, %cmp1
  %cmp23 = or i1 %cmp2, %cmp3
  %cmp = or i1 %cmp01, %cmp23
  br i1 %cmp, label %error, label %exit
error:
  load i32, i32* null, align 4 ; Cause segmentation fault
  br label %exit
exit:
  ret void
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call void @__VERIFIER_atomic_f()
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)";

  /* Figure out what the result value should be on this machine. */
  int32_t i = 0;
  ((char*)&i)[0] = 1;
  ((char*)&i)[1] = 2;
  ((char*)&i)[2] = 3;
  ((char*)&i)[3] = 4;

  format(prog,{i});

  check_all(StrModule::portasm(prog));
}

BOOST_AUTO_TEST_CASE(DryRunMem_6){
  std::string prog = R"(
@x = global i32 0, align 4
@y = global i32 {0}, align 4

define i8* @p(i8* %arg){
  call void @__VERIFIER_atomic_f();
  ret i8* null
}

define void @__VERIFIER_atomic_f(){
  store i32 1, i32* @x, align 4
  %yc0 = bitcast i32* @y to i8*
  %yc1 = getelementptr i8, i8* %yc0, i32 1
  store i8 42, i8* %yc1
  %y = load i32, i32* @y, align 4
  %cmp = icmp ne i32 %y, {1}
  br i1 %cmp, label %error, label %exit
error:
  load i32, i32* null, align 4 ; Cause segmentation fault
  br label %exit
exit:
  ret void
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  call void @__VERIFIER_atomic_f()
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)";

  /* Figure out what the result value should be on this machine. */
  int32_t i = 0;
  ((char*)&i)[0] = 1;
  ((char*)&i)[1] = 2;
  ((char*)&i)[2] = 3;
  ((char*)&i)[3] = 4;
  int32_t j = i;
  ((char*)&j)[1] = 42;

  format(prog,{i,j});

  check_all(StrModule::portasm(prog));
}

BOOST_AUTO_TEST_SUITE_END()

#endif
