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
}

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
#ifdef LLVM_LINKER_CTOR_PARAM_MODULE_REFERENCE
  llvm::Linker lnk(M);
#else
  llvm::Linker lnk(&M);
#endif
  std::string err;
  bool added_def = false;
  for(auto it = srces.begin(); !added_def && it != srces.end(); ++it){
    std::string src = StrModule::portasm(*it);
    llvm::Module *M2 = StrModule::read_module_src(src);

    if(!tgtTy || M2->getFunction(name)->getType() == tgtTy){
#ifdef LLVM_LINKER_LINKINMODULE_PTR_BOOL
      if(lnk.linkInModule(M2)){
#elif defined LLVM_LINKER_LINKINMODULE_UNIQUEPTR_BOOL
      if(lnk.linkInModule(std::unique_ptr<llvm::Module>(M2))){
#else
      if(lnk.linkInModule(M2,llvm::Linker::DestroySource,&err)){
#endif
#ifndef LLVM_LINKER_LINKINMODULE_UNIQUEPTR_BOOL
        delete M2;
#endif
        throw std::logic_error("Failed to link in library code: "+err);
      }
      added_def = true;
    }
#ifndef LLVM_LINKER_LINKINMODULE_UNIQUEPTR_BOOL
    delete M2;
#endif
  }
  if(!added_def){
    Debug::warn("AddLibPass:"+name)
      << "WARNING: Failed to add library function definition for function " << name << "\n";
  }
  return added_def;
}

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
}

bool AddLibPass::optConstIntFunction(llvm::Module &M,
                                     std::string name,
                                     int val){
  llvm::Function *F = M.getFunction(name);
  if(!F || !F->isDeclaration()) return false;
  llvm::Type *retTy = F->getReturnType();
  if(!retTy->isIntegerTy()){
    Debug::warn("AddLibPass::optConstIntFunction:"+name)
      << "WARNING: Failed to add library function (nop) definition for function "
      << name << "\n";
    return false;
  }
  llvm::Value *rv = llvm::ConstantInt::get(retTy,val);
  llvm::BasicBlock *B =
    llvm::BasicBlock::Create(F->getContext(),"",F);
  llvm::ReturnInst::Create(F->getContext(),rv,B);
  return true;
}

