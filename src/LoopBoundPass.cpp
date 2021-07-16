/* Copyright (C) 2021 Magnus Lång
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
#include "LoopBoundPass.h"
#include "SpinAssumePass.h"

#include <sstream>

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
#include <llvm/Transforms/Utils/Cloning.h>

namespace {
  llvm::CallInst *insertAssume(llvm::Value *cond, llvm::Instruction *before) {
    llvm::Function *F_assume = before->getParent()->getParent()->getParent()
      ->getFunction("__VERIFIER_assume");
    llvm::Type *arg_ty = F_assume->arg_begin()->getType();
    assert(arg_ty->isIntegerTy());
    if(arg_ty->getIntegerBitWidth() != 1){
      cond = new llvm::ZExtInst(cond, arg_ty,"",before);
    }
    return llvm::CallInst::Create(F_assume,{cond},"",before);
  }
};

void LoopBoundPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const{
  llvm::LoopPass::getAnalysisUsage(AU);
  AU.addRequired<DeclareAssumePass>();
  AU.addPreserved<DeclareAssumePass>();
}

bool LoopBoundPass::runOnLoop(llvm::Loop *L, llvm::LPPassManager &LPM){
  llvm::BasicBlock *header = L->getHeader();
  llvm::IntegerType *i32 = llvm::IntegerType::get(header->getContext(), 32);
  auto preds = llvm::predecessors(header);
  llvm::PHINode *ctr = llvm::PHINode::Create
    (i32, std::distance(preds.begin(), preds.end()), "loop.bound.ctr",
     &*header->begin());
  llvm::Instruction *I = &*header->getFirstInsertionPt();
  llvm::BinaryOperator *ctr_plus_one = llvm::BinaryOperator::Create
    (llvm::BinaryOperator::BinaryOps::Sub, ctr, llvm::ConstantInt::get(i32, 1),
     "loop.bound.next.ctr", I);
  for (llvm::BasicBlock *pred : preds) {
    ctr->addIncoming(L->contains(pred) ? (llvm::Value*)ctr_plus_one
                     : llvm::ConstantInt::get(i32, unroll_depth),
                     pred);
  }
  llvm::CmpInst *in_bounds = llvm::CmpInst::Create
    (llvm::Instruction::OtherOps::ICmp, llvm::CmpInst::ICMP_UGT, ctr,
     llvm::ConstantInt::get(i32, 0), "loop.in.bounds", I);
  insertAssume(in_bounds, I);

  return true;
}

char LoopBoundPass::ID = 0;
