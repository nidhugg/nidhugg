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

BOOST_AUTO_TEST_CASE(Minimal_computation){
  Configuration conf = DPORDriver_test::get_tso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
define i32 @main(){
  ret i32 0
}
)"),conf);

  DPORDriver::Result res = driver->run();

  BOOST_CHECK(res.all_traces.size() == 1);
  BOOST_CHECK(static_cast<const IIDSeqTrace*>(res.all_traces[0])->get_computation() == std::vector<IID<CPid> >({{CPid(),1}}));

  delete driver;
}

BOOST_AUTO_TEST_CASE(Small_dekker){
  Configuration conf = DPORDriver_test::get_tso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p0(i8* %arg){
  store i32 1, i32* @x, align 4
  %1 = load i32, i32* @y, align 4
  ret i8* null
}

define i8* @p1(i8* %arg){
  store i32 1, i32* @y, align 4
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
    ry0(P0,4),
    uy1(U1,1),
    rx1(P1,2);

  DPORDriver_test::trace_set_spec spec =
    {{{ux0,rx1},{uy1,ry0}},
     {{ux0,rx1},{ry0,uy1}},
     {{rx1,ux0},{uy1,ry0}},
     {{rx1,ux0},{ry0,uy1}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,spec,conf));

  delete driver;
}

BOOST_AUTO_TEST_CASE(Small_peterson){
  Configuration conf = DPORDriver_test::get_tso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4
@t = global i32 0, align 4

define i8* @p0(i8* %arg){
  store i32 1, i32* @x, align 4
  store i32 1, i32* @t, align 4
  %rt = load i32, i32* @t, align 4
  %ry = load i32, i32* @y, align 4
  ret i8* null
}

define i8* @p1(i8* %arg){
  store i32 1, i32* @y, align 4
  store i32 0, i32* @t, align 4
  %rt = load i32, i32* @t, align 4
  %rx = load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %union.pthread_attr_t* null, i8*(i8*)* @p1, i8* null)
  call i8* @p0(i8* null)
  ret i32 0
}

%union.pthread_attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %union.pthread_attr_t*, i8*(i8*)*, i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();

  CPid P0;
  CPid U0 = P0.aux(0);
  CPid P1 = P0.spawn(0);
  CPid U1 = P1.aux(0);

  IID<CPid>
    ux0(U0,1),
    ut0(U0,2),
    rt0(P0,5),
    ry0(P0,6),
    uy1(U1,1),
    ut1(U1,2),
    rt1(P1,3),
    rx1(P1,4);

  DPORDriver_test::trace_set_spec expected =
    {{{ux0,rx1},{ut0,ut1},{rt0,ut1},{ry0,uy1}},
     {{ux0,rx1},{ut0,ut1},{rt0,ut1},{uy1,ry0}},
     {{ux0,rx1},{ut0,ut1},{ut1,rt0},{uy1,ry0}},
     {{ux0,rx1},{ut1,ut0},{ut0,rt1},{ry0,uy1}},
     {{ux0,rx1},{ut1,ut0},{ut0,rt1},{uy1,ry0}},
     {{ux0,rx1},{ut1,ut0},{rt1,ut0},{ry0,uy1}},
     {{ux0,rx1},{ut1,ut0},{rt1,ut0},{uy1,ry0}},
     {{rx1,ux0},{ut0,ut1},{rt0,ut1},{ry0,uy1}},
     {{rx1,ux0},{ut0,ut1},{rt0,ut1},{uy1,ry0}},
     {{rx1,ux0},{ut0,ut1},{ut1,rt0},{uy1,ry0}},
     {{rx1,ux0},{ut1,ut0},{rt1,ut0},{ry0,uy1}},
     {{rx1,ux0},{ut1,ut0},{rt1,ut0},{uy1,ry0}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));

  delete driver;
}

