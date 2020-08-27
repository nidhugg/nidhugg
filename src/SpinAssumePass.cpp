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

#include <config.h>

#include <llvm/Pass.h>
#include <llvm/Analysis/LoopPass.h>
#if defined(HAVE_LLVM_IR_DOMINATORS_H)
#include <llvm/IR/Dominators.h>
#elif defined(HAVE_LLVM_ANALYSIS_DOMINATORS_H)
#include <llvm/Analysis/Dominators.h>
#endif
#if defined(HAVE_LLVM_IR_FUNCTION_H)
#include <llvm/IR/Function.h>
#elif defined(HAVE_LLVM_FUNCTION_H)
#include <llvm/Function.h>
#endif
#if defined(HAVE_LLVM_IR_INSTRUCTIONS_H)
#include <llvm/IR/Instructions.h>
#elif defined(HAVE_LLVM_INSTRUCTIONS_H)
#include <llvm/Instructions.h>
#endif
#if defined(HAVE_LLVM_IR_LLVMCONTEXT_H)
#include <llvm/IR/LLVMContext.h>
#elif defined(HAVE_LLVM_LLVMCONTEXT_H)
#include <llvm/LLVMContext.h>
#endif
#if defined(HAVE_LLVM_IR_MODULE_H)
#include <llvm/IR/Module.h>
#elif defined(HAVE_LLVM_MODULE_H)
#include <llvm/Module.h>
#endif
#if defined(HAVE_LLVM_SUPPORT_CALLSITE_H)
#include <llvm/Support/CallSite.h>
#elif defined(HAVE_LLVM_IR_CALLSITE_H)
#include <llvm/IR/CallSite.h>
#endif
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

#include "CheckModule.h"
#include "SpinAssumePass.h"
#include "vecset.h"

#ifdef LLVM_HAS_ATTRIBUTELIST
typedef llvm::AttributeList AttributeList;
#else
typedef llvm::AttributeSet AttributeList;
#endif

#ifdef LLVM_HAS_TERMINATORINST
typedef llvm::TerminatorInst TerminatorInst;
#else
typedef llvm::Instruction TerminatorInst;
#endif

void SpinAssumePass::getAnalysisUsage(llvm::AnalysisUsage &AU) const{
  AU.addRequired<llvm::LLVM_DOMINATOR_TREE_PASS>();
  AU.addRequired<DeclareAssumePass>();
  AU.addPreserved<DeclareAssumePass>();
}

bool DeclareAssumePass::runOnModule(llvm::Module &M){
  bool modified_M = false;
  CheckModule::check_assume(&M);
  llvm::Function *F_assume = M.getFunction("__VERIFIER_assume");
  if(!F_assume){
    llvm::FunctionType *assumeTy;
    {
      llvm::Type *voidTy = llvm::Type::getVoidTy(M.getContext());
      llvm::Type *i1Ty = llvm::Type::getInt1Ty(M.getContext());
      assumeTy = llvm::FunctionType::get(voidTy,{i1Ty},false);
    }
    AttributeList assumeAttrs =
      AttributeList::get(M.getContext(),AttributeList::FunctionIndex,
                              std::vector<llvm::Attribute::AttrKind>({llvm::Attribute::NoUnwind}));
    M.getOrInsertFunction("__VERIFIER_assume",assumeTy,assumeAttrs);
    modified_M = true;
  }
  return modified_M;
}

bool SpinAssumePass::is_assume(llvm::Instruction &I) const {
  llvm::CallInst *C = llvm::dyn_cast<llvm::CallInst>(&I);
  if(!C) return false;
  llvm::Function *F = C->getCalledFunction();
  return F && F->getName().str() == "__VERIFIER_assume";
}

bool SpinAssumePass::is_spin(const llvm::Loop *l) const{
  for(auto B_it = l->block_begin(); B_it != l->block_end(); ++B_it){
    for(auto it = (*B_it)->begin(); it != (*B_it)->end(); ++it){
      if(it->mayHaveSideEffects() &&
         !llvm::isa<llvm::LoadInst>(*it) &&
         !is_assume(*it)){
        return false;
      }
      if(llvm::isa<llvm::AllocaInst>(*it)){
        return false;
      }
      if(llvm::isa<llvm::PHINode>(*it)){
        return false;
      }
    }
  }
  return true;
}

