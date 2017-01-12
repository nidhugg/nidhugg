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
#include "vecset.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(RMW_test)

/* Iterate through these memory models for all tests. */
static VecSet<Configuration::MemoryModel> MMs = {
  Configuration::SC,
  Configuration::TSO,
  Configuration::PSO
};

BOOST_AUTO_TEST_CASE(RMW_xchg){
  for(Configuration::MemoryModel MM : MMs){
    Configuration conf;
    conf.memory_model = MM;
    DPORDriver *driver =
      DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i32 @main(){
  %x0 = atomicrmw xchg i32* @x, i32 1 seq_cst
  %x1 = load i32, i32* @x, align 4
  %cmp0 = icmp ne i32 %x0, 0
  %cmp1 = icmp ne i32 %x1, 1
  %cmp0or1 = or i1 %cmp0, %cmp1
  br i1 %cmp0or1, label %error, label %L1

L1:
  br label %exit

error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i32 0
}

declare void @__assert_fail() nounwind
)"),conf);

    DPORDriver::Result res = driver->run();
    delete driver;

    BOOST_CHECK(!res.has_errors());
  }
}

BOOST_AUTO_TEST_CASE(RMW_add){
  for(Configuration::MemoryModel MM : MMs){
    Configuration conf;
    conf.memory_model = MM;
    DPORDriver *driver =
      DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 2, align 4

define i32 @main(){
  %x0 = atomicrmw add i32* @x, i32 1 seq_cst
  %x1 = load i32, i32* @x, align 4
  %cmp0 = icmp ne i32 %x0, 2
  %cmp1 = icmp ne i32 %x1, 3
  %cmp0or1 = or i1 %cmp0, %cmp1
  br i1 %cmp0or1, label %error, label %L1

L1:
  atomicrmw add i32* @x, i32 2147483647 seq_cst ; 2147483647 == 2^31-1
  %x2 = load i32, i32* @x, align 4
  %cmp2 = icmp ne i32 %x2, -2147483646 ; ~ 2147483650 == 2^31+2
  br i1 %cmp2, label %error, label %L2

L2:
  atomicrmw add i32* @x, i32 2147483647 seq_cst
  %x3 = load i32, i32* @x, align 4
  %cmp3 = icmp ne i32 %x3, 1 ; 2^31+2 + 2^31-1 == 2^32+1 ~ 1
  br i1 %cmp3, label %error, label %L3

L3:
  br label %exit

error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i32 0
}

declare void @__assert_fail() nounwind
)"),conf);

    DPORDriver::Result res = driver->run();
    delete driver;

    BOOST_CHECK(!res.has_errors());
  }
}

BOOST_AUTO_TEST_CASE(RMW_sub){
  for(Configuration::MemoryModel MM : MMs){
    Configuration conf;
    conf.memory_model = MM;
    DPORDriver *driver =
      DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 2, align 4

define i32 @main(){
  %x0 = atomicrmw sub i32* @x, i32 1 seq_cst
  %x1 = load i32, i32* @x, align 4
  %cmp0 = icmp ne i32 %x0, 2
  %cmp1 = icmp ne i32 %x1, 1
  %cmp0or1 = or i1 %cmp0, %cmp1
  br i1 %cmp0or1, label %error, label %L1

L1:
  br label %exit

error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i32 0
}

declare void @__assert_fail() nounwind
)"),conf);

    DPORDriver::Result res = driver->run();
    delete driver;

    BOOST_CHECK(!res.has_errors());
  }
}

BOOST_AUTO_TEST_CASE(RMW_and){
  for(Configuration::MemoryModel MM : MMs){
    Configuration conf;
    conf.memory_model = MM;
    DPORDriver *driver =
      DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 3840, align 4 ; 0xF00

define i32 @main(){
  %x0 = atomicrmw and i32* @x, i32 1311 seq_cst ; 0x51F
  %x1 = load i32, i32* @x, align 4
  %cmp0 = icmp ne i32 %x0, 3840
  %cmp1 = icmp ne i32 %x1, 1280 ; 0x500
  %cmp0or1 = or i1 %cmp0, %cmp1
  br i1 %cmp0or1, label %error, label %L1

L1:
  br label %exit

error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i32 0
}

declare void @__assert_fail() nounwind
)"),conf);

    DPORDriver::Result res = driver->run();
    delete driver;

    BOOST_CHECK(!res.has_errors());
  }
}

BOOST_AUTO_TEST_CASE(RMW_nand){
  for(Configuration::MemoryModel MM : MMs){
    Configuration conf;
    conf.memory_model = MM;
    DPORDriver *driver =
      DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 3840, align 4 ; 0xF00

define i32 @main(){
  %x0 = atomicrmw nand i32* @x, i32 1311 seq_cst ; 0x51F
  %x1 = load i32, i32* @x, align 4
  %cmp0 = icmp ne i32 %x0, 3840
  %cmp1 = icmp ne i32 %x1, 4294966015 ; 0xFFFFFAFF
  %cmp0or1 = or i1 %cmp0, %cmp1
  br i1 %cmp0or1, label %error, label %L1

L1:
  br label %exit

error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i32 0
}

declare void @__assert_fail() nounwind
)"),conf);

    DPORDriver::Result res = driver->run();
    delete driver;

    BOOST_CHECK(!res.has_errors());
  }
}

