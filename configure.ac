AC_INIT([Nidhugg], [0.1], [carl.leonardsson@it.uu.se])
m4_include([m4/ax_cxx_compile_stdcxx_11.m4])
m4_include([m4/ax_boost_base.m4])
m4_include([m4/ax_boost_unit_test_framework.m4])
m4_include([m4/ax_llvm.m4])
m4_include([m4/ax_git_commit.m4])
m4_include([m4/ax_clang.m4])
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
  llvm/ExecutionEngine/ExecutionEngine.h \
  llvm/ExecutionEngine/GenericValue.h \
  llvm/IRReader/IRReader.h \
  llvm/Pass.h \
  llvm/Support/CommandLine.h \
  llvm/Support/DataTypes.h \
  llvm/Support/Debug.h \
  llvm/Support/Dwarf.h \
  llvm/Support/DynamicLibrary.h \
  llvm/Support/ErrorHandling.h \
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

AC_CHECK_HEADERS_ALT([llvm/Support/CallSite.h llvm/IR/CallSite.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADERS_ALT([llvm/Constants.h llvm/IR/Constants.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADERS_ALT([llvm/DataLayout.h llvm/IR/DataLayout.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADERS_ALT([llvm/DebugInfo.h llvm/Analysis/DebugInfo.h llvm/IR/DebugInfo.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADERS_ALT([llvm/DerivedTypes.h llvm/IR/DerivedTypes.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADERS_ALT([llvm/IR/Dominators.h llvm/Analysis/Dominators.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADERS_ALT([llvm/Function.h llvm/IR/Function.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADERS_ALT([llvm/Support/GetElementPtrTypeIterator.h llvm/IR/GetElementPtrTypeIterator.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADERS_ALT([llvm/InlineAsm.h llvm/IR/InlineAsm.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADERS_ALT([llvm/Instructions.h llvm/IR/Instructions.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADERS_ALT([llvm/InstVisitor.h llvm/IR/InstVisitor.h llvm/Support/InstVisitor.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADERS_ALT([llvm/Linker.h llvm/Linker/Linker.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADERS_ALT([llvm/LLVMContext.h llvm/IR/LLVMContext.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADERS_ALT([llvm/Metadata.h llvm/IR/Metadata.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADERS_ALT([llvm/Module.h llvm/IR/Module.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADERS_ALT([llvm/Type.h llvm/IR/Type.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADERS_ALT([llvm/Assembly/PrintModulePass.h llvm/IR/IRPrintingPasses.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADERS_ALT([llvm/Analysis/Verifier.h llvm/IR/Verifier.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],AC_INCLUDES_DEFAULT[])
AC_CHECK_HEADERS_ALT([llvm/Support/ErrorOr.h],[],[AC_MSG_FAILURE([Could not find necessary headers.])],[AC_INCLUDES_DEFAULT])
AC_CHECK_HEADERS([llvm/Support/system_error.h],[],[],[AC_INCLUDES_DEFAULT])

AC_CHECK_HEADERS([ffi/ffi.h],[],[
  AC_CHECK_HEADERS([ffi.h],[],[AC_MSG_FAILURE([Could not find header ffi.h.])],[AC_INCLUDES_DEFAULT])
],[AC_INCLUDES_DEFAULT])

AC_CHECK_HEADERS([valgrind/valgrind.h])

# Check various functions in LLVM

## Check which method in llvm::Module returns the data layout string
AC_MSG_CHECKING([for llvm::Module::getDataLayoutStr])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if defined(HAVE_LLVM_MODULE_H)
#include <llvm/Module.h>
#elif defined(HAVE_LLVM_IR_MODULE_H)
#include <llvm/IR/Module.h>
#endif
#if defined(HAVE_LLVM_LLVMCONTEXT_H)
#include <llvm/LLVMContext.h>
#elif defined(HAVE_LLVM_IR_LLVMCONTEXT_H)
#include <llvm/IR/LLVMContext.h>
#endif
]],[[
          llvm::Module M("",llvm::getGlobalContext());
          std::string s = M.getDataLayoutStr();
        ]])],
        [AC_DEFINE([LLVM_MODULE_GET_DATA_LAYOUT_STRING],[getDataLayoutStr],
            [The name of the method in llvm::Module which returns the data layout string.])
         AC_MSG_RESULT([getDataLayoutStr])
        ],
        [AC_DEFINE([LLVM_MODULE_GET_DATA_LAYOUT_STRING],[getDataLayout],
            [The name of the method in llvm::Module which returns the data layout string.])
         AC_MSG_RESULT([getDataLayout])
        ])

## Check the signature of llvm::Module::materializeAllPermanently
AC_MSG_CHECKING([for llvm::Module::MaterializeAllPermanently])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if defined(HAVE_LLVM_MODULE_H)
#include <llvm/Module.h>
#elif defined(HAVE_LLVM_IR_MODULE_H)
#include <llvm/IR/Module.h>
#endif
#if defined(HAVE_LLVM_LLVMCONTEXT_H)
#include <llvm/LLVMContext.h>
#elif defined(HAVE_LLVM_IR_LLVMCONTEXT_H)
#include <llvm/IR/LLVMContext.h>
#endif
]],[[
  llvm::Module M("",llvm::getGlobalContext());
  std::error_code ec = M.materializeAllPermanently();
]])],
           [AC_DEFINE([LLVM_MODULE_MATERIALIZE_ALL_PERMANENTLY_ERRORCODE_BOOL],[1],
            [Define if Module::materializeAllPermanently accepts zero arguments and returns an error_code.])
            AC_MSG_RESULT([(bool) -> error_code])
           ],
           [AC_MSG_RESULT([string -> bool])])

## Check if AtomicCmpXchg has separate success and failure orderings
AC_MSG_CHECKING([for success/failure orderings of llvm::AtomicCmpXchgInst])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if defined(HAVE_LLVM_INSTRUCTIONS_H)
#include <llvm/Instructions.h>
#elif defined(HAVE_LLVM_IR_INSTRUCTIONS_H)
#include <llvm/IR/Instructions.h>
#endif
]],[[
  llvm::AtomicCmpXchgInst I((llvm::Value*)0,(llvm::Value*)0,(llvm::Value*)0,
                            llvm::AtomicOrdering(llvm::SequentiallyConsistent),
                            llvm::AtomicOrdering(llvm::SequentiallyConsistent),
                            llvm::SynchronizationScope(llvm::CrossThread),
                            (llvm::Instruction*)0);
]])],
           [AC_DEFINE([LLVM_CMPXCHG_SEPARATE_SUCCESS_FAILURE_ORDERING],[1],
            [Define if llvm::AtomicCmpXchgInst has separate orderings for success and failure.])
            AC_MSG_RESULT([Separate])
           ],
           [AC_MSG_RESULT([Combined])])

## Check if DominatorTrees should be generated by a wrapper pass
AC_MSG_CHECKING([for llvm::DominatorTreeWrapperPass])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if defined(HAVE_LLVM_IR_DOMINATORS_H)
#include <llvm/IR/Dominators.h>
#elif defined(HAVE_LLVM_ANALYSIS_DOMINATORS_H)
#include <llvm/Analysis/Dominators.h>
#endif
]],[[
  return llvm::DominatorTreeWrapperPass::ID;
]])],
           [AC_DEFINE([LLVM_DOMINATOR_TREE_PASS],[DominatorTreeWrapperPass],
            [The name of the FunctionPass computing dominator trees.])
            AC_MSG_RESULT([DominatorTreeWrapperPass])
           ],
           [AC_DEFINE([LLVM_DOMINATOR_TREE_PASS],[DominatorTree],
            [The name of the FunctionPass computing dominator trees.])
            AC_MSG_RESULT([DominatorTree])])

## Check the signature of MemoryBuffer::getFile
AC_MSG_CHECKING([signature of llvm::MemoryBuffer::getFile])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/ADT/OwningPtr.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/system_error.h>
]],[[
  llvm::OwningPtr<llvm::MemoryBuffer> buf;
  llvm::error_code ec =
    llvm::MemoryBuffer::getFile("",buf);
]])],
           [AC_DEFINE([LLVM_MEMORY_BUFFER_GET_FILE_OWNINGPTR_ARG],[1],
            [Define if llvm::MemoryBuffer::getFile takes an OwningPtr as an output argument.])
            AC_MSG_RESULT([OwningPtr output argument])
           ],
           [AC_MSG_RESULT([Returns unique_ptr.])])

## Check the signature of createPrintModulePass
AC_MSG_CHECKING([signature of llvm::createPrintModulePass])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if defined(HAVE_LLVM_ASSEMBLY_PRINTMODULEPASS_H)
#include <llvm/Assembly/PrintModulePass.h>
#elif defined(HAVE_LLVM_IR_IRPRINTINGPASSES_H)
#include <llvm/IR/IRPrintingPasses.h>
#endif
]],[[
  createPrintModulePass((llvm::raw_ostream*)0);
]])],
           [AC_DEFINE([LLVM_CREATE_PRINT_MODULE_PASS_PTR_ARG],[1],
            [Define if llvm::createPrintModulePass takes a pointer argument.])
            AC_MSG_RESULT([Takes pointer to raw_ostream])],
           [AC_MSG_RESULT([Takes reference to raw_ostream])])

## Check if the value F_None in llvm::sys::fs::OpenFlags exists
AC_MSG_CHECKING([for llvm::sys::fs::OpenFlags (F_None)])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/Support/FileSystem.h>
]],[[
  llvm::sys::fs::OpenFlags f = llvm::sys::fs::F_None;
  return f;
]])],
           [AC_DEFINE([HAVE_LLVM_SYS_FS_OPENFLAGS],[1],
            [Define if the type llvm::sys::fs::OpenFlags exists.])
            AC_MSG_RESULT([yes])],
           [AC_MSG_RESULT([no])])

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

