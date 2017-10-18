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

BOOST_AUTO_TEST_SUITE(Observers_test)

static Configuration sc_obs_conf(){
  Configuration conf = DPORDriver_test::get_sc_conf();
  conf.dpor_algorithm = Configuration::OPTIMAL;
  conf.observers = true;
  return conf;
}

BOOST_AUTO_TEST_CASE(lastwrite_4){
  Configuration conf = sc_obs_conf();
  std::string module = R"(
@var = global i32 zeroinitializer, align 4

define i8* @writer(i8* %arg) {
  %j = ptrtoint i8* %arg to i32
  store i32 %j, i32* @var, align 4
  ret i8* null
}

define i32 @main() {
  %T0 = alloca i64
  %T1 = alloca i64
  %T2 = alloca i64
  %T3 = alloca i64
  call i32 @pthread_create(i64* %T0, %attr_t* null, i8* (i8*)* @writer, i8* inttoptr (i64 1 to i8*))
  call i32 @pthread_create(i64* %T1, %attr_t* null, i8* (i8*)* @writer, i8* inttoptr (i64 2 to i8*))
  call i32 @pthread_create(i64* %T2, %attr_t* null, i8* (i8*)* @writer, i8* inttoptr (i64 3 to i8*))
  call i32 @pthread_create(i64* %T3, %attr_t* null, i8* (i8*)* @writer, i8* inttoptr (i64 4 to i8*))
  %T0val = load i64, i64* %T0
  %T1val = load i64, i64* %T1
  %T2val = load i64, i64* %T2
  %T3val = load i64, i64* %T3
  call i32 @pthread_join(i64 %T0val,i8** null)
  call i32 @pthread_join(i64 %T1val,i8** null)
  call i32 @pthread_join(i64 %T2val,i8** null)
  call i32 @pthread_join(i64 %T3val,i8** null)
  %res = load i32, i32* @var
  ret i32 %res
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8* (i8*)*, i8*) nounwind
declare i32 @pthread_join(i64,i8**) nounwind
)";
  std::unique_ptr<DPORDriver> driver(DPORDriver::parseIR(module, conf));
  DPORDriver::Result res = driver->run();

  CPid P0;
  CPid P1 = P0.spawn(0);
  CPid P2 = P0.spawn(1);
  CPid P3 = P0.spawn(2);
  CPid P4 = P0.spawn(3);
  IID<CPid> W1(P1,2), W2(P2,2), W3(P3,2), W4(P4,2);
  DPORDriver_test::trace_set_spec expected =
    {{{W1,W4},{W2,W4},{W3,W4}},                         // 01: W4 last
     {{W1,W3},{W2,W3},{W4,W3}},                         // 02: W3 last
     {{W1,W2},{W3,W2},{W4,W2}},                         // 03: W2 last
     {{W2,W1},{W3,W1},{W4,W1}},                         // 04: W1 last
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(treewrite_2_3){
  Configuration conf = sc_obs_conf();
  std::string module = R"(
@vars = global [2 x i32] zeroinitializer, align 4
@var = global i32 0, align 4

define i8* @writer(i8* %argp) {
  %arg = ptrtoint i8* %argp to i64
  %j = trunc i64 %arg to i32
  %i = ashr i64 %arg, 32
  %varsp = getelementptr inbounds [2 x i32], [2 x i32]* @vars, i64 0, i64 %i
  store i32 %j, i32* %varsp, align 4
  ret i8* null
}

define i8* @collector(i8* %arg) {
  %W1 = alloca i64
  %W2 = alloca i64
  %W3 = alloca i64
  %i = ptrtoint i8* %arg to i64
  %is = shl i64 %i, 32
  %iN = mul nsw i64 %i, 3
  %base = or i64 %iN, %is
  %arg1 = inttoptr i64 %base to i8*
  call i32 @pthread_create(i64* %W1, %attr_t* null, i8* (i8*)* @writer, i8* %arg1)
  %bp1 = add nsw i64 %base, 1
  %arg2 = inttoptr i64 %bp1 to i8*
  call i32 @pthread_create(i64* %W2, %attr_t* null, i8* (i8*)* @writer, i8* %arg2)
  %bp2 = add nsw i64 %base, 2
  %arg3 = inttoptr i64 %bp2 to i8*
  call i32 @pthread_create(i64* %W3, %attr_t* null, i8* (i8*)* @writer, i8* %arg3)
  %W1v = load i64, i64* %W1
  call i32 @pthread_join(i64 %W1v, i8** null)
  %W2v = load i64, i64* %W2
  call i32 @pthread_join(i64 %W2v, i8** null)
  %W3v = load i64, i64* %W3
  call i32 @pthread_join(i64 %W3v, i8** null)
  %varsp = getelementptr inbounds [2 x i32], [2 x i32]* @vars, i64 0, i64 %i
  %val = load i32, i32* %varsp, align 4
  store i32 %val, i32* @var, align 4
  ret i8* null
}

define i32 @main() {
  %C1 = alloca i64
  %C2 = alloca i64
  call i32 @pthread_create(i64* %C1, %attr_t* null, i8* (i8*)* @collector, i8* inttoptr (i64 0 to i8*))
  call i32 @pthread_create(i64* %C2, %attr_t* null, i8* (i8*)* @collector, i8* inttoptr (i64 1 to i8*))
  %C1v = load i64, i64* %C1
  call i32 @pthread_join(i64 %C1v, i8** null)
  %C2v = load i64, i64* %C2
  call i32 @pthread_join(i64 %C2v, i8** null)
  %res = load i32, i32* @var
  ret i32 %res
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8* (i8*)*, i8*)
declare i32 @pthread_join(i64, i8**)
)";
  std::unique_ptr<DPORDriver> driver(DPORDriver::parseIR(module, conf));
  DPORDriver::Result res = driver->run();

  CPid P;
  CPid C1 = P.spawn(0), C2 = P.spawn(1);
  CPid W11 = C1.spawn(0), W12 = C1.spawn(1), W13 = C1.spawn(2);
  CPid W21 = C2.spawn(0), W22 = C2.spawn(1), W23 = C2.spawn(2);
  IID<CPid>
    W11w(W11,5),W12w(W12,5),W13w(W13,5),
    W21w(W21,5),W22w(W22,5),W23w(W23,5),
    C1w(C1,24),C2w(C2,24);
  DPORDriver_test::trace_set_spec expected =
    DPORDriver_test::spec_xprod
    ({{{{W11w,W13w},{W12w,W13w}},                 // 01: W13 last
       {{W11w,W12w},{W13w,W12w}},                 // 02: W12 last
       {{W12w,W11w},{W13w,W11w}}},                // 03: W11 last
      {{{W21w,W23w},{W22w,W23w}},                 // 01: W23 last
       {{W21w,W22w},{W23w,W22w}},                 // 02: W22 last
       {{W22w,W21w},{W23w,W21w}}},                // 03: W21 last
      {{{C1w,C2w}},                               // 01: C2 last
       {{C2w,C1w}}},                              // 02: C1 last
     });
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(treewrite_3_3){
  Configuration conf = sc_obs_conf();
  std::string module = R"(
@vars = global [3 x i32] zeroinitializer, align 4
@var = global i32 0, align 4

define i8* @writer(i8* %argp) {
  %arg = ptrtoint i8* %argp to i64
  %j = trunc i64 %arg to i32
  %i = ashr i64 %arg, 32
  %varsp = getelementptr inbounds [3 x i32], [3 x i32]* @vars, i64 0, i64 %i
  store i32 %j, i32* %varsp, align 4
  ret i8* null
}

define i8* @collector(i8* %arg) {
  %W1 = alloca i64
  %W2 = alloca i64
  %W3 = alloca i64
  %i = ptrtoint i8* %arg to i64
  %is = shl i64 %i, 32
  %iN = mul nsw i64 %i, 3
  %base = or i64 %iN, %is
  %arg1 = inttoptr i64 %base to i8*
  call i32 @pthread_create(i64* %W1, %attr_t* null, i8* (i8*)* @writer, i8* %arg1)
  %bp1 = add nsw i64 %base, 1
  %arg2 = inttoptr i64 %bp1 to i8*
  call i32 @pthread_create(i64* %W2, %attr_t* null, i8* (i8*)* @writer, i8* %arg2)
  %bp2 = add nsw i64 %base, 2
  %arg3 = inttoptr i64 %bp2 to i8*
  call i32 @pthread_create(i64* %W3, %attr_t* null, i8* (i8*)* @writer, i8* %arg3)
  %W1v = load i64, i64* %W1
  call i32 @pthread_join(i64 %W1v, i8** null)
  %W2v = load i64, i64* %W2
  call i32 @pthread_join(i64 %W2v, i8** null)
  %W3v = load i64, i64* %W3
  call i32 @pthread_join(i64 %W3v, i8** null)
  %varsp = getelementptr inbounds [3 x i32], [3 x i32]* @vars, i64 0, i64 %i
  %val = load i32, i32* %varsp, align 4
  store i32 %val, i32* @var, align 4
  ret i8* null
}

define i32 @main() {
  %C1 = alloca i64
  %C2 = alloca i64
  %C3 = alloca i64
  call i32 @pthread_create(i64* %C1, %attr_t* null, i8* (i8*)* @collector, i8* inttoptr (i64 0 to i8*))
  call i32 @pthread_create(i64* %C2, %attr_t* null, i8* (i8*)* @collector, i8* inttoptr (i64 1 to i8*))
  call i32 @pthread_create(i64* %C3, %attr_t* null, i8* (i8*)* @collector, i8* inttoptr (i64 2 to i8*))
  %C1v = load i64, i64* %C1
  call i32 @pthread_join(i64 %C1v, i8** null)
  %C2v = load i64, i64* %C2
  call i32 @pthread_join(i64 %C2v, i8** null)
  %C3v = load i64, i64* %C3
  call i32 @pthread_join(i64 %C3v, i8** null)
  %res = load i32, i32* @var
  ret i32 %res
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8* (i8*)*, i8*)
declare i32 @pthread_join(i64, i8**)
)";
  std::unique_ptr<DPORDriver> driver(DPORDriver::parseIR(module, conf));
  DPORDriver::Result res = driver->run();

  CPid P;
  CPid C1 = P.spawn(0), C2 = P.spawn(1), C3 = P.spawn(2);
  CPid W11 = C1.spawn(0), W12 = C1.spawn(1), W13 = C1.spawn(2);
  CPid W21 = C2.spawn(0), W22 = C2.spawn(1), W23 = C2.spawn(2);
  CPid W31 = C3.spawn(0), W32 = C3.spawn(1), W33 = C3.spawn(2);
  IID<CPid>
    W11w(W11,5),W12w(W12,5),W13w(W13,5),
    W21w(W21,5),W22w(W22,5),W23w(W23,5),
    W31w(W31,5),W32w(W32,5),W33w(W33,5),
    C1w(C1,24),C2w(C2,24),C3w(C3,24);
  DPORDriver_test::trace_set_spec expected =
    DPORDriver_test::spec_xprod
    ({{{{W11w,W13w},{W12w,W13w}},                 // 01: W13 last
       {{W11w,W12w},{W13w,W12w}},                 // 02: W12 last
       {{W12w,W11w},{W13w,W11w}}},                // 03: W11 last
      {{{W21w,W23w},{W22w,W23w}},                 // 01: W23 last
       {{W21w,W22w},{W23w,W22w}},                 // 02: W22 last
       {{W22w,W21w},{W23w,W21w}}},                // 03: W21 last
      {{{W31w,W33w},{W32w,W33w}},                 // 01: W33 last
       {{W31w,W32w},{W33w,W32w}},                 // 02: W32 last
       {{W32w,W31w},{W33w,W31w}}},                // 03: W31 last
      {{{C1w,C3w},{C2w,C3w}},                     // 01: C3 last
       {{C1w,C2w},{C3w,C2w}},                     // 02: C2 last
       {{C2w,C1w},{C3w,C1w}}},                    // 03: C1 last
     });
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(condtree_2_3r){
  Configuration conf = sc_obs_conf();
  std::string module = R"(
@var_a = global i64 0, align 8
@var_b = global i64 0, align 8

define i8* @writer_a(i8* %arg) {
  %i = ptrtoint i8* %arg to i64
  store i64 %i, i64* @var_a, align 8
  ret i8* null
}

define i8* @writer_b(i8* %arg) {
  %i = ptrtoint i8* %arg to i64
  store i64 %i, i64* @var_b, align 8
  ret i8* null
}

define i32 @main() {
  %a0 = alloca i64
  %a1 = alloca i64
  %a2 = alloca i64
  %b0 = alloca i64
  %b1 = alloca i64
  %b2 = alloca i64
  call i32 @pthread_create(i64* %a0, %attr_t* null, i8* (i8*)* @writer_a, i8* null)
  call i32 @pthread_create(i64* %a1, %attr_t* null, i8* (i8*)* @writer_a, i8* inttoptr (i64 1 to i8*))
  call i32 @pthread_create(i64* %a2, %attr_t* null, i8* (i8*)* @writer_a, i8* inttoptr (i64 2 to i8*))
  call i32 @pthread_create(i64* %b0, %attr_t* null, i8* (i8*)* @writer_b, i8* null)
  call i32 @pthread_create(i64* %b1, %attr_t* null, i8* (i8*)* @writer_b, i8* inttoptr (i64 1 to i8*))
  call i32 @pthread_create(i64* %b2, %attr_t* null, i8* (i8*)* @writer_b, i8* inttoptr (i64 2 to i8*))
  %a0val = load i64, i64* %a0
  %a1val = load i64, i64* %a1
  %a2val = load i64, i64* %a2
  %b0val = load i64, i64* %b0
  %b1val = load i64, i64* %b1
  %b2val = load i64, i64* %b2
  call i32 @pthread_join(i64 %a0val, i8** null)
  call i32 @pthread_join(i64 %a1val, i8** null)
  call i32 @pthread_join(i64 %a2val, i8** null)
  call i32 @pthread_join(i64 %b0val, i8** null)
  call i32 @pthread_join(i64 %b1val, i8** null)
  call i32 @pthread_join(i64 %b2val, i8** null)
  %bv = load i64, i64* @var_b, align 8
  %bz = icmp eq i64 %bv, 0
  br i1 %bz, label %bzl, label %bnzl

bzl:
  ret i32 1

bnzl:
  %av = load i64, i64* @var_a, align 8
  %anz = icmp ne i64 %av, 0
  %anze = sext i1 %anz to i32
  ret i32 %anze
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8* (i8*)*, i8*)
declare i32 @pthread_join(i64, i8**)
)";
  std::unique_ptr<DPORDriver> driver(DPORDriver::parseIR(module, conf));
  DPORDriver::Result res = driver->run();

  CPid P;
  CPid A0 = P.spawn(0), A1 = P.spawn(1), A2 = P.spawn(2);
  CPid B0 = P.spawn(3), B1 = P.spawn(4), B2 = P.spawn(5);
  IID<CPid>
    WA0(A0,2),WA1(A1,2),WA2(A2,2),
    WB0(B0,2),WB1(B1,2),WB2(B2,2),
    RA(P,28),RB(P,25);
  DPORDriver_test::trace_set_spec expected =
    DPORDriver_test::spec_xprod
    ({{{{WB0,WB1},{WB2,WB1}},                     // 01: WB1 last
       {{WB0,WB2},{WB1,WB2}}},                    // 03: WB2 last
      {{{WA1,WA0},{WA2,WA0}},                     // 01: WA0 last
       {{WA0,WA1},{WA2,WA1}},                     // 02: WA1 last
       {{WA0,WA2},{WA1,WA2}}},                    // 03: WA2 last
     });
  expected.push_back({{WB1,WB0},{WB2,WB0}});      // WB0 last
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(condtree_2_3f){
  Configuration conf = sc_obs_conf();
  std::string module = R"(
@var_a = global i64 0, align 8
@var_b = global i64 0, align 8

define i8* @writer_a(i8* %arg) {
  %i = ptrtoint i8* %arg to i64
  store i64 %i, i64* @var_a, align 8
  ret i8* null
}

define i8* @writer_b(i8* %arg) {
  %i = ptrtoint i8* %arg to i64
  store i64 %i, i64* @var_b, align 8
  ret i8* null
}

define i32 @main() {
  %a0 = alloca i64
  %a1 = alloca i64
  %a2 = alloca i64
  %b0 = alloca i64
  %b1 = alloca i64
  %b2 = alloca i64
  call i32 @pthread_create(i64* %a0, %attr_t* null, i8* (i8*)* @writer_a, i8* null)
  call i32 @pthread_create(i64* %a1, %attr_t* null, i8* (i8*)* @writer_a, i8* inttoptr (i64 1 to i8*))
  call i32 @pthread_create(i64* %a2, %attr_t* null, i8* (i8*)* @writer_a, i8* inttoptr (i64 2 to i8*))
  call i32 @pthread_create(i64* %b0, %attr_t* null, i8* (i8*)* @writer_b, i8* null)
  call i32 @pthread_create(i64* %b1, %attr_t* null, i8* (i8*)* @writer_b, i8* inttoptr (i64 1 to i8*))
  call i32 @pthread_create(i64* %b2, %attr_t* null, i8* (i8*)* @writer_b, i8* inttoptr (i64 2 to i8*))
  %a0val = load i64, i64* %a0
  %a1val = load i64, i64* %a1
  %a2val = load i64, i64* %a2
  %b0val = load i64, i64* %b0
  %b1val = load i64, i64* %b1
  %b2val = load i64, i64* %b2
  call i32 @pthread_join(i64 %a0val, i8** null)
  call i32 @pthread_join(i64 %a1val, i8** null)
  call i32 @pthread_join(i64 %a2val, i8** null)
  call i32 @pthread_join(i64 %b0val, i8** null)
  call i32 @pthread_join(i64 %b1val, i8** null)
  call i32 @pthread_join(i64 %b2val, i8** null)
  %av = load i64, i64* @var_a, align 8
  %az = icmp eq i64 %av, 0
  br i1 %az, label %azl, label %anzl

azl:
  ret i32 0

anzl:
  %bv = load i64, i64* @var_b, align 8
  %bz = icmp eq i64 %bv, 0
  %ret = select i1 %bz, i32 1, i32 2
  ret i32 %ret
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8* (i8*)*, i8*)
declare i32 @pthread_join(i64, i8**)
)";
  std::unique_ptr<DPORDriver> driver(DPORDriver::parseIR(module, conf));
  DPORDriver::Result res = driver->run();

  CPid P;
  CPid A0 = P.spawn(0), A1 = P.spawn(1), A2 = P.spawn(2);
  CPid B0 = P.spawn(3), B1 = P.spawn(4), B2 = P.spawn(5);
  IID<CPid>
    WA0(A0,2),WA1(A1,2),WA2(A2,2),
    WB0(B0,2),WB1(B1,2),WB2(B2,2);
  DPORDriver_test::trace_set_spec expected =
    DPORDriver_test::spec_xprod
    ({{{{WA0,WA1},{WA2,WA1}},                     // 01: WA1 last
       {{WA0,WA2},{WA1,WA2}}},                    // 03: WA2 last
      {{{WB1,WB0},{WB2,WB0}},                     // 01: WB0 last
       {{WB0,WB1},{WB2,WB1}},                     // 02: WB1 last
       {{WB0,WB2},{WB1,WB2}}},                    // 03: WB2 last
     });
  expected.push_back({{WA1,WA0},{WA2,WA0}});      // WA0 last
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(condtree_3_3f){
  Configuration conf = sc_obs_conf();
  std::string module = R"(
@var_a = global i64 0, align 8
@var_b = global i64 0, align 8
@var_c = global i64 0, align 8

define i8* @writer_a(i8* %arg) {
  %i = ptrtoint i8* %arg to i64
  store i64 %i, i64* @var_a, align 8
  ret i8* null
}

define i8* @writer_b(i8* %arg) {
  %i = ptrtoint i8* %arg to i64
  store i64 %i, i64* @var_b, align 8
  ret i8* null
}

define i8* @writer_c(i8* %arg) {
  %i = ptrtoint i8* %arg to i64
  store i64 %i, i64* @var_c, align 8
  ret i8* null
}

define i32 @main() {
  %a0 = alloca i64
  %a1 = alloca i64
  %a2 = alloca i64
  %b0 = alloca i64
  %b1 = alloca i64
  %b2 = alloca i64
  %c0 = alloca i64
  %c1 = alloca i64
  %c2 = alloca i64
  call i32 @pthread_create(i64* %a0, %attr_t* null, i8* (i8*)* @writer_a, i8* null)
  call i32 @pthread_create(i64* %a1, %attr_t* null, i8* (i8*)* @writer_a, i8* inttoptr (i64 1 to i8*))
  call i32 @pthread_create(i64* %a2, %attr_t* null, i8* (i8*)* @writer_a, i8* inttoptr (i64 2 to i8*))
  call i32 @pthread_create(i64* %b0, %attr_t* null, i8* (i8*)* @writer_b, i8* null)
  call i32 @pthread_create(i64* %b1, %attr_t* null, i8* (i8*)* @writer_b, i8* inttoptr (i64 1 to i8*))
  call i32 @pthread_create(i64* %b2, %attr_t* null, i8* (i8*)* @writer_b, i8* inttoptr (i64 2 to i8*))
  call i32 @pthread_create(i64* %c0, %attr_t* null, i8* (i8*)* @writer_c, i8* null)
  call i32 @pthread_create(i64* %c1, %attr_t* null, i8* (i8*)* @writer_c, i8* inttoptr (i64 1 to i8*))
  call i32 @pthread_create(i64* %c2, %attr_t* null, i8* (i8*)* @writer_c, i8* inttoptr (i64 2 to i8*))
  %a0val = load i64, i64* %a0
  %a1val = load i64, i64* %a1
  %a2val = load i64, i64* %a2
  %b0val = load i64, i64* %b0
  %b1val = load i64, i64* %b1
  %b2val = load i64, i64* %b2
  %c0val = load i64, i64* %c0
  %c1val = load i64, i64* %c1
  %c2val = load i64, i64* %c2
  call i32 @pthread_join(i64 %a0val, i8** null)
  call i32 @pthread_join(i64 %a1val, i8** null)
  call i32 @pthread_join(i64 %a2val, i8** null)
  call i32 @pthread_join(i64 %b0val, i8** null)
  call i32 @pthread_join(i64 %b1val, i8** null)
  call i32 @pthread_join(i64 %b2val, i8** null)
  call i32 @pthread_join(i64 %c0val, i8** null)
  call i32 @pthread_join(i64 %c1val, i8** null)
  call i32 @pthread_join(i64 %c2val, i8** null)
  %av = load i64, i64* @var_a, align 8
  %az = icmp eq i64 %av, 0
  br i1 %az, label %azl, label %anzl

azl:
  ret i32 0

anzl:
  %bv = load i64, i64* @var_b, align 8
  %bz = icmp eq i64 %bv, 0
  br i1 %bz, label %bzl, label %bnzl

bzl:
  ret i32 1

bnzl:
  %cv = load i64, i64* @var_c, align 8
  %cz = icmp eq i64 %cv, 0
  %ret = select i1 %cz, i32 2, i32 3
  ret i32 %ret
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8* (i8*)*, i8*)
declare i32 @pthread_join(i64, i8**)
)";
  std::unique_ptr<DPORDriver> driver(DPORDriver::parseIR(module, conf));
  DPORDriver::Result res = driver->run();

  CPid P;
  CPid A0 = P.spawn(0), A1 = P.spawn(1), A2 = P.spawn(2);
  CPid B0 = P.spawn(3), B1 = P.spawn(4), B2 = P.spawn(5);
  CPid C0 = P.spawn(6), C1 = P.spawn(7), C2 = P.spawn(8);
  IID<CPid>
    WA0(A0,2),WA1(A1,2),WA2(A2,2),
    WB0(B0,2),WB1(B1,2),WB2(B2,2),
    WC0(C0,2),WC1(C1,2),WC2(C2,2);
  DPORDriver_test::trace_set_spec expected =
    DPORDriver_test::spec_xprod
    ({{{{WA0,WA1},{WA2,WA1}},                     // 01: WA1 last
       {{WA0,WA2},{WA1,WA2}}},                    // 02: WA2 last
      {{{WB0,WB1},{WB2,WB1}},                     // 01: WB1 last
       {{WB0,WB2},{WB1,WB2}}},                    // 02: WB2 last
      {{{WC1,WC0},{WC2,WC0}},                     // 01: WC0 last
       {{WC0,WC1},{WC2,WC1}},                     // 02: WC1 last
       {{WC0,WC2},{WC1,WC2}}},                    // 03: WC2 last
     });
  DPORDriver_test::trace_set_spec more_expected =
    DPORDriver_test::spec_xprod
    ({{{{WA0,WA1},{WA2,WA1}},                     // 01: WA1 last
       {{WA0,WA2},{WA1,WA2}}},                    // 02: WA2 last
      {{{WB1,WB0},{WB2,WB0}}},                    // 01: WB0 last
     });
  expected.insert(expected.end(), more_expected.begin(), more_expected.end());
  expected.push_back({{WA1,WA0},{WA2,WA0}});      // WA0 last
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(condtree_3_3r){
  Configuration conf = sc_obs_conf();
  std::string module = R"(
@var_a = global i64 0, align 8
@var_b = global i64 0, align 8
@var_c = global i64 0, align 8

define i8* @writer_a(i8* %arg) {
  %i = ptrtoint i8* %arg to i64
  store i64 %i, i64* @var_a, align 8
  ret i8* null
}

define i8* @writer_b(i8* %arg) {
  %i = ptrtoint i8* %arg to i64
  store i64 %i, i64* @var_b, align 8
  ret i8* null
}

define i8* @writer_c(i8* %arg) {
  %i = ptrtoint i8* %arg to i64
  store i64 %i, i64* @var_c, align 8
  ret i8* null
}

define i32 @main() {
  %a0 = alloca i64
  %a1 = alloca i64
  %a2 = alloca i64
  %b0 = alloca i64
  %b1 = alloca i64
  %b2 = alloca i64
  %c0 = alloca i64
  %c1 = alloca i64
  %c2 = alloca i64
  call i32 @pthread_create(i64* %a0, %attr_t* null, i8* (i8*)* @writer_a, i8* null)
  call i32 @pthread_create(i64* %a1, %attr_t* null, i8* (i8*)* @writer_a, i8* inttoptr (i64 1 to i8*))
  call i32 @pthread_create(i64* %a2, %attr_t* null, i8* (i8*)* @writer_a, i8* inttoptr (i64 2 to i8*))
  call i32 @pthread_create(i64* %b0, %attr_t* null, i8* (i8*)* @writer_b, i8* null)
  call i32 @pthread_create(i64* %b1, %attr_t* null, i8* (i8*)* @writer_b, i8* inttoptr (i64 1 to i8*))
  call i32 @pthread_create(i64* %b2, %attr_t* null, i8* (i8*)* @writer_b, i8* inttoptr (i64 2 to i8*))
  call i32 @pthread_create(i64* %c0, %attr_t* null, i8* (i8*)* @writer_c, i8* null)
  call i32 @pthread_create(i64* %c1, %attr_t* null, i8* (i8*)* @writer_c, i8* inttoptr (i64 1 to i8*))
  call i32 @pthread_create(i64* %c2, %attr_t* null, i8* (i8*)* @writer_c, i8* inttoptr (i64 2 to i8*))
  %a0val = load i64, i64* %a0
  %a1val = load i64, i64* %a1
  %a2val = load i64, i64* %a2
  %b0val = load i64, i64* %b0
  %b1val = load i64, i64* %b1
  %b2val = load i64, i64* %b2
  %c0val = load i64, i64* %c0
  %c1val = load i64, i64* %c1
  %c2val = load i64, i64* %c2
  call i32 @pthread_join(i64 %a0val, i8** null)
  call i32 @pthread_join(i64 %a1val, i8** null)
  call i32 @pthread_join(i64 %a2val, i8** null)
  call i32 @pthread_join(i64 %b0val, i8** null)
  call i32 @pthread_join(i64 %b1val, i8** null)
  call i32 @pthread_join(i64 %b2val, i8** null)
  call i32 @pthread_join(i64 %c0val, i8** null)
  call i32 @pthread_join(i64 %c1val, i8** null)
  call i32 @pthread_join(i64 %c2val, i8** null)
  %cv = load i64, i64* @var_c, align 8
  %cz = icmp eq i64 %cv, 0
  br i1 %cz, label %czl, label %cnzl

czl:
  ret i32 2

cnzl:
  %bv = load i64, i64* @var_b, align 8
  %bz = icmp eq i64 %bv, 0
  br i1 %bz, label %bzl, label %bnzl

bzl:
  ret i32 1

bnzl:
  %av = load i64, i64* @var_a, align 8
  %anz = icmp ne i64 %av, 0
  %anze = sext i1 %anz to i32
  ret i32 %anze
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8* (i8*)*, i8*)
declare i32 @pthread_join(i64, i8**)
)";
  std::unique_ptr<DPORDriver> driver(DPORDriver::parseIR(module, conf));
  DPORDriver::Result res = driver->run();

  CPid P;
  CPid A0 = P.spawn(0), A1 = P.spawn(1), A2 = P.spawn(2);
  CPid B0 = P.spawn(3), B1 = P.spawn(4), B2 = P.spawn(5);
  CPid C0 = P.spawn(6), C1 = P.spawn(7), C2 = P.spawn(8);
  IID<CPid>
    WA0(A0,2),WA1(A1,2),WA2(A2,2),
    WB0(B0,2),WB1(B1,2),WB2(B2,2),
    WC0(C0,2),WC1(C1,2),WC2(C2,2);
  DPORDriver_test::trace_set_spec expected =
    DPORDriver_test::spec_xprod
    ({{{{WC0,WC1},{WC2,WC1}},                     // 01: WC1 last
       {{WC0,WC2},{WC1,WC2}}},                    // 02: WC2 last
      {{{WB0,WB1},{WB2,WB1}},                     // 01: WB1 last
       {{WB0,WB2},{WB1,WB2}}},                    // 02: WB2 last
      {{{WA1,WA0},{WA2,WA0}},                     // 01: WA0 last
       {{WA0,WA1},{WA2,WA1}},                     // 02: WA1 last
       {{WA0,WA2},{WA1,WA2}}},                    // 03: WA2 last
     });
  DPORDriver_test::trace_set_spec more_expected =
    DPORDriver_test::spec_xprod
    ({{{{WC0,WC1},{WC2,WC1}},                     // 01: WC1 last
       {{WC0,WC2},{WC1,WC2}}},                    // 02: WC2 last
      {{{WB1,WB0},{WB2,WB0}}},                    // 01: WB0 last
     });
  expected.insert(expected.end(), more_expected.begin(), more_expected.end());
  expected.push_back({{WC1,WC0},{WC2,WC0}});      // WC0 last
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(wrong_witness){
  /* Exposes a bug where the witness of a race was incorrectly chosen as
   * an "observer" of the race that happened-after another observer.
   * Test case could probably be massively simplified.
   * Bug should resurface if the special case race subsumption condition
   * added to fix this is commented out in
   * TSOTraceBuilder::compute_vclocks().
   */
  Configuration conf = sc_obs_conf();
  std::string module = R"#(
; ModuleID = 'wrong_witness.c'
source_filename = "proofexample3.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%union.pthread_attr_t = type { i64, [48 x i8] }

@c = global i32 0, align 4
@x = common global i32 0, align 4
@y = common global i32 0, align 4
@z = common global i32 0, align 4

; Function Attrs: noinline norecurse nounwind uwtable
define void @__VERIFIER_atomic_p() local_unnamed_addr #0 {
entry:
  store volatile i32 0, i32* @x, align 4, !tbaa !1
  store volatile i32 0, i32* @y, align 4, !tbaa !1
  ret void
}

; Function Attrs: norecurse nounwind uwtable
define noalias i8* @p(i8* nocapture readnone %arg) #1 {
entry:
  tail call void @__VERIFIER_atomic_p()
  ret i8* null
}

