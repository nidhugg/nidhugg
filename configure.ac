AC_INIT([Nidhugg], [0.4], [magnus.lang@it.uu.se])
m4_include([m4/ax_cxx_compile_stdcxx.m4])
m4_include([m4/ax_boost_base.m4])
m4_include([m4/ax_boost_system.m4])
m4_include([m4/ax_boost_unit_test_framework.m4])
m4_include([m4/ax_llvm.m4])
m4_include([m4/ax_git_commit.m4])
m4_include([m4/ax_clang.m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
# AC_CONFIG_SRCDIR([TSOTraceBuilder.cpp])

AM_SILENT_RULES([yes])

AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB
AM_PROG_AR

AC_LANG([C++])

# Checks for C++14
AX_CXX_COMPILE_STDCXX(14, [noext])

AX_GIT_COMMIT

# Checks for libraries
AX_BOOST_BASE([1.65], [], [AC_MSG_FAILURE([Boost Required!])])
AX_BOOST_UNIT_TEST_FRAMEWORK
AX_BOOST_SYSTEM
AX_LLVM(,[AC_MSG_FAILURE(LLVM is required.)])
AC_SUBST(LLVMVERSION)
CXXFLAGS="`echo " $CXXFLAGS " | sed 's/ -fno-rtti / /' | sed 's/ -fno-exceptions / /'` --std=c++14 $BOOST_CPPFLAGS"
LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
AC_SEARCH_LIBS([dlopen], [dl],[],[AC_MSG_FAILURE([Could not find library libdl.])])
ifdef([PKG_CHECK_MODULES],[
  PKG_CHECK_MODULES([FFI], [libffi], [
    AC_DEFINE([HAVE_LIBFFI], [1], [Define to 1 if we have libffi])
    CFLAGS="$CFLAGS $FFI_CFLAGS"
    CXXFLAGS="$CXXFLAGS $FFI_CFLAGS"
    LIBS="$LIBS $FFI_LIBS"
  ],[
    AC_CHECK_LIB([ffi], [ffi_call],[],[AC_MSG_FAILURE([Could not find library libffi.])])
  ])
  PKG_CHECK_MODULES([HWLOC], [hwloc], [
    AC_DEFINE([HAVE_HWLOC], [1], [Define to 1 if we have hwloc])
    CFLAGS="$CFLAGS $HWLOC_CFLAGS"
    CXXFLAGS="$CXXFLAGS $HWLOC_CFLAGS"
    LIBS="$LIBS $HWLOC_LIBS"
  ],[
    # Do nothing if not found
  ])
],[
  errprint([WARNING: pkg-config not found, generating configure script without pkg-config support.
])
  AC_CHECK_LIB([ffi], [ffi_call],[],[AC_MSG_FAILURE([Could not find library libffi (did not try pkg-config).])])
  AC_MSG_WARN([Configuring without hwloc because of no pkg-config.])
])

BUILDNIDHUGGC='yes'
AC_ARG_ENABLE([nidhuggc],[AS_HELP_STRING([--disable-nidhuggc],[Don't build the script nidhuggc.])],
              [if test "x$enableval" = "xno"; then
                 BUILDNIDHUGGC='no'
               elif test "x$enableval" != "xyes"; then
                 AC_MSG_WARN([Unknown value given to --enable-nidhuggc. Defaulting to enabling building of nidhuggc.])
               fi],
              [])
if test "x$BUILDNIDHUGGC" = "xyes"; then
  AX_CLANG(,[
    AC_MSG_WARN([The script nidhuggc cannot be built without clang/clang++.])
    AC_MSG_WARN([Consider indicating the binaries clang/clang++ with switches --with-clang/--with-clangxx,])
    AC_MSG_WARN([or disable nidhuggc with the switch --disable-nidhuggc.])
    AC_MSG_FAILURE([Failed to detect clang/clang++.])
  ])
fi

# Enable assert? Compile with NDEBUG?
AC_ARG_ENABLE([asserts],
              [AS_HELP_STRING([--enable-asserts],[Enable asserts for debugging.])],[
  if test "x$enableval" = "xyes"; then
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
  else
    AC_MSG_NOTICE([Disabling asserts.])
    CXXFLAGS="$CXXFLAGS -DNDEBUG"
  fi
],[CXXFLAGS="$CXXFLAGS -DNDEBUG"])

# Enable (re)building documentation?
AC_ARG_ENABLE([build-documentation],
              [AS_HELP_STRING([--enable-build-documentation],[Enable (re)building of the pdf documentation. (Requires pdflatex.)])],
  [if test "x$enableval" = "xyes"; then
     AC_MSG_NOTICE([Enabling building pdf documentation.])
     BUILDPDFDOCUMENTATION='yes'
   else
     AC_MSG_NOTICE([Disabling building pdf documentation.])
     BUILDPDFDOCUMENTATION='no'
   fi
  ],
  [AC_MSG_NOTICE([Disabling building pdf documentation (default).])
   BUILDPDFDOCUMENTATION='no']
  )

# Check if we can enable the x86 popcnt instruction
AC_CANONICAL_HOST
case "$host_cpu" in
  (i?86|x86_64) popcnt_flag="-mpopcnt";;
  (*)           popcnt_flag="no";;
esac
AC_ARG_ENABLE([popcnt], [AS_HELP_STRING([--disable-popcnt],[Do not try to use the popcnt instruction])] ,
  [if test "x$enableval" != "xno"; then
      if test "x$enableval" != "xyes"; then
         popcnt_flag="$enableval"
      fi
      AC_MSG_NOTICE([Forcing popcnt usage on with flag $enableval.])
      old_CXXFLAGS="$CXXFLAGS"
      old_CFLAGS="$CFLAGS"
      CFLAGS="$CFLAGS $popcnt_flag"
      CXXFLAGS="$CXXFLAGS $popcnt_flag"
   else
      AC_MSG_NOTICE([Forcing popcnt usage unchanged.])
   fi
  ],[
   if test "$popcnt_flag" != "no"; then
     AC_MSG_CHECKING([If cpu supports $popcnt_flag])
     CFLAGS="$CFLAGS $popcnt_flag"
     CXXFLAGS="$CXXFLAGS $popcnt_flag"
     AC_RUN_IFELSE([AC_LANG_SOURCE([[int main(int argc, char *argv[]) {
                      return __builtin_popcount(argc-1);
                    }]])],[AC_MSG_RESULT([yes])],[
         AC_MSG_RESULT([no])
         CXXFLAGS="$old_CXXFLAGS"
         CFLAGS="$old_CFLAGS"
       ],[
         AC_MSG_RESULT([cross-compiling, assuming no])
         CXXFLAGS="$old_CXXFLAGS"
         CFLAGS="$old_CFLAGS"
       ])
   else
      AC_MSG_WARN([Don't know how to enable popcount instruction for $host_cpu.])
   fi
  ])

# Checks for header files
AC_DEFUN([AC_CHECK_HEADERS_ALT],
[
  ac_check_headers_alt_ok="no"
  AC_CHECK_HEADERS([$1],[ac_check_headers_alt_ok="yes"],[],[$4])
  if test "x$ac_check_headers_alt_ok" = "xyes"; then
    $2
    :
  else
    $3
    :
  fi
])

AC_ARG_ENABLE([timing], [Add instrumentation for measuring performance])
if test "x$enable_timing" != "xno"; then
  AC_CHECK_HEADER([pthread.h],[],[
    if test "x$enable_timing" != "xyes"; then
      AC_MSG_FAILURE([Timing instrumentation requires pthread.h.])
    fi
    AC_MSG_WARN([No pthread.h, so timing instrumentation will be disabled.])
    AC_DEFINE([NO_TIMING], [1], [Define to 1 if timing should be disabled])
  ])
else
  AC_MSG_NOTICE([Disabling timing instrumentation.])
  AC_DEFINE([NO_TIMING], [1], [Define to 1 if timing should be disabled])
fi

AC_ARG_ENABLE(smtlib-consistency-check,
              [AS_HELP_STRING([--enable-smtlib-consistency-check],
               [Allow the use of SMTLib compatible SMT solvers as decision procedure for RF-SMC])],
              [if test "x$enableval" != "xno" ; then
                 enable_smtlib_consistency_check=yes
               fi
              ], [enable_smtlib_consistency_check="auto"])
if test "x$enable_smtlib_consistency_check" != "xno" -a "x$ax_cv_boost_system" != "xyes"; then
  if test "x$enable_smtlib_consistency_check" = "xyes"; then
    AC_MSG_FAILURE([SMTLib consistency procedure requires boost-system.])
  fi
  AC_MSG_NOTICE([Disabling SMTLib consistency procedure for RF-SMC because boost-system is not available.])
  AC_DEFINE([NO_SMTLIB_SOLVER], [1], [Define to 1 if timing should be disabled])
elif test "x$enable_smtlib_consistency_check" != "xno"; then
  AC_MSG_CHECKING([whether boost-system can compile with -fno-rtti])
  old_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -fno-rtti"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <boost/process.hpp>
]],[[
  boost::process::ipstream out;
  boost::process::opstream in;
  boost::process::child c;
  in << "hej";
        ]])],
        [AC_MSG_RESULT([yes])],
        [AC_MSG_RESULT([no])
         if test "x$enable_smtlib_consistency_check" = "xyes"; then
           AC_MSG_FAILURE([SMTLib consistency procedure requires boost-system.])
         fi
         AC_MSG_NOTICE([Disabling SMTLib consistency procedure for RF-SMC because boost-system is not compatible with -fno-rtti.])
         AC_DEFINE([NO_SMTLIB_SOLVER], [1], [Define to 1 if timing should be disabled])
        ])
  CXXFLAGS="$old_CXXFLAGS"
else
  AC_MSG_NOTICE([Disabling SMTLib consistency procedure for RF-SMC.])
  AC_DEFINE([NO_SMTLIB_SOLVER], [1], [Define to 1 if timing should be disabled])
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
  regex.h \
  set \
  sstream \
  stdexcept \
  string \
  vector \
  llvm/ADT/APInt.h \
  llvm/ADT/SmallString.h \
  llvm/ADT/Statistic.h \
  llvm/Analysis/LoopPass.h \
  llvm/BinaryFormat/Dwarf.h \
  llvm/CodeGen/IntrinsicLowering.h \
  llvm/ExecutionEngine/ExecutionEngine.h \
  llvm/ExecutionEngine/GenericValue.h \
  llvm/IR/Constants.h \
  llvm/IR/DataLayout.h \
  llvm/IR/DebugInfo.h \
  llvm/IR/DerivedTypes.h \
  llvm/IR/Dominators.h \
  llvm/IR/Function.h \
  llvm/IR/GetElementPtrTypeIterator.h \
  llvm/IR/InlineAsm.h \
  llvm/IR/Instructions.h \
  llvm/IR/InstVisitor.h \
  llvm/IR/IRPrintingPasses.h \
  llvm/IR/LLVMContext.h \
  llvm/IR/Metadata.h \
  llvm/IR/Module.h \
  llvm/IR/PassManager.h \
  llvm/IR/Type.h \
  llvm/IR/Verifier.h \
  llvm/IRReader/IRReader.h \
  llvm/Linker/Linker.h \
  llvm/Pass.h \
  llvm/Support/CommandLine.h \
  llvm/Support/DataTypes.h \
  llvm/Support/Debug.h \
  llvm/Support/DynamicLibrary.h \
  llvm/Support/ErrorHandling.h \
  llvm/Support/ErrorOr.h \
  llvm/Support/FileSystem.h \
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

AC_CHECK_HEADERS([llvm/Support/system_error.h],[],[],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADERS([llvm/Transforms/Utils.h],[],[],[AC_INCLUDES_DEFAULT]) # llvm >= 7

AC_CHECK_HEADERS([ffi.h],[],[
  AC_CHECK_HEADERS([ffi/ffi.h],[],[AC_MSG_FAILURE([Could not find header ffi.h.])],[AC_INCLUDES_DEFAULT])
],[AC_INCLUDES_DEFAULT])

AC_CHECK_HEADERS([ \
  valgrind/valgrind.h
])

# Check various functions in LLVM

## Check if Mutexes use the method "lock" or "acquire"
AC_MSG_CHECKING([for llvm::sys::Mutex::lock])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/Support/Mutex.h>
]],[[
  llvm::sys::Mutex m;
  m.lock();
]])],
        [AC_DEFINE([LLVM_SYS_MUTEX_LOCK_FN],[lock],
         [The name of the locking method of llvm::sys::mutex.])
         AC_DEFINE([LLVM_SYS_MUTEX_UNLOCK_FN],[unlock],
         [The name of the unlocking method of llvm::sys::mutex.])
         AC_MSG_RESULT([lock])],
        [AC_DEFINE([LLVM_SYS_MUTEX_LOCK_FN],[acquire],
         [The name of the locking method of llvm::sys::mutex.])
         AC_DEFINE([LLVM_SYS_MUTEX_UNLOCK_FN],[release],
         [The name of the unlocking method of llvm::sys::mutex.])
         AC_MSG_RESULT([acquire])])

