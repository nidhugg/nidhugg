AUTOMAKE_OPTIONS = -Wno-override ## Make autotools quit complaining about explicit rule for nidhuggc.

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
unittest_LDADD=@BOOST_UNIT_TEST_FRAMEWORK_LIB@ ../src/libnidhugg.a
unittest_CPPFLAGS=-I../src

.PHONY: test valtest
BOOSTTESTFLAGS=--report_level=short --log_level=warning
test: unittest
	./unittest $(BOOSTTESTFLAGS) --run_test="$(UTEST)"

valtest: unittest
	valgrind --leak-check=full ./unittest $(BOOSTTESTFLAGS) --show_progress --run_test="$(UTEST)"

