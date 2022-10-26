
#include "LLVMUtils.h"

#if defined(HAVE_LLVM_IR_DERIVEDTYPES_H)
#include <llvm/IR/DerivedTypes.h>
#elif defined(HAVE_LLVM_DERIVEDTYPES_H)
#include <llvm/DerivedTypes.h>
#endif

namespace {
#if LLVM_VERSION_MAJOR > 14
  template <typename pthread_t_type = pthread_t>
  struct getLLVMType {};

  template<typename T>
  struct getLLVMType<T*> {
    llvm::Type* operator()(llvm::LLVMContext &C) {
      return llvm::PointerType::get(C, 0);
    };
  };

  template<>
  struct getLLVMType<std::uint32_t> {
    llvm::Type *operator()(llvm::LLVMContext &C) {
      return llvm::IntegerType::get(C, 32);
    };
  };

  template<>
  struct getLLVMType<std::uint64_t> {
    llvm::Type *operator()(llvm::LLVMContext &C) {
      return llvm::IntegerType::get(C, 64);
    };
  };
#endif
}

namespace LLVMUtils {
  llvm::Type* getPthreadTType(llvm::PointerType *PthreadTPtr) {
#if LLVM_VERSION_MAJOR > 14
    if (PthreadTPtr->isOpaque())
      return getLLVMType<pthread_t>()(PthreadTPtr->getContext());
    else
#endif
      return PthreadTPtr->getPointerElementType();
  }
}