## Check the return type of llvm::MemoryBuffer::getMemBuffer
AC_MSG_CHECKING([for return type of llvm::MemoryBuffer::getMemBuffer])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/Support/MemoryBuffer.h>
]],[[
  llvm::MemoryBuffer *b = llvm::MemoryBuffer::getMemBuffer("");
]])],
        [AC_DEFINE([LLVM_GETMEMBUFFER_RET_PTR],[1],
         [Define if llvm::MemoryBuffer::getMemBuffer returns a pointer.])
         AC_MSG_RESULT([llvm::MemoryBuffer*])],
        [AC_MSG_RESULT([std::unique_ptr<llvm::MemoryBuffer>])])

## Check the type of the error reporting argument for llvm::raw_fd_ostream
AC_MSG_CHECKING([for error reporting in llvm::raw_fd_ostream])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
]],[[
  std::string errs;
#ifdef HAVE_LLVM_SYS_FS_OPENFLAGS
  llvm::raw_fd_ostream("foo",errs,llvm::sys::fs::F_None);
#else
  llvm::raw_fd_ostream("foo",errs,0);
#endif
]])],
        [AC_DEFINE([LLVM_RAW_FD_OSTREAM_ERR_STR],[1],
         [Define if llvm::raw_fd_ostream reports errors through a string reference.])
         AC_MSG_RESULT([string])],
        [AC_MSG_RESULT([error_code])])

