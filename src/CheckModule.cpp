/* Copyright (C) 2014-2017 Carl Leonardsson
 *
 * This file is part of Nidhugg.
 *
 * Nidhugg is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nidhugg is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "CheckModule.h"
#include "Debug.h"

#if defined(HAVE_LLVM_IR_LLVMCONTEXT_H)
#include <llvm/IR/LLVMContext.h>
#elif defined(HAVE_LLVM_LLVMCONTEXT_H)
#include <llvm/LLVMContext.h>
#endif
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/StringSet.h>

#include <set>

void CheckModule::check_functions(const llvm::Module *M){
  check_pthread_create(M);
  check_pthread_join(M);
  check_pthread_self(M);
  check_pthread_exit(M);
  check_pthread_mutex_init(M);
  check_pthread_mutex_lock(M);
  check_pthread_mutex_trylock(M);
  check_pthread_mutex_unlock(M);
  check_pthread_mutex_destroy(M);
  check_pthread_cond_init(M);
  check_pthread_cond_signal(M);
  check_pthread_cond_broadcast(M);
  check_pthread_cond_wait(M);
  check_pthread_cond_destroy(M);
  check_malloc(M);
  check_calloc(M);
  check_nondet_int(M);
  check_assume(M);
  llvm::StringSet<> supported =
    {"pthread_create",
     "pthread_join",
     "pthread_self",
     "pthread_exit",
     "pthread_mutex_init",
     "pthread_mutex_lock",
     "pthread_mutex_trylock",
     "pthread_mutex_unlock",
     "pthread_mutex_destroy",
     "pthread_cond_init",
     "pthread_cond_signal",
     "pthread_cond_broadcast",
     "pthread_cond_wait",
     "pthread_cond_destroy"};
  for(auto it = M->getFunctionList().begin(); it != M->getFunctionList().end(); ++it){
    if(it->getName().startswith("pthread_") &&
       supported.count(it->getName()) == 0){
      Debug::warn("CheckModule:"+it->getName().str())
        << "WARNING: Unsupported pthread function: " << it->getName() << "\n";
    }
  }
}

static bool check_pthread_t(const llvm::Type *ty) {
  return ty->isIntegerTy() || ty->isPointerTy();
}

void CheckModule::check_pthread_create(const llvm::Module *M){
  std::string _err;
  llvm::raw_string_ostream err(_err);
  llvm::Function *pthread_create = M->getFunction("pthread_create");
  if(pthread_create){
    if(!pthread_create->getReturnType()){
      err << "pthread_create returns non-integer type: "
          << *pthread_create->getReturnType();
      throw CheckModuleError(err.str());
    }
    if(pthread_create->arg_size() != 4){
      err << "pthread_create takes wrong number of arguments ("
          << pthread_create->arg_size() << ")";
      throw CheckModuleError(err.str());
    }
    if(!pthread_create->arg_begin()->getType()->isPointerTy()){
      err << "First argument of pthread_create is non-pointer type: "
          << *pthread_create->arg_begin()->getType();
      throw CheckModuleError(err.str());
    }
    llvm::Type *ty0e = static_cast<llvm::PointerType*>(pthread_create->arg_begin()->getType())->getElementType();
    if(!check_pthread_t(ty0e)){
      err << "First argument of pthread_create is pointer to invalid pthread_t type: "
          << *ty0e;
      throw CheckModuleError(err.str());
    }
    llvm::Type *vpty = llvm::Type::getInt8PtrTy(M->getContext());
    llvm::Type *fty = llvm::FunctionType::get(vpty,{vpty},false)->getPointerTo();
    llvm::Type *arg2_ty, *arg3_ty;
    {
      auto it = pthread_create->arg_begin();
      arg2_ty = (++++it)->getType();
      arg3_ty = (++it)->getType();
    }
    if(arg2_ty != fty){
      err << "Third argument of pthread_create has wrong type: " << *arg2_ty
          << ", should be " << *fty;
      throw CheckModuleError(err.str());
    }
    if(arg3_ty != vpty){
      err << "Fourth argument of pthread_create has wrong type: " << *arg3_ty
          << ", should be " << *vpty;
      throw CheckModuleError(err.str());
    }
  }
}

void CheckModule::check_pthread_join(const llvm::Module *M){
  std::string _err;
  llvm::raw_string_ostream err(_err);
  llvm::Function *pthread_join = M->getFunction("pthread_join");
  if(pthread_join){
    if(!pthread_join->getReturnType()->isIntegerTy()){
      err << "pthread_join returns non-integer type: "
          << *pthread_join->getReturnType();
      throw CheckModuleError(err.str());
    }
    if(pthread_join->arg_size() != 2){
      err << "pthread_join takes wrong number of arguments ("
          << pthread_join->arg_size() << ")";
      throw CheckModuleError(err.str());
    }
    llvm::Type *arg0_ty, *arg1_ty;
    {
      auto it = pthread_join->arg_begin();
      arg0_ty = it->getType();
      arg1_ty = (++it)->getType();
    }
    if(!check_pthread_t(arg0_ty)){
      err << "First argument of pthread_join is invalid pthread_t type: " << *arg0_ty;
      throw CheckModuleError(err.str());
    }
    llvm::Type *arg1_ty_expected =
      llvm::Type::getInt8PtrTy(M->getContext())->getPointerTo();
    if(arg1_ty != arg1_ty_expected){
      err << "Second argument of pthread_join has wrong type: "
          << *arg1_ty << ", should be " << *arg1_ty_expected;
      throw CheckModuleError(err.str());
    }
  }
}

void CheckModule::check_pthread_self(const llvm::Module *M){
  std::string _err;
  llvm::raw_string_ostream err(_err);
  llvm::Function *pthread_self = M->getFunction("pthread_self");
  if(pthread_self){
    if(!check_pthread_t(pthread_self->getReturnType())){
      err << "pthread_self returns invalid pthread_t type: "
          << *pthread_self->getReturnType();
      throw CheckModuleError(err.str());
    }
    if(pthread_self->arg_size()){
      err << "pthread_self takes arguments. Should not take any.";
      throw CheckModuleError(err.str());
    }
  }
}


void CheckModule::check_pthread_exit(const llvm::Module *M){
  std::string _err;
  llvm::raw_string_ostream err(_err);
  llvm::Function *pthread_exit = M->getFunction("pthread_exit");
  if(pthread_exit){
    if(!pthread_exit->getReturnType()->isVoidTy()){
      err << "pthread_exit returns non-void type: "
          << *pthread_exit->getReturnType();
      throw CheckModuleError(err.str());
    }
    if(pthread_exit->arg_size() != 1){
      err << "pthread_exit takes wrong number of arguments ("
          << pthread_exit->arg_size() << ")";
      throw CheckModuleError(err.str());
    }
    llvm::Type *ty = pthread_exit->arg_begin()->getType(),
      *ty_expected = llvm::Type::getInt8PtrTy(M->getContext());
    if(ty != ty_expected){
      err << "Argument of pthread_exit has wrong type: "
          << *ty << ", should be " << *ty_expected;
      throw CheckModuleError(err.str());
    }
  }
}

void CheckModule::check_pthread_mutex_init(const llvm::Module *M){
  std::string _err;
  llvm::raw_string_ostream err(_err);
  llvm::Function *F = M->getFunction("pthread_mutex_init");
  if(F){
    if(!F->getReturnType()->isIntegerTy()){
      err << "pthread_mutex_init returns non-integer type: "
          << *F->getReturnType();
      throw CheckModuleError(err.str());
    }
    if(F->arg_size() != 2){
      err << "pthread_mutex_init takes wrong number of arguments ("
          << F->arg_size() << ")";
      throw CheckModuleError(err.str());
    }
    llvm::Type *arg0ty, *arg1ty;
    {
      auto it = F->arg_begin();
      arg0ty = it->getType();
      arg1ty = (++it)->getType();
    }
    if(!arg0ty->isPointerTy()){
      err << "First argument of pthread_mutex_init has non-pointer type: "
          << *arg0ty;
      throw CheckModuleError(err.str());
    }
    if(!arg1ty->isPointerTy()){
      err << "Second argument of pthread_mutex_init has non-pointer type: "
          << *arg1ty;
      throw CheckModuleError(err.str());
    }
  }
}

void CheckModule::check_pthread_mutex_lock(const llvm::Module *M){
  std::string _err;
  llvm::raw_string_ostream err(_err);
  llvm::Function *F = M->getFunction("pthread_mutex_lock");
  if(F){
    if(!F->getReturnType()->isIntegerTy()){
      err << "pthread_mutex_lock returns non-integer type: "
          << *F->getReturnType();
      throw CheckModuleError(err.str());
    }
    if(F->arg_size() != 1){
      err << "pthread_mutex_lock takes wrong number of arguments ("
          << F->arg_size() << ")";
      throw CheckModuleError(err.str());
    }
    llvm::Type *arg0ty = F->arg_begin()->getType();
    if(!arg0ty->isPointerTy()){
      err << "First argument of pthread_mutex_lock has non-pointer type: "
          << *arg0ty;
      throw CheckModuleError(err.str());
    }
  }
}

void CheckModule::check_pthread_mutex_trylock(const llvm::Module *M){
  std::string _err;
  llvm::raw_string_ostream err(_err);
  llvm::Function *F = M->getFunction("pthread_mutex_trylock");
  if(F){
    if(!F->getReturnType()->isIntegerTy()){
      err << "pthread_mutex_trylock returns non-integer type: "
          << *F->getReturnType();
      throw CheckModuleError(err.str());
    }
    if(F->arg_size() != 1){
      err << "pthread_mutex_trylock takes wrong number of arguments ("
          << F->arg_size() << ")";
      throw CheckModuleError(err.str());
    }
    llvm::Type *arg0ty = F->arg_begin()->getType();
    if(!arg0ty->isPointerTy()){
      err << "First argument of pthread_mutex_trylock has non-pointer type: "
          << *arg0ty;
      throw CheckModuleError(err.str());
    }
  }
}

void CheckModule::check_pthread_mutex_unlock(const llvm::Module *M){
  std::string _err;
  llvm::raw_string_ostream err(_err);
  llvm::Function *F = M->getFunction("pthread_mutex_unlock");
  if(F){
    if(!F->getReturnType()->isIntegerTy()){
      err << "pthread_mutex_unlock returns non-integer type: "
          << *F->getReturnType();
      throw CheckModuleError(err.str());
    }
    if(F->arg_size() != 1){
      err << "pthread_mutex_unlock takes wrong number of arguments ("
          << F->arg_size() << ")";
      throw CheckModuleError(err.str());
    }
    llvm::Type *arg0ty = F->arg_begin()->getType();
    if(!arg0ty->isPointerTy()){
      err << "First argument of pthread_mutex_unlock has non-pointer type: "
          << *arg0ty;
      throw CheckModuleError(err.str());
    }
  }
}

void CheckModule::check_pthread_mutex_destroy(const llvm::Module *M){
  std::string _err;
  llvm::raw_string_ostream err(_err);
  llvm::Function *F = M->getFunction("pthread_mutex_destroy");
  if(F){
    if(!F->getReturnType()->isIntegerTy()){
      err << "pthread_mutex_destroy returns non-integer type: "
          << *F->getReturnType();
      throw CheckModuleError(err.str());
    }
    if(F->arg_size() != 1){
      err << "pthread_mutex_destroy takes wrong number of arguments ("
          << F->arg_size() << ")";
      throw CheckModuleError(err.str());
    }
    llvm::Type *arg0ty = F->arg_begin()->getType();
    if(!arg0ty->isPointerTy()){
      err << "First argument of pthread_mutex_destroy has non-pointer type: "
          << *arg0ty;
      throw CheckModuleError(err.str());
    }
  }
}

void CheckModule::check_malloc(const llvm::Module *M){
  std::string _err;
  llvm::raw_string_ostream err(_err);
  llvm::Function *F = M->getFunction("malloc");
  if(F){
    if(!F->getReturnType()->isPointerTy()){
      err << "malloc returns non-pointer type: "
          << *F->getReturnType();
      throw CheckModuleError(err.str());
    }
    if(F->arg_size() != 1){
      err << "malloc takes wrong number of arguments ("
          << F->arg_size() << ")";
      throw CheckModuleError(err.str());
    }
    llvm::Type *arg0ty = F->arg_begin()->getType();
    if(!arg0ty->isIntegerTy()){
      err << "Argument of malloc has non-integer type: "
          << *arg0ty;
      throw CheckModuleError(err.str());
    }
  }
}

void CheckModule::check_calloc(const llvm::Module *M){
  std::string _err;
  llvm::raw_string_ostream err(_err);
  llvm::Function *F = M->getFunction("calloc");
  if(F){
    if(!F->getReturnType()->isPointerTy()){
      err << "calloc returns non-pointer type: "
          << *F->getReturnType();
      throw CheckModuleError(err.str());
    }
    if(F->arg_size() != 2){
      err << "calloc takes wrong number of arguments ("
          << F->arg_size() << ")";
      throw CheckModuleError(err.str());
    }
    llvm::Type *arg0ty, *arg1ty;
    {
      auto it = F->arg_begin();
      arg0ty = it->getType();
      arg1ty = (++it)->getType();
    }
    if(!arg0ty->isIntegerTy()){
      err << "First argument of calloc has non-integer type: "
          << *arg0ty;
      throw CheckModuleError(err.str());
    }
    if(!arg1ty->isIntegerTy()){
      err << "Second argument of calloc has non-integer type: "
          << *arg1ty;
      throw CheckModuleError(err.str());
    }
  }
}

namespace CheckModule {

  static void check_nondet_int(const llvm::Module *M, const std::string &name){
    std::string _err;
    llvm::raw_string_ostream err(_err);
    llvm::Function *F = M->getFunction(name);
    if(F){
      if(!F->getReturnType()->isIntegerTy()){
        err << name << " returns non-integer type: "
            << *F->getReturnType();
        throw CheckModuleError(err.str());
      }
    }
  }

  static void check_assume(const llvm::Module *M, const std::string &name){
    std::string _err;
    llvm::raw_string_ostream err(_err);
    llvm::Function *F = M->getFunction(name);
    if(F){
      if(!F->getReturnType()->isVoidTy()){
        err << name << " has non-void return type: "
            << *F->getReturnType();
        throw CheckModuleError(err.str());
      }
      if(F->arg_size() != 1){
        err << name << " takes wrong number of arguments ("
            << F->arg_size() << ")";
        throw CheckModuleError(err.str());
      }
      if(!F->arg_begin()->getType()->isIntegerTy()){
        err << "First argument of " << name << " has non-integer type: "
            << *F->arg_begin()->getType();
        throw CheckModuleError(err.str());
      }
    }
  }

}

void CheckModule::check_nondet_int(const llvm::Module *M){
  check_nondet_int(M,"__VERIFIER_nondet_int");
  check_nondet_int(M,"__VERIFIER_nondet_uint");
}

void CheckModule::check_assume(const llvm::Module *M){
  check_assume(M,"__VERIFIER_assume");
}

void CheckModule::check_pthread_cond_init(const llvm::Module *M){
  std::string _err;
  llvm::raw_string_ostream err(_err);
  llvm::Function *F = M->getFunction("pthread_cond_init");
  if(F){
    if(!F->getReturnType()->isIntegerTy()){
      err << "pthread_cond_init returns non-integer type: "
          << *F->getReturnType();
      throw CheckModuleError(err.str());
    }
    if(F->arg_size() != 2){
      err << "pthread_cond_init takes wrong number of arguments ("
          << F->arg_size() << ")";
      throw CheckModuleError(err.str());
    }
    llvm::Type *arg0ty, *arg1ty;
    {
      auto it = F->arg_begin();
      arg0ty = it->getType();
      arg1ty = (++it)->getType();
    }
    if(!arg0ty->isPointerTy()){
      err << "First argument of pthread_cond_init has non-pointer type: "
          << *arg0ty;
      throw CheckModuleError(err.str());
    }
    if(!arg1ty->isPointerTy()){
      err << "Second argument of pthread_cond_init has non-pointer type: "
          << *arg1ty;
      throw CheckModuleError(err.str());
    }
  }
}

void CheckModule::check_pthread_cond_signal(const llvm::Module *M){
  std::string _err;
  llvm::raw_string_ostream err(_err);
  llvm::Function *F = M->getFunction("pthread_cond_signal");
  if(F){
    if(!F->getReturnType()->isIntegerTy()){
      err << "pthread_cond_signal returns non-integer type: "
          << *F->getReturnType();
      throw CheckModuleError(err.str());
    }
    if(F->arg_size() != 1){
      err << "pthread_cond_signal takes wrong number of arguments ("
          << F->arg_size() << ")";
      throw CheckModuleError(err.str());
    }
    llvm::Type *arg0ty = F->arg_begin()->getType();
    if(!arg0ty->isPointerTy()){
      err << "First argument of pthread_cond_signal has non-pointer type: "
          << *arg0ty;
      throw CheckModuleError(err.str());
    }
  }
}

void CheckModule::check_pthread_cond_broadcast(const llvm::Module *M){
  std::string _err;
  llvm::raw_string_ostream err(_err);
  llvm::Function *F = M->getFunction("pthread_cond_broadcast");
  if(F){
    if(!F->getReturnType()->isIntegerTy()){
      err << "pthread_cond_broadcast returns non-integer type: "
          << *F->getReturnType();
      throw CheckModuleError(err.str());
    }
    if(F->arg_size() != 1){
      err << "pthread_cond_broadcast takes wrong number of arguments ("
          << F->arg_size() << ")";
      throw CheckModuleError(err.str());
    }
    llvm::Type *arg0ty = F->arg_begin()->getType();
    if(!arg0ty->isPointerTy()){
      err << "First argument of pthread_cond_broadcast has non-pointer type: "
          << *arg0ty;
      throw CheckModuleError(err.str());
    }
  }
}

void CheckModule::check_pthread_cond_wait(const llvm::Module *M){
  std::string _err;
  llvm::raw_string_ostream err(_err);
  llvm::Function *F = M->getFunction("pthread_cond_wait");
  if(F){
    if(!F->getReturnType()->isIntegerTy()){
      err << "pthread_cond_wait returns non-integer type: "
          << *F->getReturnType();
      throw CheckModuleError(err.str());
    }
    if(F->arg_size() != 2){
      err << "pthread_cond_wait takes wrong number of arguments ("
          << F->arg_size() << ")";
      throw CheckModuleError(err.str());
    }
    llvm::Type *arg0ty, *arg1ty;
    {
      auto it = F->arg_begin();
      arg0ty = it->getType();
      arg1ty = (++it)->getType();
    }
    if(!arg0ty->isPointerTy()){
      err << "First argument of pthread_cond_wait has non-pointer type: "
          << *arg0ty;
      throw CheckModuleError(err.str());
    }
    if(!arg1ty->isPointerTy()){
      err << "Second argument of pthread_cond_wait has non-pointer type: "
          << *arg1ty;
      throw CheckModuleError(err.str());
    }
  }
}

void CheckModule::check_pthread_cond_destroy(const llvm::Module *M){
  std::string _err;
  llvm::raw_string_ostream err(_err);
  llvm::Function *F = M->getFunction("pthread_cond_destroy");
  if(F){
    if(!F->getReturnType()->isIntegerTy()){
      err << "pthread_cond_destroy returns non-integer type: "
          << *F->getReturnType();
      throw CheckModuleError(err.str());
    }
    if(F->arg_size() != 1){
      err << "pthread_cond_destroy takes wrong number of arguments ("
          << F->arg_size() << ")";
      throw CheckModuleError(err.str());
    }
    llvm::Type *arg0ty = F->arg_begin()->getType();
    if(!arg0ty->isPointerTy()){
      err << "First argument of pthread_cond_destroy has non-pointer type: "
          << *arg0ty;
      throw CheckModuleError(err.str());
    }
  }
}