bool AddLibPass::runOnModule(llvm::Module &M){
  optNopFunction(M,"fclose");
  optNopFunction(M,"fflush");
  optNopFunction(M,"fopen");
  optNopFunction(M,"fprintf");
  optConstIntFunction(M,"sched_setaffinity",-1);
  /**********************************
   *           calloc               *
   **********************************/
  {
    llvm::Function *F = M.getFunction("calloc");
    if(F && F->isDeclaration()){
      /* Figure out types for arguments of calloc (declaration) and
       * malloc.
       */
      if(F->getArgumentList().size() != 2){
        throw std::logic_error("Unable to add definition of calloc. Wrong signature.");
      }
      std::string arg0ty, arg1ty, malloc_argty, malloc_declaration;
      llvm::raw_string_ostream arg0tys(arg0ty), arg1tys(arg1ty),
        malloc_argtys(malloc_argty);
      auto it = F->arg_begin();
      arg0tys << *it->getType();
      ++it;
      arg1tys << *it->getType();
      arg0tys.flush();
      arg1tys.flush();
      llvm::Function *F_malloc = M.getFunction("malloc");
      if(F_malloc){
        if(F_malloc->getArgumentList().size() != 1){
          throw std::logic_error("Unable to add definition of calloc. malloc has the wrong signature.");
        }
        malloc_argtys << *F_malloc->arg_begin()->getType();
        malloc_argtys.flush();
        malloc_declaration="declare i8* @malloc("+malloc_argty+")\n";
      }else{
        malloc_declaration="declare i8* @malloc(i64)\n";
        malloc_argty="i64";
      }
      if(!(arg0ty == "i32" || arg0ty == "i64") ||
         !(arg1ty == "i32" || arg1ty == "i64")){
        throw std::logic_error("Unable to add definition of calloc. Wrong signature.");
      }
      if(!(malloc_argty == "i32" || malloc_argty == "i64")){
        throw std::logic_error("Unable to add definition of calloc. malloc has the wrong signature.");
      }
      /* Define calloc */
      optAddFunction(M,"calloc",{R"(
define i8* @calloc()"+arg0ty+R"( %_nmemb, )"+arg1ty+R"( %_size){
entry:
  %nmemb = trunc )"+arg0ty+R"( %_nmemb to i32
  %size = trunc )"+arg1ty+R"( %_size to i32
  %sz = mul i32 %nmemb, %size
  %sz_m = zext i32 %sz to )"+malloc_argty+R"(
  %m = call i8* @malloc()"+malloc_argty+R"( %sz_m)
  br label %head
head:
  %n = phi i32 [%sz,%entry], [%nnext, %body]
  %ncmp = icmp sgt i32 %n, 0
  br i1 %ncmp, label %body, label %exit
body:
  %nnext = sub i32 %n, 1
  %scur = getelementptr i8, i8* %m, i32 %nnext
  store i8 0, i8* %scur
  br label %head
exit:
  ret i8* %m
}
)"+malloc_declaration});
    }
  }
  /**********************************
   *           strcmp               *
   **********************************/
  optAddFunction(M,"strcmp",{R"(
define i32 @strcmp(i8* %p1, i8* %p2) nounwind readonly uwtable {
entry:
  br label %head

head:
  %s1 = phi i8* [ %p1, %entry ], [ %s1next, %body ]
  %s2 = phi i8* [ %p2, %entry ], [ %s2next, %body ]
  %a = load i8, i8* %s1, align 1
  %b = load i8, i8* %s2, align 1
  %a0 = icmp eq i8 %a, 0
  br i1 %a0, label %exit, label %body

body:
  %s1next = getelementptr inbounds i8, i8* %s1, i64 1
  %s2next = getelementptr inbounds i8, i8* %s2, i64 1
  %abeq = icmp eq i8 %a, %b
  br i1 %abeq, label %head, label %exit

exit:
  %a32 = zext i8 %a to i32
  %b32 = zext i8 %b to i32
  %rv = sub nsw i32 %a32, %b32
  ret i32 %rv
}
)",R"(
define i64 @strcmp(i8* %p1, i8* %p2) nounwind readonly uwtable {
entry:
  br label %head

head:
  %s1 = phi i8* [ %p1, %entry ], [ %s1next, %body ]
  %s2 = phi i8* [ %p2, %entry ], [ %s2next, %body ]
  %a = load i8, i8* %s1, align 1
  %b = load i8, i8* %s2, align 1
  %a0 = icmp eq i8 %a, 0
  br i1 %a0, label %exit, label %body

body:
  %s1next = getelementptr inbounds i8, i8* %s1, i64 1
  %s2next = getelementptr inbounds i8, i8* %s2, i64 1
  %abeq = icmp eq i8 %a, %b
  br i1 %abeq, label %head, label %exit

exit:
  %a64 = zext i8 %a to i64
  %b64 = zext i8 %b to i64
  %rv = sub nsw i64 %a64, %b64
  ret i64 %rv
}
)"
    });
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
  %scur = getelementptr i8, i8* %s, i64 %nnext
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
  %scur = getelementptr i8, i8* %s, i32 %nnext
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
  %scur = getelementptr i8, i8* %s, i64 %nnext
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
  %scur = getelementptr i8, i8* %s, i32 %nnext
  store i8 %c, i8* %scur
  br label %head
exit:
  ret i8* %s
}
)"});
  /************************
   *        puts          *
   ************************/
  /* Figure out signature of putchar */
  std::string putchar_ret_ty = "i32";
  std::string putchar_arg_ty = "i32";
  std::string putchar_decl = "declare i32 @putchar(i32)";
  std::string print_newline =
    "call "+putchar_ret_ty+" @putchar("+putchar_arg_ty+" 10)";
  /* Define puts */
  optAddFunction(M,"puts",{
      R"(
define i32 @puts(i8* %s){
entry:
  br label %head
head:
  %i = phi i32 [0, %entry], [%inext, %body]
  %si = getelementptr i8, i8* %s, i32 %i
  %c = load i8, i8* %si
  %cc = icmp eq i8 %c, 0
  br i1 %cc, label %exit, label %body
body:
  %ca = zext i8 %c to )"+putchar_arg_ty+R"(
  call )"+putchar_ret_ty+R"( @putchar()"+putchar_arg_ty+R"( %ca)
  %inext = add i32 %i, 1
  br label %head
exit:
  )"+print_newline+R"(
  ret i32 1
}
)"+putchar_decl+"\n",
      R"(
define i64 @puts(i8* %s){
entry:
  br label %head
head:
  %i = phi i32 [0, %entry], [%inext, %body]
  %si = getelementptr i8, i8* %s, i32 %i
  %c = load i8, i8* %si
  %cc = icmp eq i8 %c, 0
  br i1 %cc, label %exit, label %body
body:
  %ca = zext i8 %c to )"+putchar_arg_ty+R"(
  call )"+putchar_ret_ty+R"( @putchar()"+putchar_arg_ty+R"( %ca)
  %inext = add i32 %i, 1
  br label %head
exit:
  )"+print_newline+R"(
  ret i64 1
}
)"+putchar_decl+"\n"});
  return true;
}

char AddLibPass::ID = 0;
static llvm::RegisterPass<AddLibPass> X("add-lib",
                                        "Add definitions for some library functions.");

