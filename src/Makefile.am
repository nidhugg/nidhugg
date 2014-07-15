bin_PROGRAMS = nidhugg
TESTS = unittest
check_PROGRAMS = unittest
nidhugg_SOURCES = \
  CheckModule.cpp \
  CPid.cpp \
  Debug.cpp \
  VClock.cpp \
  main.cpp

unittest_SOURCES = \
  CheckModule.cpp \
  CPid.cpp \
  Debug.cpp \
  CPid_test.cpp \
  VClock.cpp \
  VClock_CPid_test.cpp \
  VClock_int_test.cpp \
  unittest.cpp

unittest_LDADD=@BOOST_UNIT_TEST_FRAMEWORK_LIB@

.PHONY: test valtest
BOOSTTESTFLAGS=--report_level=short --log_level=warning
test: unittest
	./unittest $(BOOSTTESTFLAGS)

valtest: unittest
	valgrind --leak-check=full ./unittest $(BOOSTTESTFLAGS) --show_progress
