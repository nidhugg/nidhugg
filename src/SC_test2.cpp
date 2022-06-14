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

#ifdef HAVE_VALGRIND_VALGRIND_H
#include <valgrind/valgrind.h>
#endif
#include "DPORDriver.h"
#include "DPORDriver_test.h"
#include "StrModule.h"

#include <boost/test/unit_test.hpp>
#include <cstdint>

BOOST_AUTO_TEST_SUITE(SC_test)

BOOST_AUTO_TEST_CASE(Thread_local_1){
  Configuration conf = DPORDriver_test::get_sc_conf();
  std::string module = R"(
@x = thread_local global i32 0, align 4

define i8* @w(i8* %arg){
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @w, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @w, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @w, i8* null)
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)";

  DPORDriver *driver = DPORDriver::parseIR(module, conf);
  DPORDriver::Result res = driver->run();
  delete driver;

  conf.dpor_algorithm = Configuration::OPTIMAL;
  driver = DPORDriver::parseIR(module, conf);
  DPORDriver::Result opt_res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 1); // No conflicts on @x
  BOOST_CHECK(DPORDriver_test::check_optimal_equiv(res, opt_res, conf));
}

BOOST_AUTO_TEST_CASE(Thread_local_2){
  Configuration conf = DPORDriver_test::get_sc_conf();
  std::string module = StrModule::portasm(R"(
@x = thread_local global i32 0, align 4
@p = global i32* null, align 8

define i8* @w(i8* %arg){
  store i32* @x, i32** @p, align 8
  store i32 1, i32* @x, align 4
  %p = load i32*, i32** @p, align 8
  load i32, i32* %p, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @w, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @w, i8* null)
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)");

  DPORDriver *driver = DPORDriver::parseIR(module, conf);
  DPORDriver::Result res = driver->run();
  delete driver;

  conf.dpor_algorithm = Configuration::OPTIMAL;
  driver = DPORDriver::parseIR(module, conf);
  DPORDriver::Result opt_res = driver->run();
  delete driver;

  CPid P0, P1 = P0.spawn(0), P2 = P0.spawn(1);
  IID<CPid> wp1(P1,1), wx1(P1,2), rp1(P1,3), rpx1(P1,4),
    wp2(P2,1), wx2(P2,2), rp2(P2,3), rpx2(P2,4);
  DPORDriver_test::trace_set_spec expected =
    {{{wp1,wp2},{rp1,wp2}},
     {{wp1,wp2},{wp2,rp1},{rpx1,wx2}},
     {{wp1,wp2},{wp2,rp1},{wx2,rpx1}},
     {{wp2,wp1},{rp2,wp1}},
     {{wp2,wp1},{wp1,rp2},{rpx2,wx1}},
     {{wp2,wp1},{wp1,rp2},{wx1,rpx2}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf,&opt_res));
}

BOOST_AUTO_TEST_CASE(Thread_local_3){ // Initialization
  Configuration conf = DPORDriver_test::get_sc_conf();
  std::string module = StrModule::portasm(R"(
@x = thread_local global i32 666, align 4

define i8* @w(i8* %arg){
  %x = load i32, i32* @x, align 4
  %c = icmp eq i32 %x, 666
  br i1 %c, label %exit, label %error
error:
  call void @__assert_fail()
  br label %exit
exit:
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @w, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @w, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @w, i8* null)
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare void @__assert_fail()
)");

  DPORDriver *driver = DPORDriver::parseIR(module, conf);
  DPORDriver::Result res = driver->run();
  delete driver;

  conf.dpor_algorithm = Configuration::OPTIMAL;
  driver = DPORDriver::parseIR(module, conf);
  DPORDriver::Result opt_res = driver->run();
  delete driver;

  BOOST_CHECK(!res.has_errors());
  BOOST_CHECK(DPORDriver_test::check_optimal_equiv(res, opt_res, conf));
}