; Function Attrs: noinline norecurse nounwind uwtable
define void @__VERIFIER_atomic_q() local_unnamed_addr #0 {
entry:
  store volatile i32 1, i32* @y, align 4, !tbaa !1
  store volatile i32 1, i32* @z, align 4, !tbaa !1
  ret void
}

; Function Attrs: norecurse nounwind uwtable
define noalias i8* @q(i8* nocapture readnone %arg) #1 {
entry:
  tail call void @__VERIFIER_atomic_q()
  ret i8* null
}

; Function Attrs: noinline norecurse nounwind uwtable
define void @__VERIFIER_atomic_r() local_unnamed_addr #0 {
entry:
  store volatile i32 2, i32* @x, align 4, !tbaa !1
  store volatile i32 2, i32* @y, align 4, !tbaa !1
  store volatile i32 2, i32* @z, align 4, !tbaa !1
  ret void
}

; Function Attrs: norecurse nounwind uwtable
define noalias i8* @r(i8* nocapture readnone %arg) #1 {
entry:
  tail call void @__VERIFIER_atomic_r()
  ret i8* null
}

; Function Attrs: norecurse nounwind uwtable
define noalias i8* @s(i8* nocapture readnone %arg) #1 {
entry:
  store volatile i32 1, i32* @c, align 4, !tbaa !1
  ret i8* null
}