## Check the signature of llvm::ParseIR
AC_MSG_CHECKING([for llvm::ParseIR])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#if defined(HAVE_LLVM_LLVMCONTEXT_H)
#include <llvm/LLVMContext.h>
#elif defined(HAVE_LLVM_IR_LLVMCONTEXT_H)
#include <llvm/IR/LLVMContext.h>
#endif
]],[[
  llvm::SMDiagnostic err;
  llvm::ParseIR((llvm::MemoryBuffer*)0,
                err,
                llvm::getGlobalContext());
]])],
        [AC_DEFINE([LLVM_PARSE_IR_MEMBUF_PTR],[1],
         [Define if llvm::ParseIR takes a MemoryBuffer pointer as argument.])
         AC_MSG_RESULT([takes MemoryBuffer*])],
        [AC_MSG_RESULT([takes MemoryBufferRef])])

## Check the return type of llvm::MemoryBuffer::getMemBuffer
AC_MSG_CHECKING([for return type of llvm::MemoryBuffer::getMemBuffer])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/Support/MemoryBuffer.h>
]],[[
  llvm::MemoryBuffer *b =
    llvm::MemoryBuffer::getMemBuffer("");
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

## Check whether an ExecutionEngine takes a Module pointer or a Module unique_ptr
AC_MSG_CHECKING([for unique_ptr<Module> in ExecutionEngine])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/ExecutionEngine/ExecutionEngine.h>
]],[[
  class EE : public llvm::ExecutionEngine {
  public:
    EE(llvm::Module *M) : llvm::ExecutionEngine(std::unique_ptr<llvm::Module>(M)) {};
  };
]])],
        [AC_DEFINE([LLVM_EXECUTIONENGINE_MODULE_UNIQUE_PTR],[1],
         [Define if llvm::ExecutionEngine takes a unique_ptr to its Module.])
         AC_MSG_RESULT([yes])],
        [AC_MSG_RESULT([no])])