## Check whether llvm::ParseIR takes ownership of its buffer
AC_MSG_CHECKING([whether llvm::ParseIR takes ownership of buffer])
AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
]],[[
  llvm::Module *mod;
  llvm::SMDiagnostic err;
  llvm::MemoryBuffer *buf =
#ifdef LLVM_GETMEMBUFFER_RET_PTR
    llvm::MemoryBuffer::getMemBuffer("","",false);
#else
    llvm::MemoryBuffer::getMemBuffer("","",false).release();
#endif
  llvm::LLVMContext ctx;
  mod = llvm::parseIR(buf->getMemBufferRef(),err,ctx).release();
  delete buf;
  delete mod;
  return 0;
]])],
        [AC_MSG_RESULT([no])],
        [AC_DEFINE([LLVM_PARSE_IR_TAKES_OWNERSHIP],[1],
         [Define if llvm::ParseIR takes ownership of its buffer.])
         AC_MSG_RESULT([yes])])

## Check if llvm::LLVMDebugVersion is defined
AC_MSG_CHECKING([for LLVMDebugVersion])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/BinaryFormat/Dwarf.h>
]],[[
  unsigned dbg = llvm::LLVMDebugVersion;
]])],
        [AC_MSG_RESULT([yes])
         AC_DEFINE([HAVE_LLVM_LLVMDEBUGVERSION],[1],
         [Define if llvm::LLVMDebugVersion is defined.])],
        [AC_MSG_RESULT([no])])