; Function Attrs: norecurse nounwind uwtable
define { i64, i32 } @oprime() local_unnamed_addr #1 {
entry:
  %0 = load volatile i32, i32* @x, align 4, !tbaa !1
  %1 = load volatile i32, i32* @y, align 4, !tbaa !1
  %2 = load volatile i32, i32* @z, align 4, !tbaa !1
  %retval.sroa.0.0.insert.ext = zext i32 %0 to i64
  %retval.sroa.0.4.insert.ext = zext i32 %1 to i64
  %retval.sroa.0.4.insert.shift = shl nuw i64 %retval.sroa.0.4.insert.ext, 32
  %retval.sroa.0.4.insert.insert = or i64 %retval.sroa.0.4.insert.shift, %retval.sroa.0.0.insert.ext
  %.fca.0.insert = insertvalue { i64, i32 } undef, i64 %retval.sroa.0.4.insert.insert, 0
  %.fca.1.insert = insertvalue { i64, i32 } %.fca.0.insert, i32 %2, 1
  ret { i64, i32 } %.fca.1.insert
}

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.start(i64, i8* nocapture) #2

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.end(i64, i8* nocapture) #2

; Function Attrs: noinline norecurse nounwind uwtable
define { i64, i32 } @__VERIFIER_atomic_oprime() local_unnamed_addr #0 {
entry:
  %0 = load volatile i32, i32* @x, align 4, !tbaa !1
  %1 = load volatile i32, i32* @y, align 4, !tbaa !1
  %2 = load volatile i32, i32* @z, align 4, !tbaa !1
  %retval.sroa.0.0.insert.ext = zext i32 %0 to i64
  %retval.sroa.0.4.insert.ext = zext i32 %1 to i64
  %retval.sroa.0.4.insert.shift = shl nuw i64 %retval.sroa.0.4.insert.ext, 32
  %retval.sroa.0.4.insert.insert = or i64 %retval.sroa.0.4.insert.shift, %retval.sroa.0.0.insert.ext
  %.fca.0.insert = insertvalue { i64, i32 } undef, i64 %retval.sroa.0.4.insert.insert, 0
  %.fca.1.insert = insertvalue { i64, i32 } %.fca.0.insert, i32 %2, 1
  ret { i64, i32 } %.fca.1.insert
}