BOOST_AUTO_TEST_CASE(CAS_load){
  /* Check that a failing CAS does not have a conflict with loads. */
  Configuration conf = DPORDriver_test::get_sc_conf();
  std::string module = StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @r(i8* %arg){
  load i32, i32* @x, align 4
  ret i8* null
}

define i8* @w(i8* %arg){
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @r, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @w, i8* null)
)"
#if defined(LLVM_CMPXCHG_SEPARATE_SUCCESS_FAILURE_ORDERING)
R"(  cmpxchg i32* @x, i32 1, i32 2 seq_cst seq_cst)"
#else
R"(  cmpxchg i32* @x, i32 1, i32 2 seq_cst)"
#endif
R"(
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)");

  DPORDriver *driver = DPORDriver::parseIR(module, conf);
  DPORDriver::Result res = driver->run();
  delete driver;

  conf.dpor_algorithm = Configuration::OPTIMAL;
  driver = DPORDriver::parseIR(module, conf);
  DPORDriver::Result opt_res = driver->run();
  delete driver;

  CPid P0, P1 = P0.spawn(0), P2 = P0.spawn(1);
  IID<CPid> cas(P0,3), r(P1,1), w(P2,1);
  DPORDriver_test::trace_set_spec expected =
    {{{cas,w},{r,w}},
     {{cas,w},{w,r}},
     {{w,cas},{r,w}},
     {{w,cas},{w,r},{r,cas}},
     {{w,cas},{cas,r}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf,&opt_res));
}

BOOST_AUTO_TEST_CASE(CAS_tricky){
  /* Check that we can predict that a CAS will fail when reversed with a
   * store. */
  Configuration conf = DPORDriver_test::get_sc_conf();
  std::string module = StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @r(i8* %arg){
  load i32, i32* @x, align 4
  ret i8* null
}

define i8* @cas(i8* %arg){
)"
#if defined(LLVM_CMPXCHG_SEPARATE_SUCCESS_FAILURE_ORDERING)
R"(  cmpxchg i32* @x, i32 1, i32 2 seq_cst seq_cst)"
#else
R"(  cmpxchg i32* @x, i32 1, i32 2 seq_cst)"
#endif
R"(
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @r, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @cas, i8* null)
  store i32 1, i32* @x, align 4
  ret i32 0
}

%attr_t = type {i64*, [48 x i8]}
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)");

  DPORDriver *driver = DPORDriver::parseIR(module, conf);
  DPORDriver::Result res = driver->run();
  delete driver;

  conf.dpor_algorithm = Configuration::OPTIMAL;
  driver = DPORDriver::parseIR(module, conf);
  DPORDriver::Result opt_res = driver->run();
  delete driver;

  CPid P0, P1 = P0.spawn(0), P2 = P0.spawn(1);
  IID<CPid> w(P0,3), r(P1,1), cas(P2,1);
  DPORDriver_test::trace_set_spec expected =
    {{{cas,w},{r,w}},
     {{cas,w},{w,r}},
     {{w,cas},{r,w}},
     {{w,cas},{w,r},{r,cas}},
     {{w,cas},{cas,r}}
    };
  BOOST_CHECK(DPORDriver_test::check_all_traces(res,expected,conf,&opt_res));
}

BOOST_AUTO_TEST_CASE(Compiler_fence_dekker){
  Configuration conf = DPORDriver_test::get_sc_conf();
  std::string module = StrModule::portasm(R"(
@x = global i32 0, align 4
@y = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 1, i32* @y, align 4
  call void asm sideeffect "", "~{memory}"()
  load i32, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  store i32 1, i32* @x, align 4
  call void asm sideeffect "", "~{memory}"()
  load i32, i32* @y, align 4
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)");

  DPORDriver *driver = DPORDriver::parseIR(module, conf);
  DPORDriver::Result res = driver->run();
  delete driver;

  conf.dpor_algorithm = Configuration::OPTIMAL;
  driver = DPORDriver::parseIR(module, conf);
  DPORDriver::Result opt_res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 3);
  BOOST_CHECK(DPORDriver_test::check_optimal_equiv(res, opt_res, conf));
}

