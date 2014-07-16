noinst_LIBRARIES = libnidhugg.a
libnidhugg_a_SOURCES = \
  CheckModule.cpp \
  Configuration.cpp \
  CPid.cpp \
  Debug.cpp \
  DPORDriver.cpp \
  Trace.cpp \
  VClock.cpp
libnidhugg_a_CXXFLAGS = -fno-rtti

bin_PROGRAMS = nidhugg
nidhugg_SOURCES = main.cpp
nidhugg_LDADD = libnidhugg.a
nidhugg_CXXFLAGS = -fno-rtti

TESTS = unittest
check_PROGRAMS = unittest
unittest_SOURCES = \
  VClock_CPid_test.cpp \
  VClock_int_test.cpp \
  unittest.cpp
unittest_LDADD=@BOOST_UNIT_TEST_FRAMEWORK_LIB@ libnidhugg.a

.PHONY: test valtest
BOOSTTESTFLAGS=--report_level=short --log_level=warning
test: unittest
	./unittest $(BOOSTTESTFLAGS)

valtest: unittest
	valgrind --leak-check=full ./unittest $(BOOSTTESTFLAGS) --show_progress