## Check whether llvm::MDNode::getOperand returns an llvm::Value
AC_MSG_CHECKING([for llvm::MDNode::getOperand return type])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/IR/Metadata.h>
]],[[
  llvm::MDNode *n = 0;
  llvm::Value *v = n->getOperand(0);
]])],
        [AC_MSG_RESULT([llvm::Value])
         AC_DEFINE([LLVM_MDNODE_OPERAND_IS_VALUE],[1],
         [Define if llvm::MDNode::getOperand returns llvm::Value*.])
         AC_DEFINE([LLVM_METADATA_IS_VALUE],[1],
         [Define if llvm considers metadata to be a value.])
        ],
        [AC_MSG_RESULT([llvm::MDOperand])])

## Check the metadata version number for LLVM
AC_MSG_CHECKING([the LLVM metadata version number])
AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <iostream>
#include <llvm/IR/Metadata.h>
]],[[
  std::cout << llvm::DEBUG_METADATA_VERSION << std::endl;
]])],
  [VN=`./conftest$EXEEXT`
   AC_DEFINE_UNQUOTED([LLVM_METADATA_VERSION_NUMBER],[$VN],
             [The LLVM metadata version number.])
   AC_DEFINE_UNQUOTED([LLVM_METADATA_VERSION_NUMBER_STR],["$VN"],
             [The LLVM metadata version number.])
  ],
  [AC_MSG_RESULT(0)
   AC_DEFINE_UNQUOTED([LLVM_METADATA_VERSION_NUMBER],[0],
             [The LLVM metadata version number.])
   AC_DEFINE_UNQUOTED([LLVM_METADATA_VERSION_NUMBER_STR],["0"],
             [The LLVM metadata version number.])])

