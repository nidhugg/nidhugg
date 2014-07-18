noinst_LIBRARIES = libnidhugg.a
libnidhugg_a_SOURCES = \
  CheckModule.cpp CheckModule.h \
  Configuration.cpp Configuration.h \
  CPid.cpp CPid.h \
  Debug.cpp Debug.h \
  DPORDriver.cpp DPORDriver.h \
  Execution.cpp \
  ExternalFunctions.cpp \
  IID.h IID.tcc \
  Interpreter.cpp Interpreter.h \
  MRef.h \
  SigSegvHandler.cpp SigSegvHandler.h \
  Trace.cpp Trace.h \
  TraceBuilder.cpp TraceBuilder.h \
  VClock.cpp VClock.h VClock.tcc \
  vecset.h vecset.tcc
libnidhugg_a_CXXFLAGS = -fno-rtti

bin_PROGRAMS = nidhugg
nidhugg_SOURCES = main.cpp
nidhugg_LDADD = libnidhugg.a
nidhugg_CXXFLAGS = -fno-rtti

TESTS = unittest
check_PROGRAMS = unittest
unittest_SOURCES = \
  CPid_test.cpp \
  VClock_CPid_test.cpp \
  VClock_int_test.cpp \
  unittest.cpp
unittest_LDADD=@BOOST_UNIT_TEST_FRAMEWORK_LIB@ libnidhugg.a

.PHONY: test valtest
BOOSTTESTFLAGS=--report_level=short --log_level=warning
test: unittest
	./unittest $(BOOSTTESTFLAGS) --run_test="$(UTEST)"

valtest: unittest
	valgrind --leak-check=full ./unittest $(BOOSTTESTFLAGS) --show_progress --run_test="$(UTEST)"
