AUTOMAKE_OPTIONS = -Wno-override ## Make autotools quit complaining about explicit rule for nidhuggc.

noinst_LIBRARIES = libnidhugg.a
libnidhugg_a_SOURCES = \
  AddLibPass.cpp AddLibPass.h \
  AssumeAwaitPass.cpp AssumeAwaitPass.h \
  AwaitCond.cpp AwaitCond.h \
  BVClock.cpp BVClock.h \
  CastElimPass.cpp CastElimPass.h \
  CheckModule.cpp CheckModule.h \
  Configuration.cpp Configuration.h \
  CPid.cpp CPid.h \
  Cpubind.cpp Cpubind.h \
  DeadCodeElimPass.cpp DeadCodeElimPass.h \
  Debug.cpp Debug.h \
  DetCheckTraceBuilder.cpp DetCheckTraceBuilder.h \
  DPORDriver.cpp DPORDriver.h \
  DPORInterpreter.h \
  Execution.cpp \
  ExternalFunctions.cpp \
  FBVClock.cpp FBVClock.h \
  GlobalContext.cpp GlobalContext.h \
  IID.h IID.tcc \
  Interpreter.cpp Interpreter.h \
  LoopBoundPass.cpp LoopBoundPass.h \
  MRef.cpp MRef.h \
  nregex.cpp nregex.h \
  Option.h \
  PartialLoopPurityPass.h PartialLoopPurityPass.cpp \
  POWERExecution.cpp \
  POWERInterpreter.cpp POWERInterpreter.h \
  POWERARMTraceBuilder.cpp POWERARMTraceBuilder.tcc POWERARMTraceBuilder.h POWERARMTraceBuilder_decl.h \
  PrefixHeuristic.h PrefixHeuristic.cpp \
  PSOInterpreter.cpp PSOInterpreter.h \
  PSOTraceBuilder.cpp PSOTraceBuilder.h \
  SaturatedGraph.h SaturatedGraph.cpp \
  SatSolver.h \
  Seqno.h \
  SExpr.h SExpr.cpp \
  SmtlibSatSolver.h SmtlibSatSolver.cpp \
  SpinAssumePass.cpp SpinAssumePass.h \
  StrModule.cpp StrModule.h \
  SymEv.cpp SymEv.h \
  SymAddr.cpp SymAddr.h \
  Timing.cpp Timing.h \
  Trace.cpp Trace.h \
  TraceUtil.cpp TraceUtil.h \
  TraceBuilder.cpp TraceBuilder.h \
  Transform.cpp Transform.h \
  TSOInterpreter.cpp TSOInterpreter.h \
  TSOPSOTraceBuilder.h \
  TSOTraceBuilder.cpp TSOTraceBuilder.h \
  RFSCUnfoldingTree.cpp RFSCUnfoldingTree.h \
  RFSCDecisionTree.cpp RFSCDecisionTree.h \
  RFSCTraceBuilder.cpp RFSCTraceBuilder.h \
  RMWAction.cpp RMWAction.h \
  VClock.cpp VClock.h VClock.tcc \
  vecset.h vecset.tcc
libnidhugg_a_CXXFLAGS = -fno-rtti

EXTRA_PROGRAMS = nidhuggc
bin_PROGRAMS = nidhugg @NIDHUGGCBIN@
nidhugg_SOURCES = main.cpp
nidhugg_LDADD = libnidhugg.a @BOOST_SYSTEM_LIB@
nidhugg_LDFLAGS = -pthread
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
  GenMap_test.cpp \
  GenVector_test.cpp \
  Interpreter_test.cpp \
  nregex_test.cpp \
  Observers_test.cpp \
  POWER_test.cpp \
  POWER_test2.cpp \
  PSO_test.cpp \
  PSO_test2.cpp \
  Regression_test.cpp \
  RFSC_test.cpp \
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
unittest_LDADD=libnidhugg.a @BOOST_SYSTEM_LIB@ @BOOST_UNIT_TEST_FRAMEWORK_LIB@
unittest_LDFLAGS=-pthread


## Run only the tests matching UTEST. Default: all.
## Override by a make command like
## $ make test UTEST='some filter'
UTEST ?=

.PHONY: test valtest litmus mustpass
BOOSTTESTFLAGS=--report_level=short --log_level=warning
test: run_unittest smoke_test
run_unittest: unittest
	./unittest $(BOOSTTESTFLAGS) `test -z "$(UTEST)" || echo "--run_test=$(UTEST)"`

valtest: unittest
	valgrind --leak-check=full ./unittest $(BOOSTTESTFLAGS) --show_progress \
		`test -z "$(UTEST)" || echo "--run_test=$(UTEST)"`
litmus_test: nidhugg$(EXEEXT) nidhuggc$(EXEEXT)
	cd $(top_srcdir)/tests/litmus && \
	python3 ./test-nidhugg.py --nidhuggc=$(CURDIR)/nidhuggc all

smoke_test: nidhugg$(EXEEXT) nidhuggc$(EXEEXT)
	cd $(top_srcdir)/tests/smoke && \
	python3 ./test-nidhugg.py --nidhuggc=$(CURDIR)/nidhuggc all

nidhuggc$(EXEEXT): $(srcdir)/nidhuggc.py
	$(AM_V_GEN)cat $(srcdir)/nidhuggc.py \
	| sed 's|%%PYTHON%%|@PYTHON@|g' \
	| sed 's|%%CLANG%%|@CLANG@|g' \
	| sed 's|%%CLANGXX%%|@CLANGXX@|g' \
	> nidhuggc
	$(AM_V_at)chmod a+x nidhuggc
