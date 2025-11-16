
#include "LLVMUtils.h"

#include <llvm/IR/DerivedTypes.h>

namespace {
#if LLVM_VERSION_MAJOR > 14
  template <typename pthread_t_type = pthread_t>
  struct getLLVMType {};

  template<typename T>
  struct getLLVMType<T*> {
    llvm::Type* operator()(llvm::LLVMContext &C) {
      return llvm::PointerType::get(C, 0);
    }
  };

  template<>
  struct getLLVMType<std::uint32_t> {
    llvm::Type *operator()(llvm::LLVMContext &C) {
      return llvm::IntegerType::get(C, 32);
    }
  };

  template<>
  struct getLLVMType<std::uint64_t> {
    llvm::Type *operator()(llvm::LLVMContext &C) {
      return llvm::IntegerType::get(C, 64);
    }
  };
#endif
}  // namespace

namespace LLVMUtils {
  llvm::Type* getPthreadTType(llvm::PointerType *PthreadTPtr) {
#if LLVM_VERSION_MAJOR >= 16  // only opaque pointers exist
    assert(PthreadTPtr->isOpaque());  // just for sanity...
    return getLLVMType<pthread_t>()(PthreadTPtr->getContext());
#else
#if LLVM_VERSION_MAJOR == 15  // this version supports both kinds
    if (PthreadTPtr->isOpaque())
      return getLLVMType<pthread_t>()(PthreadTPtr->getContext());
    else
#endif
      return PthreadTPtr->getPointerElementType();
#endif
  }
}  // namespace LLVMUtils