## Check if llvm.dbg.declare should take two arguments (or three)
AC_MSG_CHECKING([for number of arguments to llvm.dbg.declare])
AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/ErrorOr.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/SourceMgr.h>
#if defined(HAVE_LLVM_SUPPORT_SYSTEM_ERROR_H)
#include <llvm/Support/system_error.h>
#endif
]],[[
#ifdef LLVM_METADATA_IS_VALUE
  std::string src = R"(
define i8* @p(i8* %arg){
  call void @llvm.dbg.declare(metadata !{i8* %arg}, metadata !0)
  ret i8* null
}
declare void @llvm.dbg.declare(metadata, metadata) nounwind readnone
!llvm.module.flags = !{!1}
!0 = metadata !{i32 0}
!1 = !{i32 2, !"Debug Info Version", i32 )" LLVM_METADATA_VERSION_NUMBER_STR R"(}
)";
#else
  std::string src = R"(
define i8* @p(i8* %arg){
  call void @llvm.dbg.declare(metadata i8* %arg, metadata !0)
  ret i8* null
}
declare void @llvm.dbg.declare(metadata, metadata) nounwind readnone
!llvm.module.flags = !{!1}
!0 = !{i32 0}
!1 = !{i32 2, !"Debug Info Version", i32 )" LLVM_METADATA_VERSION_NUMBER_STR R"(}
)";
#endif
  llvm::Module *mod;
  llvm::SMDiagnostic err;
  llvm::MemoryBuffer *buf =
#ifdef LLVM_GETMEMBUFFER_RET_PTR
    llvm::MemoryBuffer::getMemBuffer(src,"",false);
