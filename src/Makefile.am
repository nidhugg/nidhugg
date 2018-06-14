AUTOMAKE_OPTIONS = -Wno-override ## Make autotools quit complaining about explicit rule for swscc.

noinst_LIBRARIES = libswsc.a
libswsc_a_SOURCES = \
  AddLibPass.cpp AddLibPass.h \
  BVClock.cpp BVClock.h \
  CheckModule.cpp CheckModule.h \
  Configuration.cpp Configuration.h \
  CPid.cpp CPid.h \
  Debug.cpp Debug.h \
  DetCheckTraceBuilder.cpp DetCheckTraceBuilder.h \
  DPORDriver.cpp DPORDriver.h \
  Execution.cpp \
  ExternalFunctions.cpp \
  FBVClock.cpp FBVClock.h \
  GlobalContext.cpp GlobalContext.h \
  IID.h IID.tcc \
  Interpreter.cpp Interpreter.h \
  LoopUnrollPass.cpp LoopUnrollPass.h \
  MRef.cpp MRef.h \
  nregex.cpp nregex.h \
  Option.h \
  POWERExecution.cpp \
  POWERInterpreter.cpp POWERInterpreter.h \
  POWERARMTraceBuilder.cpp POWERARMTraceBuilder.tcc POWERARMTraceBuilder.h POWERARMTraceBuilder_decl.h \
  PSOInterpreter.cpp PSOInterpreter.h \
  PSOTraceBuilder.cpp PSOTraceBuilder.h \
  SaturatedGraph.h SaturatedGraph.cpp \
  SigSegvHandler.cpp SigSegvHandler.h \
  SatSolver.h \
  SExpr.h SExpr.cpp \
  SmtlibSatSolver.h SmtlibSatSolver.cpp \
  SpinAssumePass.cpp SpinAssumePass.h \
  StrModule.cpp StrModule.h \
  SymEv.cpp SymEv.h \
  SymAddr.cpp SymAddr.h \
  Trace.cpp Trace.h \
  TraceBuilder.cpp TraceBuilder.h \
  Transform.cpp Transform.h \
  TSOInterpreter.cpp TSOInterpreter.h \
  TSOPSOTraceBuilder.h \
  TSOTraceBuilder.cpp TSOTraceBuilder.h \
  WSCTraceBuilder.cpp WSCTraceBuilder.h \
  VClock.cpp VClock.h VClock.tcc \
  vecset.h vecset.tcc
libswsc_a_CXXFLAGS = -fno-rtti

EXTRA_PROGRAMS = swscc
bin_PROGRAMS = swsc @SWSCCBIN@
swsc_SOURCES = main.cpp
swsc_LDADD = @BOOST_SYSTEM_LIB@ libswsc.a
swsc_LDFLAGS = -pthread
swsc_CXXFLAGS = -fno-rtti
swscc_SOURCES = swscc.py

TESTS = unittest
check_PROGRAMS = unittest
unittest_SOURCES = \
  ARM_test.cpp \
  ARM_test2.cpp \
  CPid_test.cpp \
  DPORDriver_test.cpp DPORDriver_test.h \
  DryRun_test.cpp \
  FBVClock_test.cpp \
  nregex_test.cpp \
  Observers_test.cpp \
  POWER_test.cpp \
  POWER_test2.cpp \
  PSO_test.cpp \
  PSO_test2.cpp \
  RMW_test.cpp \
  Robustness_test.cpp \
  SC_test.cpp \
  SC_test2.cpp \
  TSO_test.cpp \
  TSO_test2.cpp \
  Unroll_test.cpp \
  VClock_CPid_test.cpp \
  VClock_int_test.cpp \
  unittest.cpp
unittest_LDADD=@BOOST_SYSTEM_LIB@ @BOOST_UNIT_TEST_FRAMEWORK_LIB@ libswsc.a
unittest_LDFLAGS=-pthread


## Run only the tests matching UTEST. Default: all.
## Override by a make command like
## $ make test UTEST='some filter'
UTEST ?=

.PHONY: test valtest litmus mustpass
BOOSTTESTFLAGS=--report_level=short --log_level=warning
test: unittest
	./unittest $(BOOSTTESTFLAGS) `test -z "$(UTEST)" || echo "--run_test=$(UTEST)"`

valtest: unittest
	valgrind --leak-check=full ./unittest $(BOOSTTESTFLAGS) --show_progress \
		`test -z "$(UTEST)" || echo "--run_test=$(UTEST)"`
litmus: 
	cd $(top_builddir)/tests/litmus && python3 ./test-swsc.py all

mustpass:
	cd $(top_builddir)/tests/smallbutchallenging && python3 ./test-swsc.py all

swscc$(EXEEXT): $(srcdir)/swscc.py
	cat $(srcdir)/swscc.py \
	| sed 's|%%PYTHON%%|@PYTHON@|g' \
	| sed 's|%%CLANG%%|@CLANG@|g' \
	| sed 's|%%CLANGXX%%|@CLANGXX@|g' \
	> swscc
	chmod a+x swscc