BOOST_AUTO_TEST_CASE(Intrinsic_return){
  /* Regression test.
   *
   * Nidhugg would segmentation fault when analyzing this
   * sample. (Similarly under TSO, PSO, POWER and ARM.)
   *
   * The bug was caused by Execution not handling llvm.dbg.declare
   * correctly: When a thread first visits a llvm.dbg.declare
   * statement, the statement is removed from its basic
   * block. Therefore if any other thread's program counter (the
   * iterator ExecutionContext::CurInst) points into that basic block,
   * the iterator will become invalid. For this reason, the thread
   * that removes the llvm.dbg.declare needs to check all other
   * program counters and restore them if necessary. This was already
   * properly done for the program pointer of the top-most stack frame
   * of each other thread. However, program pointers in lower stack
   * frames were erroneously neglected and left in an invalid state.
   *
   * This test is designed to trigger that error. Both threads P0 and
   * P1 enter function f. From f they enter function g. Then one of
   * the threads is delayed inside g (a schedule which delays the
   * thread is triggered by having races in g). The other thread exits
   * g and visits the llvm.dbg.declare statement which immediately
   * follows the call to g from f. This will invalidate the return
   * address in the second to top stack frame for the other thread,
   * since it is pointing to that very statement.
   *
   * A complication is that in order to trigger the error, the above
   * described situation must happen in the first run after the test
   * module has been reparsed by DPORDriver. (Otherwise the
   * llvm.dbg.declare statements are no longer present in the module.)
   * Reparse happens (at the time of writing) every 1000 runs. This
   * test has 7 stores to x in g, which means that the situation is
   * caused to appear in (7*2)!/(7!*7!) = 3432 consecutive runs (save
   * the first and last run).
   */
  Configuration conf = DPORDriver_test::get_sc_conf();

#ifdef LLVM_METADATA_IS_VALUE
  std::string declarecall = "call void @llvm.dbg.declare(metadata !{i32 %id}, metadata !0)";
  std::string declaredeclare = R"(
declare void @llvm.dbg.declare(metadata, metadata) nounwind readnone
!llvm.module.flags = !{!1}
!0 = metadata !{i32 0}
!1 = metadata !{i32 2, metadata !"Debug Info Version", i32 )" LLVM_METADATA_VERSION_NUMBER_STR R"(}
)";
#else
#ifdef LLVM_DBG_DECLARE_TWO_ARGS
  std::string declarecall = "call void @llvm.dbg.declare(metadata i32 %id, metadata !0)";
  std::string declaredeclare = R"(
declare void @llvm.dbg.declare(metadata, metadata) nounwind readnone
!llvm.module.flags = !{!1}
!0 = !{i32 0}
!1 = !{i32 2, !"Debug Info Version", i32 )" LLVM_METADATA_VERSION_NUMBER_STR R"(}
)";
#else
  std::string declarecall = "call void @llvm.dbg.declare(metadata i32 %id, metadata !0, metadata !0)";
  std::string declaredeclare = R"(
declare void @llvm.dbg.declare(metadata, metadata, metadata) nounwind readnone
!llvm.module.flags = !{!1}
!0 = !{i32 0}
!1 = !{i32 2, !"Debug Info Version", i32 )" LLVM_METADATA_VERSION_NUMBER_STR R"(}
)";
#endif
#endif

  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define void @g(i32 %id){
  store i32 1, i32* @x, align 4
  store i32 1, i32* @x, align 4
  store i32 1, i32* @x, align 4
  store i32 1, i32* @x, align 4
  store i32 1, i32* @x, align 4
  store i32 1, i32* @x, align 4
  store i32 1, i32* @x, align 4
  ret void
}

define void @f(i32 %id){
  call void @g(i32 %id)
  )"+declarecall+R"(
  ret void
}

define i8* @p1(i8* %arg){
  call void @f(i32 1)
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  call void @f(i32 0)
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
)"+declaredeclare),conf);

  DPORDriver::Result res = driver->run();

  // No test. We only check for crashes.

  delete driver;
}

