/* Copyright (C) 2020 Magnus Lång
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

#ifdef HAVE_VALGRIND_VALGRIND_H
#include <valgrind/valgrind.h>
#endif
#include "DPORDriver.h"
#include "DPORDriver_test.h"
#include "StrModule.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(Interpreter_test)

enum MM {
  SC,
  TSO,
  PSO,
  RFSC,
  POWER,
};

Configuration get_conf(MM mm) {
  switch(mm) {
  case SC:  return DPORDriver_test::get_sc_conf();
  case TSO: return DPORDriver_test::get_tso_conf();
  case PSO: return DPORDriver_test::get_pso_conf();
  case RFSC: {
    Configuration c = DPORDriver_test::get_sc_conf();
    c.dpor_algorithm = Configuration::READS_FROM;
    return c;
  }
  case POWER: return DPORDriver_test::get_power_conf();
  default: abort();
  }
}

void do_test_error(const char *module, const std::initializer_list<MM> mms) {
  for (MM mm : mms) {
    BOOST_TEST_CHECKPOINT( "mm=" << mm );
    Configuration conf = get_conf(mm);
    std::unique_ptr<DPORDriver> driver(DPORDriver::parseIR(module, conf));
    DPORDriver::Result res = driver->run();
    BOOST_CHECK(res.trace_count == 1);
    BOOST_CHECK(res.has_errors());
  }
}

#define TEST_ERROR(Name, IR) TEST_ERROR2(Name, IR, "")
#define TEST_ERROR2(Name, IR, IRAfter) TEST_ERROR3(Name, IR, IRAfter, SC)
#define TEST_ERROR3(Name, IR, IRAfter, ...)                         \
  BOOST_AUTO_TEST_CASE(Name){                                       \
    do_test_error("define i32 @main(){\n" IR                        \
                  "\n ret i32 0\n }\n" IRAfter, { __VA_ARGS__  });  \
  }

#define BADADDRT(T) T " inttoptr (i32 3735928559 to " T ")"
#define BADADDR BADADDRT("i32*")

TEST_ERROR3(Load_segv_badaddr, "load i32, " BADADDR, "", SC, TSO, PSO)
TEST_ERROR3(Store_segv_badaddr, "store i32 42, " BADADDR, "", SC, TSO, PSO)
TEST_ERROR(Cmpxhg_segv_badaddr, "cmpxchg " BADADDR ", i32 0, i32 42 seq_cst seq_cst")
TEST_ERROR(Atomicrmw_segv_badaddr, "atomicrmw add " BADADDR ", i32 42 seq_cst")
TEST_ERROR2(Pthread_create_tid_badaddr,
            "call i32 @pthread_create(" BADADDR
            ", i32* null, i8*(i8*)* @p1, i8* null)",
            R"(define i8* @p1(i8*){ ret i8* null }
               declare i32 @pthread_create(i32*, i32*, i8*(i8*)*, i8*))")
// TEST_ERROR2(Pthread_create_proc_badaddr,
//             "call i32 @pthread_create(i32* null, i32* null, "
//             BADADDRT("i8*(i8*)*") ", i8* null)",
//             R"(declare i32 @pthread_create(i32*, i32*, i8*(i8*)*, i8*))")
TEST_ERROR2(Pthread_join_ret_badaddr,
            R"(%tidp = alloca i8*
               call i32 @pthread_create(i8** %tidp, i32* null, i8*(i8*)* @p1, i8* null)
               %tid = load i8*, i8** %tidp
               call i32 @pthread_join(i8* %tid, )" BADADDRT("i8**") ")",
            R"(define i8* @p1(i8*){ ret i8* null }
               declare i32 @pthread_create(i8**, i32*, i8*(i8*)*, i8*)
               declare i32 @pthread_join(i8*, i8**))")
TEST_ERROR2(Pthread_mutex_init_mtx_badaddr,
            "call i32 @pthread_mutex_init(" BADADDR ", i32* null)",
            "declare i32 @pthread_mutex_init(i32*, i32*)")
TEST_ERROR2(Pthread_mutex_lock_badaddr,
            "call i32 @pthread_mutex_lock(" BADADDR ")",
            "declare i32 @pthread_mutex_lock(i32*)")
TEST_ERROR2(Pthread_mutex_trylock_badaddr,
            "call i32 @pthread_mutex_trylock(" BADADDR ")",
            "declare i32 @pthread_mutex_trylock(i32*)")
TEST_ERROR2(Pthread_mutex_unlock_badaddr,
            "call i32 @pthread_mutex_unlock(" BADADDR ")",
            "declare i32 @pthread_mutex_unlock(i32*)")
TEST_ERROR2(Pthread_mutex_destroy_badaddr,
            "call i32 @pthread_mutex_destroy(" BADADDR ")",
            "declare i32 @pthread_mutex_destroy(i32*)")
TEST_ERROR2(Pthread_cond_init_cnd_badaddr,
            "call i32 @pthread_cond_init(" BADADDR ", i32* null)",
            "declare i32 @pthread_cond_init(i32*, i32*)")
TEST_ERROR2(Pthread_cond_wait_cnd_badaddr,
            R"(call i32 @pthread_mutex_init(i32* @lck, i32* null)
               call i32 @pthread_cond_wait()" BADADDR ", i32* @lck)",
            R"(@lck = global i32 0, align 4
               declare i32 @pthread_mutex_init(i32*, i32*)
               declare i32 @pthread_cond_wait(i32*, i32*))")
TEST_ERROR2(Pthread_cond_wait_mtx_badaddr,
            R"(call i32 @pthread_cond_init(i32* @cnd, i32* null)
               call i32 @pthread_cond_wait(i32* @cnd, )" BADADDR ")",
            R"(@cnd = global i32 0, align 4
               declare i32 @pthread_cond_init(i32*, i32*)
               declare i32 @pthread_cond_wait(i32*, i32*))")
TEST_ERROR2(Pthread_cond_broadcast_badaddr,
            "call i32 @pthread_cond_broadcast(" BADADDR ")",
            "declare i32 @pthread_cond_broadcast(i32*)")
TEST_ERROR2(Pthread_cond_destroy_badaddr,
            "call i32 @pthread_cond_destroy(" BADADDR ")",
            "declare i32 @pthread_cond_destroy(i32*)")

BOOST_AUTO_TEST_CASE(Global_ctor_test){
  for (MM mm : { SC, TSO, PSO, RFSC, POWER }) {
    BOOST_TEST_CHECKPOINT( "mm=" << mm );
    Configuration conf = get_conf(mm);
    std::unique_ptr<DPORDriver> driver(DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
%0 = type { i32, void ()*, i8* }
@llvm.global_ctors = appending global [1 x %0] [%0 { i32 65535, void ()* @ctor, i8* null }]

define internal void @ctor() {
  store i32 0, i32* @x, align 4
  ret void
}

define i32 @main() {
  call void @__assert_fail()
  unreachable
}

declare void @__assert_fail()
)"),conf));

    DPORDriver::Result res = driver->run();

    BOOST_CHECK(res.has_errors());
    BOOST_CHECK(res.trace_count == 1);
  }
}

BOOST_AUTO_TEST_CASE(Global_dtor_test){
  for (MM mm : { SC, TSO, PSO, RFSC, POWER }) {
    BOOST_TEST_CHECKPOINT( "mm=" << mm );
    Configuration conf = get_conf(mm);
    std::unique_ptr<DPORDriver> driver(DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
%0 = type { i32, void ()*, i8* }
@llvm.global_dtors = appending global [1 x %0] [%0 { i32 65535, void ()* @dtor, i8* null }]

define i32 @main() {
  store i32 0, i32* @x, align 4
  ret i32 0
}

define internal void @dtor() {
  call void @__assert_fail()
  unreachable
}

declare void @__assert_fail()
)"),conf));

    DPORDriver::Result res = driver->run();

    BOOST_CHECK(res.has_errors());
    BOOST_CHECK(res.trace_count == 1);
  }
}

BOOST_AUTO_TEST_CASE(Global_ctor_block_no_main_dtor){
  for (MM mm : { SC, TSO, PSO, RFSC, POWER }) {
    BOOST_TEST_CHECKPOINT( "mm=" << mm );
    Configuration conf = get_conf(mm);
    std::unique_ptr<DPORDriver> driver(DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
%0 = type { i32, void ()*, i8* }
@llvm.global_ctors = appending global [1 x %0] [%0 { i32 65535, void ()* @ctor, i8* null }]
@llvm.global_dtors = appending global [1 x %0] [%0 { i32 65535, void ()* @dtor, i8* null }]

define internal void @ctor() {
  call void @__VERIFIER_assume(i1 false)
  ret void
}

define i32 @main() {
  call void @__assert_fail()
  unreachable
}

define internal void @dtor() {
  call void @__assert_fail()
  unreachable
}

declare void @__VERIFIER_assume(i1)
declare void @__assert_fail()
)"),conf));

    DPORDriver::Result res = driver->run();

    BOOST_CHECK(!res.has_errors());
    BOOST_CHECK(res.trace_count == 0);
    BOOST_CHECK(res.assume_blocked_trace_count == 1);
  }
}

BOOST_AUTO_TEST_CASE(Main_block_no_global_dtor){
  for (MM mm : { SC, TSO, PSO, RFSC, POWER }) {
    BOOST_TEST_CHECKPOINT( "mm=" << mm );
    Configuration conf = get_conf(mm);
    std::unique_ptr<DPORDriver> driver(DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
%0 = type { i32, void ()*, i8* }
@llvm.global_dtors = appending global [1 x %0] [%0 { i32 65535, void ()* @dtor, i8* null }]

define i32 @main() {
  call void @__VERIFIER_assume(i1 false)
  ret i32 0
}

define internal void @dtor() {
  call void @__assert_fail()
  unreachable
}

declare void @__VERIFIER_assume(i1)
declare void @__assert_fail()
)"),conf));

    DPORDriver::Result res = driver->run();

    BOOST_CHECK(!res.has_errors());
    BOOST_CHECK(res.trace_count == 0);
    BOOST_CHECK(res.assume_blocked_trace_count == 1);
  }
}

BOOST_AUTO_TEST_SUITE_END()

#endif