; Function Attrs: nounwind uwtable
define i32 @main() local_unnamed_addr #3 {
entry:
  %pt = alloca i64, align 8
  %qt = alloca i64, align 8
  %rt = alloca i64, align 8
  %st = alloca i64, align 8
  %0 = bitcast i64* %pt to i8*
  call void @llvm.lifetime.start(i64 8, i8* %0) #6
  %1 = bitcast i64* %qt to i8*
  call void @llvm.lifetime.start(i64 8, i8* %1) #6
  %2 = bitcast i64* %rt to i8*
  call void @llvm.lifetime.start(i64 8, i8* %2) #6
  %3 = bitcast i64* %st to i8*
  call void @llvm.lifetime.start(i64 8, i8* %3) #6
  %call = call i32 @pthread_create(i64* nonnull %pt, %union.pthread_attr_t* null, i8* (i8*)* nonnull @p, i8* null) #6
  %call1 = call i32 @pthread_create(i64* nonnull %qt, %union.pthread_attr_t* null, i8* (i8*)* nonnull @q, i8* null) #6
  %call2 = call i32 @pthread_create(i64* nonnull %rt, %union.pthread_attr_t* null, i8* (i8*)* nonnull @r, i8* null) #6
  %call3 = call i32 @pthread_create(i64* nonnull %st, %union.pthread_attr_t* null, i8* (i8*)* nonnull @s, i8* null) #6
  %4 = load i64, i64* %pt, align 8, !tbaa !5
  %call4 = call i32 @pthread_join(i64 %4, i8** null) #6
  %5 = load i64, i64* %qt, align 8, !tbaa !5
  %call5 = call i32 @pthread_join(i64 %5, i8** null) #6
  %6 = load i64, i64* %rt, align 8, !tbaa !5
  %call6 = call i32 @pthread_join(i64 %6, i8** null) #6
  %7 = load volatile i32, i32* @c, align 4, !tbaa !1
  %tobool = icmp eq i32 %7, 0
  br i1 %tobool, label %if.else, label %if.then