BOOST_AUTO_TEST_CASE(RMW_or){
  for(Configuration::MemoryModel MM : MMs){
    Configuration conf;
    conf.memory_model = MM;
    DPORDriver *driver =
      DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 3840, align 4 ; 0xF00

define i32 @main(){
  %x0 = atomicrmw or i32* @x, i32 1311 seq_cst ; 0x51F
  %x1 = load i32, i32* @x, align 4
  %cmp0 = icmp ne i32 %x0, 3840
  %cmp1 = icmp ne i32 %x1, 3871 ; 0xF1F
  %cmp0or1 = or i1 %cmp0, %cmp1
  br i1 %cmp0or1, label %error, label %L1

L1:
  br label %exit

error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i32 0
}

declare void @__assert_fail() nounwind
)"),conf);

    DPORDriver::Result res = driver->run();
    delete driver;

    BOOST_CHECK(!res.has_errors());
  }
}

BOOST_AUTO_TEST_CASE(RMW_xor){
  for(Configuration::MemoryModel MM : MMs){
    Configuration conf;
    conf.memory_model = MM;
    DPORDriver *driver =
      DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 3840, align 4 ; 0xF00

define i32 @main(){
  %x0 = atomicrmw xor i32* @x, i32 1311 seq_cst ; 0x51F
  %x1 = load i32, i32* @x, align 4
  %cmp0 = icmp ne i32 %x0, 3840
  %cmp1 = icmp ne i32 %x1, 2591 ; 0xA1F
  %cmp0or1 = or i1 %cmp0, %cmp1
  br i1 %cmp0or1, label %error, label %L1

L1:
  br label %exit

error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i32 0
}

declare void @__assert_fail() nounwind
)"),conf);

    DPORDriver::Result res = driver->run();
    delete driver;

    BOOST_CHECK(!res.has_errors());
  }
}

BOOST_AUTO_TEST_CASE(RMW_max){
  for(Configuration::MemoryModel MM : MMs){
    Configuration conf;
    conf.memory_model = MM;
    DPORDriver *driver =
      DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 -1, align 4

define i32 @main(){
  %x0 = atomicrmw max i32* @x, i32 4 seq_cst
  %x1 = load i32, i32* @x, align 4
  %cmp0 = icmp ne i32 %x0, -1
  %cmp1 = icmp ne i32 %x1, 4
  %cmp0or1 = or i1 %cmp0, %cmp1
  br i1 %cmp0or1, label %error, label %L1

L1:
  atomicrmw max i32* @x, i32 1 seq_cst
  %x2 = load i32, i32* @x, align 4
  %cmp2 = icmp ne i32 %x2, 4
  br i1 %cmp2, label %error, label %L2

L2:
  br label %exit

error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i32 0
}

declare void @__assert_fail() nounwind
)"),conf);

    DPORDriver::Result res = driver->run();
    delete driver;

    BOOST_CHECK(!res.has_errors());
  }
}

BOOST_AUTO_TEST_CASE(RMW_min){
  for(Configuration::MemoryModel MM : MMs){
    Configuration conf;
    conf.memory_model = MM;
    DPORDriver *driver =
      DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 2, align 4

define i32 @main(){
  %x0 = atomicrmw min i32* @x, i32 4 seq_cst
  %x1 = load i32, i32* @x, align 4
  %cmp0 = icmp ne i32 %x0, 2
  %cmp1 = icmp ne i32 %x1, 2
  %cmp0or1 = or i1 %cmp0, %cmp1
  br i1 %cmp0or1, label %error, label %L1

L1:
  atomicrmw min i32* @x, i32 -3 seq_cst
  %x2 = load i32, i32* @x, align 4
  %cmp2 = icmp ne i32 %x2, -3
  br i1 %cmp2, label %error, label %L2

L2:
  br label %exit

error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i32 0
}

declare void @__assert_fail() nounwind
)"),conf);

    DPORDriver::Result res = driver->run();
    delete driver;

    BOOST_CHECK(!res.has_errors());
  }
}

BOOST_AUTO_TEST_CASE(RMW_umax){
  for(Configuration::MemoryModel MM : MMs){
    Configuration conf;
    conf.memory_model = MM;
    DPORDriver *driver =
      DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 2, align 4

define i32 @main(){
  %x0 = atomicrmw umax i32* @x, i32 4 seq_cst
  %x1 = load i32, i32* @x, align 4
  %cmp0 = icmp ne i32 %x0, 2
  %cmp1 = icmp ne i32 %x1, 4
  %cmp0or1 = or i1 %cmp0, %cmp1
  br i1 %cmp0or1, label %error, label %L1

L1:
  atomicrmw umax i32* @x, i32 -2 seq_cst
  %x2 = load i32, i32* @x, align 4
  %cmp2 = icmp ne i32 %x2, -2
  br i1 %cmp2, label %error, label %L2

L2:
  br label %exit

error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i32 0
}

declare void @__assert_fail() nounwind
)"),conf);

    DPORDriver::Result res = driver->run();
    delete driver;

    BOOST_CHECK(!res.has_errors());
  }
}

BOOST_AUTO_TEST_SUITE_END()

#endif
