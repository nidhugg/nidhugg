AC_INIT([Nidhugg], [0.1], [carl.leonardsson@it.uu.se])
m4_include([m4/ax_cxx_compile_stdcxx_11.m4])
m4_include([m4/ax_boost_base.m4])
m4_include([m4/ax_boost_unit_test_framework.m4])
m4_include([m4/ax_llvm.m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
# AC_CONFIG_SRCDIR([TSOTraceBuilder.cpp])

AC_PROG_CXX
AC_PROG_CC

AC_LANG([C++])

# Checks for C++11
AX_CXX_COMPILE_STDCXX_11()

# Checks for libraries
AX_BOOST_BASE([1.49])
AX_BOOST_UNIT_TEST_FRAMEWORK
AX_LLVM(,[AC_MSG_FAILURE(LLVM is required.)])
CXXFLAGS="`echo " $CXXFLAGS " | sed 's/ -fno-rtti / /' | sed 's/ -fno-exceptions / /'`"
AC_CHECK_LIB([dl], [dlopen],[],[AC_MSG_FAILURE([Could not find library libdl.])])
AC_CHECK_LIB([ffi], [ffi_call],[],[AC_MSG_FAILURE([Could not find library libffi.])])

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
AC_CHECK_HEADER([algorithm],[],[AC_MSG_FAILURE([Could not find header algorithm.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([cassert],[],[AC_MSG_FAILURE([Could not find header cassert.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([cctype],[],[AC_MSG_FAILURE([Could not find header cctype.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([cmath],[],[AC_MSG_FAILURE([Could not find header cmath.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([csetjmp],[],[AC_MSG_FAILURE([Could not find header csetjmp.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([csignal],[],[AC_MSG_FAILURE([Could not find header csignal.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([cstdio],[],[AC_MSG_FAILURE([Could not find header cstdio.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([cstring],[],[AC_MSG_FAILURE([Could not find header cstring.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADERS([ffi/ffi.h],[],[
  AC_CHECK_HEADERS([ffi.h],[],[AC_MSG_FAILURE([Could not find header ffi.h.])],[AC_INCLUDES_DEFAULT])
],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([fstream],[],[AC_MSG_FAILURE([Could not find header fstream.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([functional],[],[AC_MSG_FAILURE([Could not find header functional.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([initializer_list],[],[AC_MSG_FAILURE([Could not find header initializer_list.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([iostream],[],[AC_MSG_FAILURE([Could not find header iostream.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([llvm/ADT/APInt.h],[],[AC_MSG_FAILURE([Could not find header llvm/ADT/APInt.h.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([llvm/ADT/SmallString.h],[],[AC_MSG_FAILURE([Could not find header llvm/ADT/SmallString.h.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([llvm/ADT/Statistic.h],[],[AC_MSG_FAILURE([Could not find header llvm/ADT/Statistic.h.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([llvm/Analysis/LoopPass.h],[],[AC_MSG_FAILURE([Could not find header llvm/Analysis/LoopPass.h.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([llvm/CodeGen/IntrinsicLowering.h],[],[AC_MSG_FAILURE([Could not find header llvm/CodeGen/IntrinsicLowering.h.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([llvm/Config/config.h],[],[AC_MSG_FAILURE([Could not find header llvm/Config/config.h])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([llvm/DebugInfo.h],[],[AC_MSG_FAILURE([Could not find header llvm/DebugInfo.h.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([llvm/ExecutionEngine/ExecutionEngine.h],[],[AC_MSG_FAILURE([Could not find header llvm/ExecutionEngine/ExecutionEngine.h.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([llvm/ExecutionEngine/GenericValue.h],[],[AC_MSG_FAILURE([Could not find header llvm/ExecutionEngine/GenericValue.h.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([llvm/InstVisitor.h],[],[AC_MSG_FAILURE([Could not find header llvm/InstVisitor.h.])],[AC_INCLUDES_DEFAULT])
if test "x$LLVM_INCLUDE_IR" = "xyes"; then
  AC_CHECK_HEADER([llvm/IR/Constants.h],[],[AC_MSG_FAILURE([Could not find header llvm/IR/Constants.h.])],[AC_INCLUDES_DEFAULT])
  AC_CHECK_HEADER([llvm/IR/DataLayout.h],[],[AC_MSG_FAILURE([Could not find header llvm/IR/DataLayout.h.])],[AC_INCLUDES_DEFAULT])
  AC_CHECK_HEADER([llvm/IR/DerivedTypes.h],[],[AC_MSG_FAILURE([Could not find header llvm/IR/DerivedTypes.h.])],[AC_INCLUDES_DEFAULT])
  AC_CHECK_HEADER([llvm/IR/Function.h],[],[AC_MSG_FAILURE([Could not find header llvm/IR/Function.h.])],[AC_INCLUDES_DEFAULT])
  AC_CHECK_HEADER([llvm/IR/InlineAsm.h],[],[AC_MSG_FAILURE([Could not find header llvm/IR/InlineAsm.h.])],[AC_INCLUDES_DEFAULT])
  AC_CHECK_HEADER([llvm/IR/Instructions.h],[],[AC_MSG_FAILURE([Could not find header llvm/IR/Instructions.h.])],[AC_INCLUDES_DEFAULT])
  AC_CHECK_HEADER([llvm/IR/LLVMContext.h],[],[AC_MSG_FAILURE([Could not find header llvm/IR/LLVMContext.h.])],[AC_INCLUDES_DEFAULT])
  AC_CHECK_HEADER([llvm/IR/Metadata.h],[],[AC_MSG_FAILURE([Could not find header llvm/IR/Metadata.h.])],[AC_INCLUDES_DEFAULT])
  AC_CHECK_HEADER([llvm/IR/Module.h],[],[AC_MSG_FAILURE([Could not find header llvm/IR/Module.h.])],[AC_INCLUDES_DEFAULT])
  AC_CHECK_HEADER([llvm/IRReader/IRReader.h],[],[AC_MSG_FAILURE([Could not find header llvm/IRReader/IRReader.h.])],[AC_INCLUDES_DEFAULT])
  AC_CHECK_HEADER([llvm/IR/Type.h],[],[AC_MSG_FAILURE([Could not find header llvm/IR/Type.h.])],[AC_INCLUDES_DEFAULT])
else
  AC_CHECK_HEADER([llvm/Constants.h],[],[AC_MSG_FAILURE([Could not find header llvm/Constants.h.])],[AC_INCLUDES_DEFAULT])
  AC_CHECK_HEADER([llvm/DataLayout.h],[],[AC_MSG_FAILURE([Could not find header llvm/DataLayout.h.])],[AC_INCLUDES_DEFAULT])
  AC_CHECK_HEADER([llvm/DerivedTypes.h],[],[AC_MSG_FAILURE([Could not find header llvm/DerivedTypes.h.])],[AC_INCLUDES_DEFAULT])
  AC_CHECK_HEADER([llvm/Function.h],[],[AC_MSG_FAILURE([Could not find header llvm/Function.h.])],[AC_INCLUDES_DEFAULT])
  AC_CHECK_HEADER([llvm/InlineAsm.h],[],[AC_MSG_FAILURE([Could not find header llvm/InlineAsm.h.])],[AC_INCLUDES_DEFAULT])
  AC_CHECK_HEADER([llvm/Instructions.h],[],[AC_MSG_FAILURE([Could not find header llvm/Instructions.h.])],[AC_INCLUDES_DEFAULT])
  AC_CHECK_HEADER([llvm/LLVMContext.h],[],[AC_MSG_FAILURE([Could not find header llvm/LLVMContext.h.])],[AC_INCLUDES_DEFAULT])
  AC_CHECK_HEADER([llvm/Metadata.h],[],[AC_MSG_FAILURE([Could not find header llvm/Metadata.h.])],[AC_INCLUDES_DEFAULT])
  AC_CHECK_HEADER([llvm/Module.h],[],[AC_MSG_FAILURE([Could not find header llvm/Module.h.])],[AC_INCLUDES_DEFAULT])
  AC_CHECK_HEADER([llvm/IRReader/IRReader.h],[],[AC_MSG_FAILURE([Could not find header llvm/IRReader/IRReader.h.])],[AC_INCLUDES_DEFAULT])
  AC_CHECK_HEADER([llvm/Type.h],[],[AC_MSG_FAILURE([Could not find header llvm/Type.h.])],[AC_INCLUDES_DEFAULT])
fi
AC_CHECK_HEADER([llvm/Pass.h],[],[AC_MSG_FAILURE([Could not find header llvm/Pass.h.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([llvm/Support/CallSite.h],[],[AC_MSG_FAILURE([Could not find header llvm/Support/CallSite.h.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([llvm/Support/CommandLine.h],[],[AC_MSG_FAILURE([Could not find header llvm/Support/CommandLine.h.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([llvm/Support/DataTypes.h],[],[AC_MSG_FAILURE([Could not find header llvm/Support/DataTypes.h.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([llvm/Support/Debug.h],[],[AC_MSG_FAILURE([Could not find header llvm/Support/Debug.h.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([llvm/Support/Dwarf.h],[],[AC_MSG_FAILURE([Could not find header llvm/Support/Dwarf.h.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([llvm/Support/DynamicLibrary.h],[],[AC_MSG_FAILURE([Could not find header llvm/Support/DynamicLibrary.h.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([llvm/Support/ErrorHandling.h],[],[AC_MSG_FAILURE([Could not find header llvm/Support/ErrorHandling.h.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([llvm/Support/GetElementPtrTypeIterator.h],[],[AC_MSG_FAILURE([Could not find header llvm/Support/GetElementPtrTypeIterator.h.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([llvm/Support/Host.h],[],[AC_MSG_FAILURE([Could not find header llvm/Support/Host.h.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([llvm/Support/ManagedStatic.h],[],[AC_MSG_FAILURE([Could not find header llvm/Support/ManagedStatic.h.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([llvm/Support/MathExtras.h],[],[AC_MSG_FAILURE([Could not find header llvm/Support/MathExtras.h.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([llvm/Support/MemoryBuffer.h],[],[AC_MSG_FAILURE([Could not find header llvm/Support/MemoryBuffer.h.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([llvm/Support/Mutex.h],[],[AC_MSG_FAILURE([Could not find header llvm/Support/Mutex.h.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([llvm/Support/raw_ostream.h],[],[AC_MSG_FAILURE([Could not find header llvm/Support/raw_ostream.h.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([llvm/Support/SourceMgr.h],[],[AC_MSG_FAILURE([Could not find header llvm/Support/SourceMgr.h.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([llvm/Transforms/Utils/BasicBlockUtils.h],[],[AC_MSG_FAILURE([Could not find header llvm/Transforms/Utils/BasicBlockUtils.h.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([llvm/Transforms/Utils/Cloning.h],[],[AC_MSG_FAILURE([Could not find header llvm/Transforms/Utils/Cloning.h.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([locale],[],[AC_MSG_FAILURE([Could not find header locale.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([map],[],[AC_MSG_FAILURE([Could not find header map.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([ostream],[],[AC_MSG_FAILURE([Could not find header ostream.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([random],[],[AC_MSG_FAILURE([Could not find header random.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([set],[],[AC_MSG_FAILURE([Could not find header set.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([sstream],[],[AC_MSG_FAILURE([Could not find header sstream.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([stdexcept],[],[AC_MSG_FAILURE([Could not find header stdexcept.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([string],[],[AC_MSG_FAILURE([Could not find header string.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADER([vector],[],[AC_MSG_FAILURE([Could not find header vector.])],[AC_INCLUDES_DEFAULT])

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