#else
    llvm::MemoryBuffer::getMemBuffer(src,"",false).release();
#endif
  llvm::LLVMContext ctx;
  mod = llvm::parseIR(buf->getMemBufferRef(),err,ctx).release();
  if(!mod) return 1;
]])],
  [AC_MSG_RESULT(2)
   AC_DEFINE([LLVM_DBG_DECLARE_TWO_ARGS],[1],
   [Define if llvm.dbg.declare takes two arguments (rather than three).])],
  [AC_MSG_RESULT(3)])

## Check how to call getAnalysis to get LoopInfo
AC_MSG_CHECKING([how to get LoopInfo analysis])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/LoopPass.h>
]],[[
  llvm::LPPassManager *LPM = 0;
  LPM->getAnalysis<llvm::LoopInfo>();
]])],
        [AC_MSG_RESULT([llvm::LoopInfo])
         AC_DEFINE([LLVM_GET_ANALYSIS_LOOP_INFO],[1],
         [Define if LPPassManager::getAnalysis<LoopInfo> can be called.])],
        [AC_MSG_RESULT([llvm::LoopInfoWrapperPass])])

## Checking if llvm::PassManager requires a template argument
AC_MSG_CHECKING([if llvm::PassManager requires a template argument])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/IR/PassManager.h>
]],[[
  llvm::PassManager PM;
]])],
        [AC_MSG_RESULT([no])],
        [AC_MSG_RESULT([yes])
         AC_DEFINE([LLVM_PASSMANAGER_TEMPLATE],[1],
         [Define if llvm::PassManager requires a template argument.])
         AC_CHECK_HEADERS([llvm/IR/LegacyPassManager.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])])

## Checking if an llvm::DILocation is an llvm::MDNode
AC_MSG_CHECKING([if an llvm::DILocation is an llvm::MDNode])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/IR/DebugInfo.h>
]],[[
  llvm::MDNode *m = 0;
  llvm::DILocation *l = static_cast<llvm::DILocation*>(m);
]])],
        [AC_MSG_RESULT([yes])
         AC_DEFINE([LLVM_DILOCATION_IS_MDNODE],[1],
         [Define if an llvm::DILocation is an MDNode.])],
        [AC_MSG_RESULT([no])])

## Checking if llvm::DILocation::getLineNumber is a method.
AC_MSG_CHECKING([if llvm::DILocation::getLineNumber is a method.])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/IR/DebugInfo.h>
]],[[
  llvm::DILocation *l;
  l->getLineNumber();
]])],
        [AC_MSG_RESULT([yes])
         AC_DEFINE([LLVM_DILOCATION_HAS_GETLINENUMBER],[1],
         [Define if llvm::DILocation::getLineNumber is a method.])],
        [AC_MSG_RESULT([no])])


## Check if loads in assembly should explicitly state their return type
AC_MSG_CHECKING([if assembly loads explicitly state their return type])
AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/ErrorOr.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/SourceMgr.h>
#if defined(HAVE_LLVM_SUPPORT_SYSTEM_ERROR_H)
#include <llvm/Support/system_error.h>
#endif
]],[[
  std::string src = R"(
@x = global i32 0, align 4
define i8* @p(i8* %arg){
  load i32* @x, align 4
  ret i8* null
}
)";
  llvm::Module *mod;
  llvm::SMDiagnostic err;
  llvm::MemoryBuffer *buf =
#ifdef LLVM_GETMEMBUFFER_RET_PTR
    llvm::MemoryBuffer::getMemBuffer(src,"",false);
#else
  llvm::MemoryBuffer::getMemBuffer(src,"",false).release();
#endif
  llvm::LLVMContext ctx;
  mod = llvm::parseIR(buf->getMemBufferRef(),err,ctx).release();
  if(!mod) return 1;
]])],
  [AC_MSG_RESULT(no)],
  [AC_MSG_RESULT(yes)
   AC_DEFINE([LLVM_ASM_LOAD_EXPLICIT_TYPE],[1],
   [Define if assembly loads should explicitly state their return type.])])