BOOST_AUTO_TEST_CASE(Nondeterminism_detection){
  /* Simulate thread-wise nondeterminism, and check that it is
   * detected.
   *
   * Here we use a memory location _changing_ which is declared
   * *outside* of the test code. This is in order to ensure that its
   * value is not reinitialized when the test is restarted (as a
   * static variable inside the test code would be).
   *
   * The code in main will run twice. (This is ensured by the race on
   * x.)
   *
   * The code in main branches on the value of _changing_, and changes
   * its value. Therefore the branch will go in different directions
   * in the first and the second computation. Since the branch is
   * evaluated in the portion of the code that is replayed, this
   * apparent non-determinism should be detected and trigger an error.
   *
   * All accesses to _changing_ are done through memcpy() in order to
   * mask the fact that it is not a valid memory location, so that this
   * test will exercise the nondeterminism detection, and not other
   * mechanisms that detect invalid input.
   */
  Configuration conf = DPORDriver_test::get_sc_conf();
  char changing = 0;
  std::stringstream ss;
  ss << "inttoptr(i64 " << (uintptr_t)(&changing) << " to i8*)";
  std::string changingAddr = ss.str();
  std::string module = StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p1(i8* %arg){
  store i32 1, i32* @x
  ret i8* null
}

define i32 @main(){
  %tp = alloca i8
  call i8* @memcpy(i8* %tp, i8* )"+changingAddr+R"(, i64 1)
  %v = load i8, i8* %tp
  %c = icmp eq i8 %v, 0
  br i1 %c, label %zero, label %nonzero
zero:
  store i8 1, i8* %tp
  call i8* @memcpy(i8* )"+changingAddr+R"(, i8* %tp, i64 1)
  br label %nonzero
nonzero:
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  load i32, i32* @x
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare void @__assert_fail()
declare i8* @memcpy(i8*, i8*, i64)
)");

  DPORDriver *driver = DPORDriver::parseIR(module, conf);
  DPORDriver::Result res = driver->run();

  BOOST_CHECK(changing == 1);
  BOOST_CHECK(res.has_errors());
  delete driver;

  changing = 0;
  conf.dpor_algorithm = Configuration::OPTIMAL;
  driver = DPORDriver::parseIR(module,conf);
  DPORDriver::Result opt_res = driver->run();
  delete driver;

  BOOST_CHECK(changing == 1);
  BOOST_CHECK(opt_res.has_errors());
}

BOOST_AUTO_TEST_CASE(lastzero_4){
  Configuration conf = DPORDriver_test::get_sc_conf();
  std::string module = StrModule::portasm(R"(
%arr_t = type [5 x i32]
@array = global %arr_t zeroinitializer, align 16

define i8* @reader(i8*) {
  br label %2

; <label>:2:                                      ; preds = %2, %1
  %3 = phi i64 [ %7, %2 ], [ 4, %1 ]
  %4 = getelementptr inbounds %arr_t, %arr_t* @array, i64 0, i64 %3
  %5 = load i32, i32* %4, align 4
  %6 = icmp eq i32 %5, 0
  %7 = add i64 %3, -1
  br i1 %6, label %8, label %2

; <label>:8:                                      ; preds = %2
  ret i8* null
}

define i8* @writer(i8* %arg) {
  %j = ptrtoint i8* %arg to i64
  %jm1 = add i64 %j, -1
  %p1 = getelementptr inbounds %arr_t, %arr_t* @array, i64 0, i64 %jm1
  %t1 = load i32, i32* %p1, align 4
  %t2 = add nsw i32 %t1, 1
  %p2 = getelementptr inbounds %arr_t, %arr_t* @array, i64 0, i64 %j
  store i32 %t2, i32* %p2, align 4
  ret i8* null
}

define i32 @main() {
  call i32 @pthread_create(i64* null, %attr_t* null, i8* (i8*)* @reader, i8* null)
  call i32 @pthread_create(i64* null, %attr_t* null, i8* (i8*)* @writer, i8* inttoptr (i64 1 to i8*))
  call i32 @pthread_create(i64* null, %attr_t* null, i8* (i8*)* @writer, i8* inttoptr (i64 2 to i8*))
  call i32 @pthread_create(i64* null, %attr_t* null, i8* (i8*)* @writer, i8* inttoptr (i64 3 to i8*))
  call i32 @pthread_create(i64* null, %attr_t* null, i8* (i8*)* @writer, i8* inttoptr (i64 4 to i8*))
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8* (i8*)*, i8*) nounwind
)");

  DPORDriver *driver = DPORDriver::parseIR(module, conf);
  DPORDriver::Result res = driver->run();
  delete driver;

  conf.dpor_algorithm = Configuration::OPTIMAL;
  driver = DPORDriver::parseIR(module, conf);
  DPORDriver::Result opt_res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 28);
  BOOST_CHECK(!res.has_errors());
  BOOST_CHECK(DPORDriver_test::check_optimal_equiv(res, opt_res, conf));
}

