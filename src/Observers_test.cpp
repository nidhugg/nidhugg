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

BOOST_AUTO_TEST_SUITE_END()

#endif
