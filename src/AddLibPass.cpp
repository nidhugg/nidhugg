/* Copyright (C) 2014 Carl Leonardsson
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

#include "AddLibPass.h"
#include "Debug.h"
#include "StrModule.h"

#if defined(HAVE_LLVM_IR_CONSTANTS_H)
#include <llvm/IR/Constants.h>
#elif defined(HAVE_LLVM_CONSTANTS_H)
#include <llvm/Constants.h>
#endif
#if defined(HAVE_LLVM_IR_INSTRUCTIONS_H)
#include <llvm/IR/Instructions.h>
#elif defined(HAVE_LLVM_INSTRUCTIONS_H)
#include <llvm/Instructions.h>
#endif
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>
#if defined(HAVE_LLVM_LINKER_H)
#include <llvm/Linker.h>
#elif defined(HAVE_LLVM_LINKER_LINKER_H)
#include <llvm/Linker/Linker.h>
#endif

#include <stdexcept>

void AddLibPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const{
};

bool AddLibPass::optAddFunction(llvm::Module &M,
                                std::string name,
                                const std::vector<std::string> &srces){
  llvm::Type *tgtTy = 0; // The desired type of the function, (or null if unknown)
  {
    llvm::Function *F = M.getFunction(name);
    if(F){
      tgtTy = F->getType();
    }
    if(F && !F->isDeclaration()){
      // Already defined
      return false;
    }
  }
  llvm::Linker lnk(&M);
  std::string err;
  bool added_def = false;
  for(auto it = srces.begin(); !added_def && it != srces.end(); ++it){
    std::string src = *it;
    llvm::Module *M2 = StrModule::read_module_src(src);

    if(!tgtTy || M2->getFunction(name)->getType() == tgtTy){
#ifdef LLVM_LINKER_LINKINMODULE_HAS_MODE
      if(lnk.linkInModule(M2,llvm::Linker::DestroySource,&err)){
#else
      if(lnk.linkInModule(M2)){
#endif
        delete M2;
        throw std::logic_error("Failed to link in library code: "+err);
      }
      added_def = true;
    }
    delete M2;
  }
  if(!added_def){
    Debug::warn("AddLibPass:"+name)
      << "WARNING: Failed to add library function definition for function " << name << "\n";
  }
  return added_def;
};

bool AddLibPass::optNopFunction(llvm::Module &M,
                                std::string name){
  llvm::Function *F = M.getFunction(name);
  if(!F || !F->isDeclaration()) return false;
  llvm::Value *rv = 0;
  llvm::Type *retTy = F->getReturnType();
  if(retTy->isIntegerTy()){
    rv = llvm::ConstantInt::get(retTy,0);
  }else if(retTy->isVoidTy()){
    rv = 0;
  }else if(retTy->isPointerTy()){
    rv = llvm::ConstantPointerNull::get(static_cast<llvm::PointerType*>(retTy));
  }else{
    Debug::warn("AddLibPass::optNopFunction:"+name)
      << "WARNING: Failed to add library function (nop) definition for function "
      << name << "\n";
    return false;
  }
  llvm::BasicBlock *B =
    llvm::BasicBlock::Create(F->getContext(),"",F);
  llvm::ReturnInst::Create(F->getContext(),rv,B);
  return true;
};

bool AddLibPass::runOnModule(llvm::Module &M){
  optNopFunction(M,"fflush");
  optNopFunction(M,"fprintf");
  /**********************************
   *           memset               *
   **********************************/
  optAddFunction(M,"memset",{R"(
define i8* @memset(i8* %s, i32 %_c, i64 %_n){
entry:
  %c = trunc i32 %_c to i8
  br label %head
head:
  %n = phi i64 [%_n,%entry], [%nnext, %body]
  %ncmp = icmp sgt i64 %n, 0
  br i1 %ncmp, label %body, label %exit
body:
  %nnext = sub i64 %n, 1
  %scur = getelementptr i8* %s, i64 %nnext
  store i8 %c, i8* %scur
  br label %head
exit:
  ret i8* %s
}
)",R"(
define i8* @memset(i8* %s, i32 %_c, i32 %_n){
entry:
  %c = trunc i32 %_c to i8
  br label %head
head:
  %n = phi i32 [%_n,%entry], [%nnext, %body]
  %ncmp = icmp sgt i32 %n, 0
  br i1 %ncmp, label %body, label %exit
body:
  %nnext = sub i32 %n, 1
  %scur = getelementptr i8* %s, i32 %nnext
  store i8 %c, i8* %scur
  br label %head
exit:
  ret i8* %s
}
)",R"(
define i8* @memset(i8* %s, i8 %c, i64 %_n){
entry:
  br label %head
head:
  %n = phi i64 [%_n,%entry], [%nnext, %body]
  %ncmp = icmp sgt i64 %n, 0
  br i1 %ncmp, label %body, label %exit
body:
  %nnext = sub i64 %n, 1
  %scur = getelementptr i8* %s, i64 %nnext
  store i8 %c, i8* %scur
  br label %head
exit:
  ret i8* %s
}
)",R"(
define i8* @memset(i8* %s, i8 %c, i32 %_n){
entry:
  br label %head
head:
  %n = phi i32 [%_n,%entry], [%nnext, %body]
  %ncmp = icmp sgt i32 %n, 0
  br i1 %ncmp, label %body, label %exit
body:
  %nnext = sub i32 %n, 1
  %scur = getelementptr i8* %s, i32 %nnext
  store i8 %c, i8* %scur
  br label %head
exit:
  ret i8* %s
}
)"});
  return true;
};

char AddLibPass::ID = 0;
static llvm::RegisterPass<AddLibPass> X("add-lib",
                                        "Add definitions for some library functions.");