BOOST_AUTO_TEST_CASE(ROWE_1){
  /* Check that precisely three canonical forms are generated.
   *
   * w(x)        w(x)
   *
   * r(x)
   */
  Configuration conf = DPORDriver_test::get_tso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p0(i8* %arg){
  store i32 1, i32* @x, align 4
  load i32, i32* @x, align 4
  ret i8* null
}

define i8* @p1(i8* %arg){
  store i32 2, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call i8* @p0(i8* null)
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();

  delete driver;

  IID<CPid>
    ux0(CPid({0},0),1),
    rx0(CPid(),4),
    ux1(CPid({0,0},0),1);

  DPORDriver_test::trace_set_spec expected =
    {{{ux0,ux1},{rx0,ux1}},
     {{ux0,ux1},{ux1,rx0}},
     {{ux1,ux0}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));

}

BOOST_AUTO_TEST_CASE(ROWE_2){
  /* Same as ROWE_1, but with process roles swapped. This is to
   * provoke different scheduling.
   *
   * Check that precisely three canonical forms are generated.
   *
   * w(x)        w(x)
   *
   * r(x)
   */
  Configuration conf = DPORDriver_test::get_tso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p0(i8* %arg){
  store i32 1, i32* @x, align 4
  load i32, i32* @x, align 4
  ret i8* null
}

define i8* @p1(i8* %arg){
  store i32 2, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p0, i8* null)
  call i8* @p1(i8* null)
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();

  delete driver;

  IID<CPid>
    ux0(CPid({0},0),1),
    rx1(CPid({0,0}),2),
    ux1(CPid({0,0},0),1);

  DPORDriver_test::trace_set_spec expected =
    {{{ux0,ux1},{ux0,rx1}},
     {{ux1,ux0},{ux0,rx1}},
     {{ux1,ux0},{rx1,ux0}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(read_read_write_1){
  Configuration conf = DPORDriver_test::get_tso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p0(i8* %arg){
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i8* @p1(i8* %arg){
  load i32, i32* @x, align 4
  ret i8* null
}

define i8* @p2(i8* %arg){
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@p1, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@p2, i8* null)
  call i8* @p0(i8* null)
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  IID<CPid>
    ux0(CPid({0},0),1),
    rx1(CPid({0,0}),1),
    rx2(CPid({0,1}),1);

  DPORDriver_test::trace_set_spec expected =
    {{{ux0,rx1},{ux0,rx2}},
     {{ux0,rx1},{rx2,ux0}},
     {{rx1,ux0},{ux0,rx2}},
     {{rx1,ux0},{rx2,ux0}}};

  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(read_read_write_2){
  /* Same as read_read_write_1, but with process roles switched. This
   * is to get a different scheduling.
   */
  Configuration conf = DPORDriver_test::get_tso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p0(i8* %arg){
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i8* @p1(i8* %arg){
  load i32, i32* @x, align 4
  ret i8* null
}

define i8* @p2(i8* %arg){
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@p2, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@p0, i8* null)
  call i8* @p1(i8* null)
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  IID<CPid>
    ux0(CPid({0,1},0),1),
    rx1(CPid({0}),4),
    rx2(CPid({0,0}),1);

  DPORDriver_test::trace_set_spec expected =
    {{{ux0,rx1},{ux0,rx2}},
     {{ux0,rx1},{rx2,ux0}},
     {{rx1,ux0},{ux0,rx2}},
     {{rx1,ux0},{rx2,ux0}}};

  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(before_update){
  /* Regression test checking that the computation does not get stuck
   * for the example below.
   *
   * This would happen because the dependency between a store and the
   * resulting update was not properly registered in TSOTraceBuilder.
   */
  Configuration conf = DPORDriver_test::get_tso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  load i32, i32* @x, align 4
  store i32 1, i32* @y, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  load i32, i32* @y, align 4
  store i32 1, i32* @x
  ret i32 0
}

%attr_t = type { i64 , [48 x i8] }
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res =
    driver->run();
  delete driver;

  IID<CPid>
    rx1(CPid({0,0}),1),
    uy1(CPid({0,0},0),1),
    ry0(CPid(),2),
    ux0(CPid({0},0),1);

  DPORDriver_test::trace_set_spec expected =
    {{{ry0,uy1},{ux0,rx1}},
     {{ry0,uy1},{rx1,ux0}},
     {{uy1,ry0},{rx1,ux0}}};

  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(fib_simple){
  Configuration conf = DPORDriver_test::get_tso_conf();
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

BOOST_AUTO_TEST_CASE(fib_simple_n2){
  Configuration conf = DPORDriver_test::get_tso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@i = global i32 1, align 4
@j = global i32 1, align 4

define i8* @p1(i8* %arg){
  %1 = load i32, i32* @i, align 4
  %2 = load i32, i32* @j, align 4
  %3 = add i32 %1, %2
  store i32 %3, i32* @i, align 4
  %4 = load i32, i32* @i, align 4
  %5 = load i32, i32* @j, align 4
  %6 = add i32 %4, %5
  store i32 %6, i32* @i, align 4
  ret i8* null
}

define i8* @p2(i8* %arg){
  %1 = load i32, i32* @i, align 4
  %2 = load i32, i32* @j, align 4
  %3 = add i32 %1, %2
  store i32 %3, i32* @j, align 4
  %4 = load i32, i32* @i, align 4
  %5 = load i32, i32* @j, align 4
  %6 = add i32 %4, %5
  store i32 %6, i32* @j, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p2, i8* null)

  %3 = load i32, i32* @i, align 4
  %4 = load i32, i32* @j, align 4

  %icmp = icmp eq i32 %3, 8
  br i1 %icmp, label %error, label %exit

error:
  call void @__assert_fail()
  br label %exit

exit:
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(fib_simple_n2_true){
  Configuration conf = DPORDriver_test::get_tso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@i = global i32 1, align 4
@j = global i32 1, align 4

define i8* @p1(i8* %arg){
  %1 = load i32, i32* @i, align 4
  %2 = load i32, i32* @j, align 4
  %3 = add i32 %1, %2
  store i32 %3, i32* @i, align 4
  %4 = load i32, i32* @i, align 4
  %5 = load i32, i32* @j, align 4
  %6 = add i32 %4, %5
  store i32 %6, i32* @i, align 4
  ret i8* null
}

define i8* @p2(i8* %arg){
  %1 = load i32, i32* @i, align 4
  %2 = load i32, i32* @j, align 4
  %3 = add i32 %1, %2
  store i32 %3, i32* @j, align 4
  %4 = load i32, i32* @i, align 4
  %5 = load i32, i32* @j, align 4
  %6 = add i32 %4, %5
  store i32 %6, i32* @j, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p2, i8* null)

  %3 = load i32, i32* @i, align 4
  %4 = load i32, i32* @j, align 4

  %icmp = icmp sgt i32 %3, 8
  br i1 %icmp, label %error, label %exit

error:
  call void @__assert_fail()
  br label %exit

exit:
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare void @__assert_fail() nounwind noreturn
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(blacken_change_clock){
  Configuration conf = DPORDriver_test::get_tso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@i = global i32 1, align 4
@j = global i32 1, align 4

define i8* @p1(i8* %arg){
  load i32, i32* @j, align 4
  store i32 2, i32* @i, align 4
  load i32, i32* @j, align 4
  store i32 3, i32* @i, align 4
  ret i8* null
}

define i8* @p2(i8* %arg){
  load i32, i32* @i, align 4
  store i32 2, i32* @j, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p2, i8* null)

  load i32, i32* @i, align 4

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
    p0ri(P0,3),
    p1rj1(P1,1), p1rj2(P1,3), p1ui1(U1,1), p1ui2(U1,2),
    p2ri(P2,1), p2uj(U2,1);
  DPORDriver_test::trace_set_spec expected =
    {
      /* p2ri -> p1ui1 */
      {{p0ri,p1ui1},{p0ri,p1ui2}, {p2ri,p1ui1},{p2ri,p1ui2}, {p1rj1,p2uj},{p1rj2,p2uj}},
      {{p1ui1,p0ri},{p0ri,p1ui2}, {p2ri,p1ui1},{p2ri,p1ui2}, {p1rj1,p2uj},{p1rj2,p2uj}},
      {{p1ui1,p0ri},{p1ui2,p0ri}, {p2ri,p1ui1},{p2ri,p1ui2}, {p1rj1,p2uj},{p1rj2,p2uj}},

      {{p0ri,p1ui1},{p0ri,p1ui2}, {p2ri,p1ui1},{p2ri,p1ui2}, {p1rj1,p2uj},{p2uj,p1rj2}},
      {{p1ui1,p0ri},{p0ri,p1ui2}, {p2ri,p1ui1},{p2ri,p1ui2}, {p1rj1,p2uj},{p2uj,p1rj2}},
      {{p1ui1,p0ri},{p1ui2,p0ri}, {p2ri,p1ui1},{p2ri,p1ui2}, {p1rj1,p2uj},{p2uj,p1rj2}},

      {{p0ri,p1ui1},{p0ri,p1ui2}, {p2ri,p1ui1},{p2ri,p1ui2}, {p2uj,p1rj1},{p2uj,p1rj2}},
      {{p1ui1,p0ri},{p0ri,p1ui2}, {p2ri,p1ui1},{p2ri,p1ui2}, {p2uj,p1rj1},{p2uj,p1rj2}},
      {{p1ui1,p0ri},{p1ui2,p0ri}, {p2ri,p1ui1},{p2ri,p1ui2}, {p2uj,p1rj1},{p2uj,p1rj2}},

      /* p1ui1 -> p2ri -> p1ui2 */
      {{p0ri,p1ui1},{p0ri,p1ui2}, {p1ui1,p2ri},{p2ri,p1ui2}, {p1rj1,p2uj},{p1rj2,p2uj}},
      {{p1ui1,p0ri},{p0ri,p1ui2}, {p1ui1,p2ri},{p2ri,p1ui2}, {p1rj1,p2uj},{p1rj2,p2uj}},
      {{p1ui1,p0ri},{p1ui2,p0ri}, {p1ui1,p2ri},{p2ri,p1ui2}, {p1rj1,p2uj},{p1rj2,p2uj}},

      {{p0ri,p1ui1},{p0ri,p1ui2}, {p1ui1,p2ri},{p2ri,p1ui2}, {p1rj1,p2uj},{p2uj,p1rj2}},
      {{p1ui1,p0ri},{p0ri,p1ui2}, {p1ui1,p2ri},{p2ri,p1ui2}, {p1rj1,p2uj},{p2uj,p1rj2}},
      {{p1ui1,p0ri},{p1ui2,p0ri}, {p1ui1,p2ri},{p2ri,p1ui2}, {p1rj1,p2uj},{p2uj,p1rj2}},

      /* p1ui2 -> p2ri */
      {{p0ri,p1ui1},{p0ri,p1ui2}, {p1ui1,p2ri},{p1ui2,p2ri}, {p1rj1,p2uj},{p1rj2,p2uj}},
      {{p1ui1,p0ri},{p0ri,p1ui2}, {p1ui1,p2ri},{p1ui2,p2ri}, {p1rj1,p2uj},{p1rj2,p2uj}},
      {{p1ui1,p0ri},{p1ui2,p0ri}, {p1ui1,p2ri},{p1ui2,p2ri}, {p1rj1,p2uj},{p1rj2,p2uj}},
    };

  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Intrinsic){
  /* Check that calls to intrinsic functions do not change instruction
   * IDs.
   *
   * Since some intrinsic functions are lowered, causing the source
   * code to change during execution, there is a risk that they may
   * affect which IIDs correspond to which real instructions.
   */
  Configuration conf = DPORDriver_test::get_tso_conf();
#ifdef LLVM_METADATA_IS_VALUE
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @work(i8* %arg){
  %_arg = alloca i8*, align 8
  store i8* %arg, i8** %_arg, align 8
  load i32, i32* @x, align 4
  call void @llvm.dbg.declare(metadata !{i8** %_arg}, metadata !0)
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@work, i8* null)
  call i8* @work(i8* null)
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }

declare void @llvm.dbg.declare(metadata, metadata) nounwind readnone
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind

!llvm.module.flags = !{!1}
!0 = metadata !{i32 0}
!1 = metadata !{i32 2, metadata !"Debug Info Version", i32 )" LLVM_METADATA_VERSION_NUMBER_STR R"(}
)"),conf);
#else
#ifdef LLVM_DBG_DECLARE_TWO_ARGS
  std::string declarecall = "call void @llvm.dbg.declare(metadata i8** %_arg, metadata !0)";
  std::string declaredeclare = "declare void @llvm.dbg.declare(metadata, metadata) nounwind readnone";
#else
  std::string declarecall = "call void @llvm.dbg.declare(metadata i8** %_arg, metadata !0, metadata !0)";
  std::string declaredeclare = "declare void @llvm.dbg.declare(metadata, metadata, metadata) nounwind readnone";
#endif
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @work(i8* %arg){
  %_arg = alloca i8*, align 8
  store i8* %arg, i8** %_arg, align 8
  load i32, i32* @x, align 4
  )" + declarecall + R"(
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@work, i8* null)
  call i8* @work(i8* null)
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }

)" + declaredeclare + R"(
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind

!llvm.module.flags = !{!1}
!0 = !{i32 0}
!1 = !{i32 2, !"Debug Info Version", i32 )" LLVM_METADATA_VERSION_NUMBER_STR R"(}
)"),conf);
#endif

  DPORDriver::Result res = driver->run();
  delete driver;

  IID<CPid>
    rx0(CPid(),5), ux0(CPid({0},0),2),
    rx1(CPid({0,0}),3), ux1(CPid({0,0},0),2);

  DPORDriver_test::trace_set_spec expected =
    {{{ux0,ux1},{rx0,ux1},{ux0,rx1}},
     {{ux0,ux1},{rx0,ux1},{rx1,ux0}},
     {{ux1,ux0},{rx0,ux1},{rx1,ux0}},
     {{ux1,ux0},{ux1,rx0},{rx1,ux0}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(small_dekker_checker){
  Configuration conf = DPORDriver_test::get_tso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4
@c = global i32 0, align 4

define i8* @p0(i8* %arg){
  store i32 1, i32* @x, align 4
  %y = load i32, i32* @y, align 4
  %ycmp = icmp eq i32 %y, 0
  br i1 %ycmp, label %p0check, label %p0exit

p0check:
  %y2 = add i32 %y, %y
  %y3 = add i32 %y, %y
  %y4 = add i32 %y, %y
  %y5 = add i32 %y, %y
  %y6 = add i32 %y, %y
  load i32, i32* @c, align 4
  br label %p0exit

p0exit:
  store i32 1, i32* @c, align 4
  ret i8* null
}

define i8* @p1(i8* %arg){
  store i32 1, i32* @y, align 4
  %x = load i32, i32* @x, align 4
  %xcmp = icmp eq i32 %x, 0
  br i1 %xcmp, label %p1check, label %p1exit

p1check:
  %x2 = add i32 %x, %x
  %x3 = add i32 %x, %x
  %x4 = add i32 %x, %x
  %x5 = add i32 %x, %x
  %x6 = add i32 %x, %x
  load i32, i32* @c, align 4
  br label %p1exit

p1exit:
  store i32 1, i32* @c, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call i8* @p0(i8* null)
  ret i32 0
}

%attr_t = type {i64,[48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0; CPid U0 = P0.aux(0);
  CPid P1 = P0.spawn(0); CPid U1 = P1.aux(0);
  IID<CPid>
    ux0(U0,1), ry0(P0,4), rc0(P0,12), uc0(U0,2),
    uy1(U1,1), rx1(P1,2), rc1(P1,10), uc1(U1,2);

  DPORDriver_test::trace_set_spec expected =
    {{{ux0,rx1},{ry0,uy1},{uc0,uc1},{rc0,uc1}},
     {{ux0,rx1},{ry0,uy1},{uc1,uc0},{rc0,uc1}},
     {{ux0,rx1},{ry0,uy1},{uc1,uc0},{uc1,rc0}},
     {{ux0,rx1},{uy1,ry0},{uc0,uc1}},
     {{ux0,rx1},{uy1,ry0},{uc1,uc0}},
     {{rx1,ux0},{ry0,uy1},{uc0,uc1},{rc0,uc1},{rc1,uc0}},
     {{rx1,ux0},{ry0,uy1},{uc0,uc1},{rc0,uc1},{uc0,rc1}},
     {{rx1,ux0},{ry0,uy1},{uc1,uc0},{rc0,uc1},{rc1,uc0}},
     {{rx1,ux0},{ry0,uy1},{uc1,uc0},{uc1,rc0},{rc1,uc0}},
     {{rx1,ux0},{uy1,ry0},{uc0,uc1},{rc1,uc0}},
     {{rx1,ux0},{uy1,ry0},{uc0,uc1},{uc0,rc1}},
     {{rx1,ux0},{uy1,ry0},{uc1,uc0},{rc1,uc0}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Disappearing_conflict){
  Configuration conf = DPORDriver_test::get_tso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p0(i8* %arg){
  %x1 = load i32, i32* @x, align 4
  %xcmp1 = icmp eq i32 %x1, 0
  br i1 %xcmp1, label %l2, label %p0exit

l2:
  %x2 = load i32, i32* @x, align 4
  %xcmp2 = icmp eq i32 %x2, 0
  br i1 %xcmp2, label %l3, label %p0exit

l3:
  %x3 = load i32, i32* @x, align 4
  %xcmp3 = icmp eq i32 %x3, 0
  br i1 %xcmp3, label %l4, label %p0exit

l4:
  %x4 = load i32, i32* @x, align 4
  %xcmp4 = icmp eq i32 %x4, 0
  br i1 %xcmp4, label %l5, label %p0exit

l5:
  %x5 = load i32, i32* @x, align 4
  %xcmp5 = icmp eq i32 %x5, 0
  br i1 %xcmp5, label %l6, label %p0exit

l6:
  load i32, i32* @y, align 4
  br label %p0exit

p0exit:
  ret i8* null
}

define i8* @p1(i8* %arg){
  store i32 1, i32* @y, align 4
  store i32 1, i32* @x, align 4
  add i32 0, 0
  add i32 0, 0
  add i32 0, 0
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call i8* @p0(i8* null)
  ret i32 0
}

%attr_t = type {i64,[48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  IID<CPid>
    rx01(CPid(),3),
    rx02(CPid(),6),
    rx03(CPid(),9),
    rx04(CPid(),12),
    rx05(CPid(),15),
    ry0(CPid(),18),
    uy1(CPid({0,0},0),1),
    ux1(CPid({0,0},0),2);

  DPORDriver_test::trace_set_spec expected =
    {{{ux1,rx01}},
     {{rx01,ux1},{ux1,rx02}},
     {{rx02,ux1},{ux1,rx03}},
     {{rx03,ux1},{ux1,rx04}},
     {{rx04,ux1},{ux1,rx05}},

     {{rx05,ux1},{uy1,ry0}},
     {{rx05,ux1},{ry0,uy1}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(rruu){
  /* r(x)          u(y)
   *  |             |
   * r(y)          u(x)
   */
  Configuration conf = DPORDriver_test::get_tso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 1, i32* @y, align 4
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@p1, i8* null)
  load i32, i32* @x, align 4
  load i32, i32* @y, align 4
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0;
  CPid P1 = P0.spawn(0); CPid U1 = P1.aux(0);
  IID<CPid>
    uy(U1,1), ux(U1,2),
    rx(P0,2), ry(P0,3);
  DPORDriver_test::trace_set_spec expected =
    {{{rx,ux},{ry,uy}},
     {{rx,ux},{uy,ry}},
     {{ux,rx},{uy,ry}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(own_black_read){
  /* r(x)           r(x)
   *  |              |
   *  V              V
   * w(x)           w(x)
   *  |              |
   *  V              V
   * u(x)           u(x)
   */
  Configuration conf = DPORDriver_test::get_tso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p(i8* %arg){
  load i32, i32* @x, align 4
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@p, i8* null)
  call i8* @p(i8* null)
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0; CPid U0 = P0.aux(0);
  CPid P1 = P0.spawn(0); CPid U1 = P1.aux(0);
  IID<CPid>
    rx0(P0,3), ux0(U0,1),
    rx1(P1,1), ux1(U1,1);
  DPORDriver_test::trace_set_spec expected =
    {{{ux0,ux1},{rx0,ux1},{ux0,rx1}},
     {{ux0,ux1},{rx0,ux1},{rx1,ux0}},
     {{ux1,ux0},{rx0,ux1},{rx1,ux0}},
     {{ux1,ux0},{ux1,rx0},{rx1,ux0}}
     /* Impossible
     {{ux0,ux1},{ux1,rx0},{ux0,rx1}},
     {{ux0,ux1},{ux1,rx0},{rx1,ux0}},
     {{ux1,ux0},{rx0,ux1},{ux0,rx1}},
     {{ux1,ux0},{ux1,rx0},{ux0,rx1}},
     */
    };

  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(own_black_read_2){
  /* r(x)
   *  |
   *  V
   * w(x)           w(x)
   *  |              |
   *  V              V
   * u(x)           u(x)
   */
  Configuration conf = DPORDriver_test::get_tso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@p1, i8* null)
  load i32, i32* @x, align 4
  store i32 1, i32* @x, align 4
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0; CPid U0 = P0.aux(0);
  CPid P1 = P0.spawn(0); CPid U1 = P1.aux(0);
  IID<CPid>
    rx0(P0,2), ux0(U0,1),
    ux1(U1,1);
  DPORDriver_test::trace_set_spec expected =
    {{{ux0,ux1},{rx0,ux1}},
     {{ux1,ux0},{rx0,ux1}},
     {{ux1,ux0},{ux1,rx0}},
     /* Impossible
     {{ux0,ux1},{ux1,rx0}},
     */
    };

  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(own_black_read_3){
  /*                r(x)
   *                 |
   *                 V
   * w(x)           w(x)
   *  |              |
   *  V              V
   * u(x)           u(x)
   */
  Configuration conf = DPORDriver_test::get_tso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  load i32, i32* @x, align 4
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)*@p1, i8* null)
  store i32 1, i32* @x, align 4
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0; CPid U0 = P0.aux(0);
  CPid P1 = P0.spawn(0); CPid U1 = P1.aux(0);
  IID<CPid>
    ux0(U0,1),
    rx1(P1,1), ux1(U1,1);
  DPORDriver_test::trace_set_spec expected =
    {{{ux0,ux1},{rx1,ux0}},
     {{ux0,ux1},{ux0,rx1}},
     {{ux1,ux0},{rx1,ux0}}
     /* Impossible
     {{ux1,ux0},{ux0,rx1}},
     */
    };

  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Refuse_1){
  /* Regression test: The store before the call to pthread_create
   * would cause the call to pthread_create to be refused, until the
   * update was performed. This would cause a bug where the
   * instruction count was wrong. Hence the subsequent load would be
   , would be
   * associated with the wrong IID.
   */
  Configuration conf = DPORDriver_test::get_tso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 2, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  store i32 1, i32* @x, align 4
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  load i32, i32* @x, align 4
  ret i32 0
}

%attr_t = type { i64, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0; CPid U0 = P0.aux(0);
  CPid P1 = P0.spawn(0); CPid U1 = P1.aux(0);
  IID<CPid>
    ux0(U0,1), rx0(P0,3),
    ux1(U1,1);
  DPORDriver_test::trace_set_spec expected =
    {{{ux0,ux1},{rx0,ux1}},
     {{ux0,ux1},{ux1,rx0}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Propagate){
  Configuration conf = DPORDriver_test::get_tso_conf();
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

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  store i32 1, i32* @x, align 4
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

BOOST_AUTO_TEST_CASE(Branch_sleep){
  Configuration conf = DPORDriver_test::get_tso_conf();
  conf.explore_all_traces = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@i = global i32 1, align 4
@j = global i32 1, align 4

define i8* @p1(i8* %arg){
  %j0 = load i32, i32* @j, align 4
  %i0 = add nsw i32 %j0, 1
  store i32 %i0, i32* @i, align 4
  %j1 = load i32, i32* @j, align 4
  %i1 = add nsw i32 %i0, %j1
  store i32 %i1, i32* @i, align 4
  %j2 = load i32, i32* @j, align 4
  %i2 = add nsw i32 %i1, %j2
  store i32 %i2, i32* @i, align 4
  %j3 = load i32, i32* @j, align 4
  %i3 = add nsw i32 %i2, %j3
  store i32 %i3, i32* @i, align 4
  ret i8* null
}

define i8* @p2(i8* %arg){
  %i0 = load i32, i32* @i, align 4
  %j0 = add nsw i32 1, %i0
  store i32 %j0, i32* @j, align 4
  %i1 = load i32, i32* @i, align 4
  %j1 = add nsw i32 %j0, %i1
  store i32 %j1, i32* @j, align 4
  %i2 = load i32, i32* @i, align 4
  %j2 = add nsw i32 %j1, %i2
  store i32 %j2, i32* @j, align 4
  %i3 = load i32, i32* @i, align 4
  %j3 = add nsw i32 %j2, %i3
  store i32 %j3, i32* @j, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p2, i8* null)
  %j = load i32, i32* @j, align 4
  %jcmp = icmp eq i32 %j, 55
  br i1 %jcmp, label %error, label %exit

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

  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(Atomic_store_1){
  Configuration conf = DPORDriver_test::get_tso_conf();
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

  CPid P0; CPid U0 = P0.aux(0);
  CPid P1 = P0.spawn(0); CPid U1 = P1.aux(0);
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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

  CPid P0; CPid U0 = P0.aux(0);
  CPid P1 = P0.spawn(0); CPid U1 = P1.aux(0);
  IID<CPid>
    ux0(U0,1), ry0(P0,4),
    uy1(U1,1), rx1(P1,3);
  DPORDriver_test::trace_set_spec expected =
    {{{ux0,rx1},{ry0,uy1}},
     {{ux0,rx1},{uy1,ry0}},
     {{rx1,ux0},{uy1,ry0}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Atomic_store_3){
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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

  CPid P0; CPid U0 = P0.aux(0);
  CPid P1 = P0.spawn(0); CPid U1 = P1.aux(0);
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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

  CPid P0; CPid U0 = P0.aux(0);
  CPid P1 = P0.spawn(0); CPid U1 = P1.aux(0);
  IID<CPid>
    ux0(U0,1), ry0(P0,4),
    uy1(U1,1), rx1(P1,3);
  DPORDriver_test::trace_set_spec expected =
    {{{ux0,rx1},{ry0,uy1}},
     {{ux0,rx1},{uy1,ry0}},
     {{rx1,ux0},{uy1,ry0}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(CMPXCHG_3){
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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

  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Join_1){
  /* Check propagation of arguments through pthread_create, and return
   * values through pthread_join.
   */
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  CPid P1 = P0.spawn(0); CPid U1 = P1.aux(0);
  IID<CPid>
    ux1(U1,1), uy1(U1,2), lock1(P1,2), unlock1(P1,4),
    ry0(P0,4), rx0(P0,6), lock0(P0,3), unlock0(P0,5);
  DPORDriver_test::trace_set_spec expected =
    {{{unlock0,lock1},{ry0,uy1},{rx0,ux1}},
     {{unlock0,lock1},{ry0,uy1},{ux1,rx0}},
     {{unlock1,lock0},{uy1,ry0},{ux1,rx0}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Mutex_4){
  /* Unlock without lock */
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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

  CPid P0; CPid U0 = P0.aux(0);
  CPid P1 = P0.spawn(0); CPid P2 = P0.spawn(1);
  IID<CPid>
    ux0(U0,1), rx0(P0,6), rx1(P1,2), rx2(P2,2);
  DPORDriver_test::trace_set_spec expected =
    {{{ux0,rx0}},
     {{ux0,rx1}},
     {{ux0,rx2}}};
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Mutex_7){
  /* Typical use of pthread_mutex_destroy */
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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

  CPid P0; CPid U0 = P0.aux(0);
  CPid P1 = P0.spawn(0);
  IID<CPid>
    lock0(P0,3), unlock0(P0,4), uf0(U0,1),
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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

BOOST_AUTO_TEST_CASE(Malloc_1){
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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

BOOST_AUTO_TEST_CASE(Assume_1){
  Configuration conf = DPORDriver_test::get_tso_conf();
  conf.malloc_may_fail = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
define i32 @main(){
  %p = call i8* @malloc(i32 10)
  %pi = ptrtoint i8* %p to i32
  call void @__VERIFIER_assume(i32 %pi)
  store i8 1, i8* %p, align 1
  call void @free(i8* %p)
  ret i32 0
}

declare i8* @malloc(i32) nounwind
declare void @free(i8*) nounwind
declare void @__VERIFIER_assume(i32)
)"),conf);
  DPORDriver::Result res = driver->run();
  delete driver;
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Assume_2){
  Configuration conf = DPORDriver_test::get_tso_conf();
  conf.malloc_may_fail = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
define i32 @main(){
  %p = call i8* @malloc(i32 10)
  %pi = ptrtoint i8* %p to i32
  call void @__VERIFIER_assume(i32 %pi)
  call void @free(i8* %p)
  call void @__assert_fail()
  ret i32 0
}

declare i8* @malloc(i32) nounwind
declare void @free(i8*) nounwind
declare void @__assert_fail()
declare void @__VERIFIER_assume(i32)
)"),conf);
  DPORDriver::Result res = driver->run();
  delete driver;
  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(Nasty_bitcast){
  /* Test the support for nastily calling pthread_create with a
   * function taking no arguments.
   */
  Configuration conf = DPORDriver_test::get_tso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
define i8* @p(){
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* bitcast(i8*()* @p to i8*(i8*)*), i8* null)
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);
  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Atomic_2){
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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

BOOST_AUTO_TEST_CASE(Non_fences){
  Configuration conf = DPORDriver_test::get_tso_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

declare void @__VERIFIER_assume(i32) nounwind

define i8* @p(i8* %arg){
  store i32 1, i32* @y, align 4
  call void @__VERIFIER_assume(i32 1)       ; Not a fence
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p, i8* null)
  store i32 1, i32* @x, align 4
  fence seq_cst
  load i32, i32* @y, align 4
  ret i32 0
}

%attr_t = type{i64, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  CPid P0; CPid U0 = P0.aux(0);
  CPid P1 = P0.spawn(0); CPid U1 = P1.aux(0);
  IID<CPid>
    ux0(U0,1), ry0(P0,4),
    uy1(U1,1), rx1(P1,3);
  DPORDriver_test::trace_set_spec expected =
    {{{ux0,rx1},{ry0,uy1}},
     {{ux0,rx1},{uy1,ry0}},
     {{rx1,ux0},{ry0,uy1}},
     {{rx1,ux0},{uy1,ry0}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf));
}

BOOST_AUTO_TEST_CASE(Bitcast_rowe_1){
  Configuration conf = DPORDriver_test::get_tso_conf();
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
  Configuration conf = DPORDriver_test::get_tso_conf();
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

BOOST_AUTO_TEST_CASE(Full_conflict_5){
  /* This test assumes that memcpy has full memory conflict. If that
   * is no longer the case, replace the call to memcpy with any
   * function that does have a full memory conflict.
   */
  Configuration conf = DPORDriver_test::get_tso_conf();
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

  CPid P0; CPid U0 = P0.aux(0);
  CPid P1 = P0.spawn(0); CPid U1 = P1.aux(0);
  IID<CPid> p1ux1(P1,1), p1ux2(U1,2), p1mc(P1,3),
    p0mc(P0,2), p0uy(U0,1);
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

BOOST_AUTO_TEST_CASE(Overlapping_ROWE_1){
  /* A load l of bytes B(l) where the latest preceding store which
   , l of bytes B(l) where the latest preceding store which
   * accesses some bytes in B(l) does not access precisely the bytes
   * in B(l), will block until that store has updated to memory. This
   * is intended to be a safe under-approximation of TSO behaviour.
   */
  Configuration conf = DPORDriver_test::get_tso_conf();
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
   * is intended to be a safe under-approximation of TSO behaviour.
   */
  Configuration conf = DPORDriver_test::get_tso_conf();
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
   * is intended to be a safe under-approximation of TSO behaviour.
   */
  Configuration conf = DPORDriver_test::get_tso_conf();
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
   * is intended to be a safe under-approximation of TSO behaviour.
   */
  Configuration conf = DPORDriver_test::get_tso_conf();
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
   * is intended to be a safe under-approximation of TSO behaviour.
   */
  Configuration conf = DPORDriver_test::get_tso_conf();
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