if.then:                                          ; preds = %entry
  %call7 = call { i64, i32 } @oprime()
  br label %if.end

if.else:                                          ; preds = %entry
  %8 = load volatile i32, i32* @x, align 4, !tbaa !1
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  %9 = load i64, i64* %st, align 8, !tbaa !5
  %call9 = call i32 @pthread_join(i64 %9, i8** null) #6
  call void @llvm.lifetime.end(i64 8, i8* %3) #6
  call void @llvm.lifetime.end(i64 8, i8* %2) #6
  call void @llvm.lifetime.end(i64 8, i8* %1) #6
  call void @llvm.lifetime.end(i64 8, i8* %0) #6
  ret i32 0
}

; Function Attrs: nounwind
declare i32 @pthread_create(i64*, %union.pthread_attr_t*, i8* (i8*)*, i8*) local_unnamed_addr #4

declare i32 @pthread_join(i64, i8**) local_unnamed_addr #5

attributes #0 = { noinline norecurse nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { norecurse nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { argmemonly nounwind }
attributes #3 = { nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nounwind "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #5 = { "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #6 = { nounwind }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.9.1 (http:/llvm.org/git/clang.git 54f5752c3600d39ee8de62ba9ff304154baf5e80) (http:/llvm.org/git/llvm.git a093ef43dd592b729da46db4ff3057fef9a46023)"}
!1 = !{!2, !2, i64 0}
!2 = !{!"int", !3, i64 0}
!3 = !{!"omnipotent char", !4, i64 0}
!4 = !{!"Simple C/C++ TBAA"}
!5 = !{!6, !6, i64 0}
!6 = !{!"long", !3, i64 0}
)#";
  std::unique_ptr<DPORDriver> driver(DPORDriver::parseIR(module, conf));
  DPORDriver::Result res = driver->run();

  /* We don't really need to check anything; the bug expreses itself as
   * an assertion failure. The test case could be made "stronger" by
   * having the incorrect witnes be from a different process and
   * disabled where it is scheduled, but this is enough for now.
   */
  BOOST_CHECK(res.trace_count == 7);
}

