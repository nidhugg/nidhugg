AUTOMAKE_OPTIONS = -Wno-override ## Make autotools quit complaining about explicit rule for nidhuggc.

noinst_LIBRARIES = libnidhugg.a
libnidhugg_a_SOURCES = \
  AddLibPass.cpp AddLibPass.h \
  BVClock.cpp BVClock.h \
  CheckModule.cpp CheckModule.h \
  Configuration.cpp Configuration.h \
  CPid.cpp CPid.h \
  Debug.cpp Debug.h \
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
  POWERExecution.cpp \
  POWERInterpreter.cpp POWERInterpreter.h \
  POWERARMTraceBuilder.cpp POWERARMTraceBuilder.tcc POWERARMTraceBuilder.h POWERARMTraceBuilder_decl.h \
  PSOInterpreter.cpp PSOInterpreter.h \
  PSOTraceBuilder.cpp PSOTraceBuilder.h \
  SigSegvHandler.cpp SigSegvHandler.h \
  SpinAssumePass.cpp SpinAssumePass.h \
  StrModule.cpp StrModule.h \
  Trace.cpp Trace.h \
  TraceBuilder.cpp TraceBuilder.h \
  Transform.cpp Transform.h \
  TSOInterpreter.cpp TSOInterpreter.h \
  TSOPSOTraceBuilder.h \
  TSOTraceBuilder.cpp TSOTraceBuilder.h \
  VClock.cpp VClock.h VClock.tcc \
  vecset.h vecset.tcc
libnidhugg_a_CXXFLAGS = -fno-rtti

EXTRA_PROGRAMS = nidhuggc
bin_PROGRAMS = nidhugg @NIDHUGGCBIN@
nidhugg_SOURCES = main.cpp
nidhugg_LDADD = libnidhugg.a
nidhugg_CXXFLAGS = -fno-rtti
nidhuggc_SOURCES = nidhuggc.py

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
unittest_LDADD=@BOOST_UNIT_TEST_FRAMEWORK_LIB@ libnidhugg.a


## Run only the tests matching UTEST. Default: all.
## Override by a make command like
## $ make test UTEST='some filter'
UTEST ?= *

.PHONY: test valtest
BOOSTTESTFLAGS=--report_level=short --log_level=warning
test: unittest
	./unittest $(BOOSTTESTFLAGS) --run_test='$(UTEST)'

valtest: unittest
	valgrind --leak-check=full ./unittest $(BOOSTTESTFLAGS) --show_progress --run_test='$(UTEST)'

nidhuggc$(EXEEXT): $(srcdir)/nidhuggc.py
	cat nidhuggc.py \
	| sed 's|%%PYTHON%%|@PYTHON@|g' \
	| sed 's|%%CLANG%%|@CLANG@|g' \
	| sed 's|%%CLANGXX%%|@CLANGXX@|g' \
	| sed 's|%%NIDHUGGPATH%%|$(bindir)/nidhugg|g' \
	> nidhuggc
	chmod a+x nidhuggc