## Check if llvm::LoopInfo::erase exists
AC_MSG_CHECKING([for llvm::LoopInfo::erase])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/LoopPass.h>
]],[[
  llvm::Loop * L = 0;
  llvm::LoopInfo *LI = 0;
  LI->erase(L);
]])],
  [AC_MSG_RESULT([yes])
   AC_DEFINE([HAVE_LLVM_LOOPINFO_ERASE],[1],
   [Define if llvm::LoopInfo::erase exists])],
  [AC_MSG_RESULT([no])])

## Check if llvm::LoopInfo::markAsRemoved exists
AC_MSG_CHECKING([for llvm::LoopInfo::markAsRemoved])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/LoopPass.h>
]],[[
  llvm::Loop * L = 0;
  llvm::LoopInfo *LI = 0;
  LI->markAsRemoved(L);
]])],
  [AC_MSG_RESULT([yes])
   AC_DEFINE([HAVE_LLVM_LOOPINFO_MARK_AS_REMOVED],[1],
   [Define if llvm::LoopInfo::markAsRemoved exists])],
  [AC_MSG_RESULT([no])])

## Check whether we have TypeIDs for scalable vector types (LLVM>=11)
AC_MSG_CHECKING([for llvm vector TypeIDs])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/IR/Instructions.h>
]],[[
  llvm::Type *T = nullptr;
  bool isVector = T->getTypeID() == llvm::Type::VectorTyID;
]])],
        [AC_DEFINE([LLVM_VECTOR_TYPEID_CASES],[case llvm::Type::VectorTyID:],
         [Case labels for recognising vector types.])
         AC_MSG_RESULT([VectorTyID])],
        [AC_DEFINE([LLVM_VECTOR_TYPEID_CASES],
         [case llvm::Type::FixedVectorTyID: case llvm::Type::ScalableVectorTyID:],
         [Case labels for recognising vector types.])
         AC_MSG_RESULT([FixedVectorTyID, ScalableVectorTyID])])


# Checks for typedefs, structures, and compiler characteristics.
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

if test "x$BUILDNIDHUGGC" = "xyes"; then
  # Checks for python3
  AM_PATH_PYTHON([3.0],[],[
    AC_MSG_WARN([Python 3 is required for the script nidhuggc.])
    AC_MSG_WARN([Consider indicating the python interpreter with PYTHON=path,])
    AC_MSG_WARN([or disable building of nidhuggc with --disable-nidhuggc.])
    AC_MSG_FAILURE([Python 3 is required.])
  ])
  AC_SUBST(PYTHON)
  NIDHUGGCBIN='nidhuggc'
else
  NIDHUGGCBIN=''
fi

AC_SUBST(NIDHUGGCBIN)
AM_CONDITIONAL([BUILDPDFDOCUMENTATION],[test "x$BUILDPDFDOCUMENTATION" = "xyes"])

# Warn about compiling with clang++ for llvm version >= 3.9
if test "x$CXX" = "xclang" || test "x$CXX" = "xclang++" ; then
  # Version comparison using sort -V
  if test "$(printf '%s\n' '3.9' "$LLVMVERSION" | sort -V | head -n 1)" = '3.9'; then
    AC_MSG_WARN([])
    AC_MSG_WARN([Trying to compile with $CXX against LLVM version >= 3.9.])
    AC_MSG_WARN([This has been known to cause problems. (Segmentation faults on execution of nidhugg.)])
    AC_MSG_WARN([It is recommended to use g++ instead of $CXX.])
    AC_MSG_WARN([])
  fi
fi

AM_EXTRA_RECURSIVE_TARGETS([test run_unittest valtest litmus_test smoke_test])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
  Makefile
  src/Makefile
  doc/Makefile
])
AC_OUTPUT
