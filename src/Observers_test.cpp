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

BOOST_AUTO_TEST_SUITE_END()

#endif
