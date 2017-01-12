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

BOOST_AUTO_TEST_SUITE(Robustness_test)

BOOST_AUTO_TEST_CASE(Robustness_small_tso_1){
  Configuration conf = DPORDriver_test::get_tso_conf();
  conf.check_robustness = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 1, i32* @y, align 4
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null,%attr_t* null, i8*(i8*)*@p1, i8* null)
  store i32 1, i32* @x, align 4
  load i32, i32* @y, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(Robustness_small_tso_2){
  Configuration conf = DPORDriver_test::get_tso_conf();
  conf.check_robustness = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null,%attr_t* null, i8*(i8*)*@p1, i8* null)
  store i32 1, i32* @x, align 4
  load i32, i32* @y, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Robustness_small_tso_3){
  Configuration conf = DPORDriver_test::get_tso_conf();
  conf.check_robustness = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 1, i32* @y, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null,%attr_t* null, i8*(i8*)*@p1, i8* null)
  store i32 1, i32* @x, align 4
  load i32, i32* @y, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Robustness_small_tso_4){
  Configuration conf = DPORDriver_test::get_tso_conf();
  conf.check_robustness = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 1, i32* @y, align 4
  store i32 1, i32* @y, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null,%attr_t* null, i8*(i8*)*@p1, i8* null)
  store i32 1, i32* @x, align 4
  load i32, i32* @y, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Robustness_small_tso_5){
  Configuration conf = DPORDriver_test::get_tso_conf();
  conf.check_robustness = true;
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
  call i32 @pthread_create(i64* null,%attr_t* null, i8*(i8*)*@p1, i8* null)
  store i32 1, i32* @x, align 4
  load i32, i32* @y, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(Robustness_small_tso_6){
  Configuration conf = DPORDriver_test::get_tso_conf();
  conf.check_robustness = true;
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
  call i32 @pthread_create(i64* null,%attr_t* null, i8*(i8*)*@p1, i8* null)
  store i32 1, i32* @x, align 4
  load i32, i32* @y, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Robustness_small_tso_7){
  Configuration conf = DPORDriver_test::get_tso_conf();
  conf.check_robustness = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  add i32 0, 0
  add i32 0, 0
  store i32 1, i32* @y, align 4
  add i32 0, 0
  add i32 0, 0
  load i32, i32* @x, align 4
  add i32 0, 0
  add i32 0, 0
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null,%attr_t* null, i8*(i8*)*@p1, i8* null)
  add i32 0, 0
  add i32 0, 0
  store i32 1, i32* @x, align 4
  add i32 0, 0
  add i32 0, 0
  load i32, i32* @y, align 4
  add i32 0, 0
  add i32 0, 0
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(Robustness_small_tso_8){
  Configuration conf = DPORDriver_test::get_tso_conf();
  conf.check_robustness = true;
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
  call i32 @pthread_create(i64* null,%attr_t* null, i8*(i8*)*@p1, i8* null)
  store i32 1, i32* @x, align 4
  store i32 1, i32* @y, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Robustness_small_pso_1){
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.check_robustness = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 1, i32* @y, align 4
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null,%attr_t* null, i8*(i8*)*@p1, i8* null)
  store i32 1, i32* @x, align 4
  load i32, i32* @y, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(Robustness_small_pso_2){
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.check_robustness = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null,%attr_t* null, i8*(i8*)*@p1, i8* null)
  store i32 1, i32* @x, align 4
  load i32, i32* @y, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Robustness_small_pso_3){
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.check_robustness = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 1, i32* @y, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null,%attr_t* null, i8*(i8*)*@p1, i8* null)
  store i32 1, i32* @x, align 4
  load i32, i32* @y, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Robustness_small_pso_4){
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.check_robustness = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 1, i32* @y, align 4
  store i32 1, i32* @y, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null,%attr_t* null, i8*(i8*)*@p1, i8* null)
  store i32 1, i32* @x, align 4
  load i32, i32* @y, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Robustness_small_pso_5){
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.check_robustness = true;
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
  call i32 @pthread_create(i64* null,%attr_t* null, i8*(i8*)*@p1, i8* null)
  store i32 1, i32* @x, align 4
  load i32, i32* @y, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(Robustness_small_pso_6){
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.check_robustness = true;
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
  call i32 @pthread_create(i64* null,%attr_t* null, i8*(i8*)*@p1, i8* null)
  store i32 1, i32* @x, align 4
  load i32, i32* @y, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Robustness_small_pso_7){
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.check_robustness = true;
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  add i32 0, 0
  add i32 0, 0
  store i32 1, i32* @y, align 4
  add i32 0, 0
  add i32 0, 0
  load i32, i32* @x, align 4
  add i32 0, 0
  add i32 0, 0
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null,%attr_t* null, i8*(i8*)*@p1, i8* null)
  add i32 0, 0
  add i32 0, 0
  store i32 1, i32* @x, align 4
  add i32 0, 0
  add i32 0, 0
  load i32, i32* @y, align 4
  add i32 0, 0
  add i32 0, 0
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_CASE(Robustness_small_pso_8){
  Configuration conf = DPORDriver_test::get_pso_conf();
  conf.check_robustness = true;
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
  call i32 @pthread_create(i64* null,%attr_t* null, i8*(i8*)*@p1, i8* null)
  store i32 1, i32* @x, align 4
  store i32 1, i32* @y, align 4
  ret i32 0
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i64*,%attr_t*,i8*(i8*)*,i8*) nounwind
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.has_errors());
}

BOOST_AUTO_TEST_SUITE_END()

#endif