## Check whether llvm::ParseIR takes ownership of its buffer
AC_MSG_CHECKING([whether llvm::ParseIR takes ownership of buffer])
AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#if defined(HAVE_LLVM_MODULE_H)
#include <llvm/Module.h>
#elif defined(HAVE_LLVM_IR_MODULE_H)
#include <llvm/IR/Module.h>
#endif
#if defined(HAVE_LLVM_LLVMCONTEXT_H)
#include <llvm/LLVMContext.h>
#elif defined(HAVE_LLVM_IR_LLVMCONTEXT_H)
#include <llvm/IR/LLVMContext.h>
#endif
]],[[
    llvm::Module *mod;
    llvm::SMDiagnostic err;
    llvm::MemoryBuffer *buf =
#ifdef LLVM_GETMEMBUFFER_RET_PTR
      llvm::MemoryBuffer::getMemBuffer("","",false);
#else
      llvm::MemoryBuffer::getMemBuffer("","",false).release();
#endif
#ifdef LLVM_PARSE_IR_MEMBUF_PTR
    mod = llvm::ParseIR(buf,err,llvm::getGlobalContext());
#else
    mod = llvm::parseIR(buf->getMemBufferRef(),err,llvm::getGlobalContext()).release();
#endif
    delete buf;
    delete mod;
    return 0;
]])],
        [AC_MSG_RESULT([no])],
        [AC_DEFINE([LLVM_PARSE_IR_TAKES_OWNERSHIP],[1],
         [Define if llvm::ParseIR takes ownership of its buffer.])
         AC_MSG_RESULT([yes])])

## Check the signature of Linker::linkInModule
AC_MSG_CHECKING([the signature of Linker::linkInModule])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if defined(HAVE_LLVM_LINKER_H)
#include <llvm/Linker.h>
#elif defined(HAVE_LLVM_LINKER_LINKER_H)
#include <llvm/Linker/Linker.h>
#endif
]],[[
  llvm::Linker lnk((llvm::Module*)0);
  bool b = lnk.linkInModule((llvm::Module*)0);
]])],
        [AC_MSG_RESULT([Module* -> bool])],
        [AC_MSG_RESULT([(Module*,unsigned,string*) -> bool])
         AC_DEFINE([LLVM_LINKER_LINKINMODULE_HAS_MODE],[1],
         [Define if llvm::Linker::linkInModule takes a mode argument.])])

## Check if llvm::LLVMDebugVersion is defined
AC_MSG_CHECKING([for LLVMDebugVersion])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <llvm/Support/Dwarf.h>
]],[[
  unsigned dbg = llvm::LLVMDebugVersion;
]])],
        [AC_MSG_RESULT([yes])
         AC_DEFINE([HAVE_LLVM_LLVMDEBUGVERSION],[1],
         [Define if llvm::LLVMDebugVersion is defined.])],
        [AC_MSG_RESULT([no])])

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

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([atexit memset setenv])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
  Makefile
  src/Makefile
  doc/Makefile
])
AC_OUTPUT
