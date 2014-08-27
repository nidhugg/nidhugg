AC_INIT([Nidhugg], [0.1], [carl.leonardsson@it.uu.se])
m4_include([m4/ax_cxx_compile_stdcxx_11.m4])
m4_include([m4/ax_boost_base.m4])
m4_include([m4/ax_boost_unit_test_framework.m4])
m4_include([m4/ax_llvm.m4])
m4_include([m4/ax_git_commit.m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
# AC_CONFIG_SRCDIR([TSOTraceBuilder.cpp])

AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB
AM_PROG_AR

AC_LANG([C++])

# Checks for C++11
# noext to avoid sending -std=gnu++11 to clang++ (Causes problems for clang++-3.3.)
AX_CXX_COMPILE_STDCXX_11([noext])

AX_GIT_COMMIT

# Checks for libraries
AX_BOOST_BASE([1.49])
AX_BOOST_UNIT_TEST_FRAMEWORK
AX_LLVM(,[AC_MSG_FAILURE(LLVM is required.)])
CXXFLAGS="`echo " $CXXFLAGS " | sed 's/ -fno-rtti / /' | sed 's/ -fno-exceptions / /'`"
AC_CHECK_LIB([dl], [dlopen],[],[AC_MSG_FAILURE([Could not find library libdl.])])
AC_CHECK_LIB([ffi], [ffi_call],[
                    AC_DEFINE([HAVE_FFI_CALL],[1],[Define if libffi is available on this platform.])
                    ],[AC_MSG_FAILURE([Could not find library libffi.])])

# Enable assert? Compile with NDEBUG?
AC_ARG_ENABLE([asserts],
              [AS_HELP_STRING([--enable-asserts],[Enable asserts for debugging.])],[
  if test "x$LLVM_NDEBUG" = "xyes"; then
    AC_MSG_WARN([LLVM was compiled without asserts, and incompatible with asserts build.])
    AC_MSG_WARN([  Refusing to enable asserts, despite --enable-asserts.])
    AC_MSG_WARN([  To enable asserts, try compiling against a debug build of LLVM.])
  elif test "x`echo "$CXXFLAGS" | grep -e -DNDEBUG`" != "x"; then
    AC_MSG_WARN([Asserts have been previously disabled for some reason.])
    AC_MSG_WARN([  Refusing to enable asserts, despite --enable-asserts.])
  else
    AC_MSG_NOTICE([Enabling asserts.])
  fi
],[CXXFLAGS="$CXXFLAGS -DNDEBUG"])

# Checks for header files
if test "x$BOOST_UNIT_TEST_FRAMEWORK_LIB" != "x"; then
  AC_CHECK_HEADER([boost/test/unit_test.hpp],[],[AC_MSG_FAILURE([Could not find header boost/test/unit_test.hpp.])],[AC_INCLUDES_DEFAULT])
fi
AC_CHECK_HEADERS([ \
  stdlib.h \
  sys/types.h \
  sys/stat.h \
  inttypes.h \
  stdint.h \
  algorithm \
  cassert \
  cctype \
  cmath \
  csetjmp \
  csignal \
  cstdio \
  cstring \
  fstream \
  functional \
  initializer_list \
  iostream \
  locale \
  map \
  ostream \
  random \
  set \
  sstream \
  stdexcept \
  string \
  vector \
  llvm/ADT/APInt.h \
  llvm/ADT/SmallString.h \
  llvm/ADT/Statistic.h \
  llvm/Analysis/LoopPass.h \
  llvm/CodeGen/IntrinsicLowering.h \
  llvm/DebugInfo.h \
  llvm/ExecutionEngine/ExecutionEngine.h \
  llvm/ExecutionEngine/GenericValue.h \
  llvm/InstVisitor.h \
  llvm/Pass.h \
  llvm/Support/CallSite.h \
  llvm/Support/CommandLine.h \
  llvm/Support/DataTypes.h \
  llvm/Support/Debug.h \
  llvm/Support/Dwarf.h \
  llvm/Support/DynamicLibrary.h \
  llvm/Support/ErrorHandling.h \
  llvm/Support/GetElementPtrTypeIterator.h \
  llvm/Support/Host.h \
  llvm/Support/ManagedStatic.h \
  llvm/Support/MathExtras.h \
  llvm/Support/MemoryBuffer.h \
  llvm/Support/Mutex.h \
  llvm/Support/raw_ostream.h \
  llvm/Support/SourceMgr.h \
  llvm/Transforms/Utils/BasicBlockUtils.h \
  llvm/Transforms/Utils/Cloning.h
],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])

if test "x$LLVM_INCLUDE_IR" = "xyes"; then
  AC_CHECK_HEADERS([ \
    llvm/IR/Constants.h \
    llvm/IR/DataLayout.h \
    llvm/IR/DerivedTypes.h \
    llvm/IR/Function.h \
    llvm/IR/InlineAsm.h \
    llvm/IR/Instructions.h \
    llvm/IR/LLVMContext.h \
    llvm/IR/Metadata.h \
    llvm/IR/Module.h \
    llvm/IRReader/IRReader.h \
    llvm/IR/Type.h
],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])
else
  AC_CHECK_HEADERS([ \
    llvm/Constants.h \
    llvm/DataLayout.h \
    llvm/DerivedTypes.h \
    llvm/Function.h \
    llvm/InlineAsm.h \
    llvm/Instructions.h \
    llvm/LLVMContext.h \
    llvm/Metadata.h \
    llvm/Module.h \
    llvm/IRReader/IRReader.h \
    llvm/Type.h
],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])
fi

AC_CHECK_HEADERS([ffi/ffi.h],[],[
  AC_CHECK_HEADERS([ffi.h],[],[AC_MSG_FAILURE([Could not find header ffi.h.])],[AC_INCLUDES_DEFAULT])
],[AC_INCLUDES_DEFAULT])


# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for python3
AM_PATH_PYTHON([3.0],[],[AC_MSG_FAILURE("Python 3 is required.")])

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([atexit memset setenv])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
  Makefile
  src/Makefile
])
AC_OUTPUT