void SpinAssumePass::remove_disconnected(llvm::Loop *l){
  // Traverse l and all its ancestor loops
  while(l){
    bool done = false;
    // Iterate until no more basic blocks can be removed
    while(!done){
      done = true;
      VecSet<llvm::BasicBlock*> has_predecessor;
      // Search for basic blocks without in-loop successors
      // Simultaneously collect blocks with in-loop predecessors
      for(auto it = l->block_begin(); done && it != l->block_end(); ++it){
        TerminatorInst *T = (*it)->getTerminator();
        bool has_loop_successor = false;
        for(unsigned i = 0; i < T->getNumSuccessors(); ++i){
          if(l->contains(T->getSuccessor(i))){
            has_loop_successor = true;
            has_predecessor.insert(T->getSuccessor(i));
          }
        }
        if(!has_loop_successor){
          done = false;
          l->removeBlockFromLoop(*it);
        }
      }
      // Search for basic blocks without in-loop predecessors
      for(auto it = l->block_begin(); done && it != l->block_end(); ++it){
        if(has_predecessor.count(*it) == 0){
          done = false;
          l->removeBlockFromLoop(*it);
        }
      }
    }
    l = l->getParentLoop();
  }
}

bool SpinAssumePass::assumify_loop(llvm::Loop *l,llvm::LPPassManager &LPM){
  llvm::BasicBlock *EB = l->getExitingBlock();
  if(!EB) return false; // Too complicated loop
  llvm::BranchInst *BI;
  {
    TerminatorInst *EI = EB->getTerminator();
    assert(EI);
    BI = llvm::dyn_cast<llvm::BranchInst>(EI);
  }
  if(!BI) return false; // Unsupported loop
  if(!BI->isConditional()) return false; // Unsupported loop
  assert(BI->getNumSuccessors() == 2);
  llvm::BasicBlock *B_exit = l->getExitBlock();
  assert(B_exit);
  bool exit_on_val; // Exit on true condition, or false
  if(BI->getSuccessor(0) == B_exit){
    assert(l->contains(BI->getSuccessor(1)));
    exit_on_val = true;
  }else{
    assert(BI->getSuccessor(1) == B_exit);
    assert(l->contains(BI->getSuccessor(0)));
    exit_on_val = false;
  }
  llvm::Value *cond = BI->getCondition();
  llvm::MDNode *MD = BI->getMetadata("dbg");
  EB->getInstList().pop_back();
  if(!exit_on_val){
    // Negate the branch condition
    llvm::BinaryOperator *binop = llvm::BinaryOperator::CreateNot(cond,"notcond",EB);
    binop->setMetadata("dbg",MD);
    cond = binop;
  }
  F_assume = EB->getParent()->getParent()->getFunction("__VERIFIER_assume");
  {
    llvm::Type *arg_ty = F_assume->arg_begin()->getType();
    assert(arg_ty->isIntegerTy());
    if(arg_ty->getIntegerBitWidth() != 1){
      llvm::ZExtInst *I = new llvm::ZExtInst(cond,arg_ty,"",EB);
      I->setMetadata("dbg",MD);
      cond = I;
    }
  }
  llvm::Instruction *I = llvm::CallInst::Create(F_assume,{cond},"",EB);
  I->setMetadata("dbg",MD);
  I = llvm::BranchInst::Create(B_exit,EB);
  I->setMetadata("dbg",MD);
  remove_disconnected(l);
  return true;
}

bool SpinAssumePass::runOnLoop(llvm::Loop *L, llvm::LPPassManager &LPM){
  bool modified = false;
  if(is_spin(L)){
    if(assumify_loop(L,LPM)){
#ifdef HAVE_LLVM_LOOPINFO_ERASE
      LPM.getAnalysis<llvm::LoopInfoWrapperPass>().getLoopInfo().erase(L);
      LPM.markLoopAsDeleted(*L);
#elif defined(HAVE_LLVM_LOOPINFO_MARK_AS_REMOVED)
      LPM.getAnalysis<llvm::LoopInfoWrapperPass>().getLoopInfo().markAsRemoved(L);
#else
      LPM.deleteLoopFromQueue(L);
#endif
      modified = true;
    }
  }
  return modified;
}

char SpinAssumePass::ID = 0;
static llvm::RegisterPass<SpinAssumePass> X("spin-assume","Replace spin loops with __VERIFIER_assumes.");

char DeclareAssumePass::ID = 0;
static llvm::RegisterPass<DeclareAssumePass> Y("declare-assume","Declare __VERIFIER_assume function in module.");