BOOST_AUTO_TEST_CASE(fullmemobs){
  Configuration conf = sc_obs_conf();
  std::string module = R"#(
; ModuleID = '<stdin>'
source_filename = "fullmemobs.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%union.pthread_attr_t = type { i64, [48 x i8] }

@x = global i32 0, align 4

; Function Attrs: norecurse nounwind uwtable
define noalias i8* @p(i8* nocapture readnone %arg) #0 {
entry:
  store volatile i32 112, i32* @x, align 4
  ret i8* null
}

; Function Attrs: norecurse nounwind uwtable
define noalias i8* @q(i8* nocapture readnone %arg) #0 {
entry:
  store volatile i32 113, i32* @x, align 4
  ret i8* null
}

; Function Attrs: nounwind uwtable
define i32 @main() local_unnamed_addr #1 {
entry:
  %pt = alloca i64, align 8
  %qt = alloca i64, align 8
  %res = alloca i32, align 4
  %call = call i32 @pthread_create(i64* nonnull %pt, %union.pthread_attr_t* null, i8* (i8*)* nonnull @p, i8* null) #4
  %call1 = call i32 @pthread_create(i64* nonnull %qt, %union.pthread_attr_t* null, i8* (i8*)* nonnull @q, i8* null) #4
  %0 = load i64, i64* %pt, align 8
  %call2 = call i32 @pthread_join(i64 %0, i8** null) #4
  %1 = load i64, i64* %qt, align 8
  %call3 = call i32 @pthread_join(i64 %1, i8** null) #4
  %2 = bitcast i32* %res to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %2, i8* bitcast (i32* @x to i8*), i64 4, i32 4, i1 false)
  %3 = load i32, i32* %res, align 4
  ret i32 %3
}

; Function Attrs: nounwind
declare i32 @pthread_create(i64*, %union.pthread_attr_t*, i8* (i8*)*, i8*) local_unnamed_addr #2

declare i32 @pthread_join(i64, i8**) local_unnamed_addr #3

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i32, i1) #3

attributes #0 = { norecurse nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nounwind }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.9.1 (http:/llvm.org/git/clang.git 54f5752c3600d39ee8de62ba9ff304154baf5e80) (http:/llvm.org/git/llvm.git a093ef43dd592b729da46db4ff3057fef9a46023)"}
)#";
  std::unique_ptr<DPORDriver> driver(DPORDriver::parseIR(module, conf));
  DPORDriver::Result res = driver->run();

  /* This could be replaced with a trace set spec */
  BOOST_CHECK(res.trace_count == 2);
}

BOOST_AUTO_TEST_SUITE_END()

#endif