BOOST_AUTO_TEST_CASE(Pthread_t_pointer_1){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p(i8* %arg){
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  %ptp = alloca i8*
  call i32 @pthread_create(i8** %ptp, %attr_t* null, i8*(i8*)* @p, i8* null)
  %pt = load i8*, i8** %ptp
  call i32 @pthread_join(i8* %pt, i8** null)
  %xv = load i32, i32* @x, align 4
  ret i32 %xv
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i8**,%attr_t*,i8*(i8*)*,i8*)
declare i32 @pthread_join(i8*,i8**)
declare void @__assert_fail()
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 1);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Pthread_t_pointer_2){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4

define i8* @p(i8* %parent){
  call i32 @pthread_join(i8* %parent, i8** null)
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  %self = call i8* @pthread_self()
  call i32 @pthread_create(i8** null, %attr_t* null, i8*(i8*)* @p, i8* %self)
  %xv = load i32, i32* @x, align 4
  ret i32 %xv
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i8**,%attr_t*,i8*(i8*)*,i8*)
declare i32 @pthread_join(i8*,i8**)
declare i8* @pthread_self()
declare void @__assert_fail()
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 1);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(pthread_join_invoke){
  Configuration conf = DPORDriver_test::get_sc_conf();
  DPORDriver *driver =
    DPORDriver::parseIR(StrModule::portasm(R"(
@x = global i32 0, align 4
@t = global i8* null, align 4

define i8* @p(i8*){
  store i32 1, i32* @x, align 4
  ret i8* null
}

define i32 @main(){
  call i32 @pthread_create(i8** @t, %attr_t* null, i8*(i8*)* @p, i8* null)
  %tid = load i8*, i8** @t, align 4
  invoke i32 @pthread_join(i8* %tid, i8** null) to label %cont unwind label %catch

cont:
  %xv = load i32, i32* @x, align 4
  %comp = icmp eq i32 %xv, 1
  br i1 %comp, label %ok, label %err

ok:
  ret i32 0

err:
  call void @__assert_fail()
  unreachable

catch:
  landingpad i8 cleanup
  call void @__assert_fail()
  unreachable
}

%attr_t = type {i64, [48 x i8]}
declare i32 @pthread_create(i8**,%attr_t*,i8*(i8*)*,i8*)
declare i32 @pthread_join(i8*,i8**)
declare void @__assert_fail()
)"),conf);

  DPORDriver::Result res = driver->run();
  delete driver;

  BOOST_CHECK(res.trace_count == 1);
  BOOST_CHECK(!res.has_errors());
}

BOOST_AUTO_TEST_CASE(Nondeterminism_without_branch){
  /* Mechanism is the same as in Nonteterminsim_detection (cf for
   * detailed description of the test)
   * Here, instead having a branch that changes direction, we have a
   * load that changes address
   */
  Configuration conf = DPORDriver_test::get_sc_conf();
  char changing = 0;
  std::stringstream ss;
  ss << "inttoptr(i64 " << (uintptr_t)(&changing) << " to i8*)";
  std::string changingAddr = ss.str();
  std::string module = StrModule::portasm(R"(
@x = global i32 0, align 4
@arr = constant [2 x i32] [i32 42, i32 999]

define i8* @p1(i8* %arg){
  store i32 1, i32* @x
  ret i8* null
}

define i32 @main(){
  %tp = alloca i8
  call i8* @memcpy(i8* %tp, i8* )"+changingAddr+R"(, i64 1)
  %v = load i8, i8* %tp
  %v64 = zext i8 %v to i64
  %p = getelementptr [2 x i32], [2 x i32]* @arr, i64 0, i64 %v64
  %xxx = load i32, i32* %p
  store i8 1, i8* %tp
  call i8* @memcpy(i8* )"+changingAddr+R"(, i8* %tp, i64 1)
  call i32 @pthread_create(i64* null, %attr_t* null, i8*(i8*)* @p1, i8* null)
  load i32, i32* @x
  ret i32 0
}

%attr_t = type { i64, [48 x i8] }
declare i32 @pthread_create(i64*, %attr_t*, i8*(i8*)*, i8*) nounwind
declare void @__assert_fail()
declare i8* @memcpy(i8*, i8*, i64)
)");

  DPORDriver *driver = DPORDriver::parseIR(module, conf);
  DPORDriver::Result res = driver->run();

  BOOST_CHECK(changing == 1);
  BOOST_CHECK(res.has_errors());
  delete driver;

  changing = 0;
  conf.dpor_algorithm = Configuration::OPTIMAL;
  driver = DPORDriver::parseIR(module,conf);
  DPORDriver::Result opt_res = driver->run();
  delete driver;

  BOOST_CHECK(changing == 1);
  BOOST_CHECK(opt_res.has_errors());
}

BOOST_AUTO_TEST_SUITE_END()

#endif
