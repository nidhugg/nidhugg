/* Copyright (C) 2020 Magnus Lång
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

#include <string>
#include <utility>
#include <vector>

#include <llvm/Config/llvm-config.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/Verifier.h>
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
#include <llvm/Pass.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

// #include "CheckModule.h"
#include "AssumeAwaitPass.h"
#include "SpinAssumePass.h"
#include "AwaitCond.h"
#include "Debug.h"

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

void AssumeAwaitPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const{
  AU.setPreservesCFG();
  AU.addRequired<llvm::LLVM_DOMINATOR_TREE_PASS>();
}

unsigned AssumeAwaitPass::sizes[AssumeAwaitPass::no_sizes] = {8,16,32,64};

namespace {
  llvm::cl::opt<bool> cl_no_assume_xchgawait("no-assume-xchgawait");
  llvm::cl::opt<int> cl_asaw_dbgp("asaw-debug");

  llvm::Value* getOrInsertFunction(llvm::Module &M, llvm::StringRef Name,
                                   llvm::FunctionType *T, AttributeList AttributeList) {
    auto ret = M.getOrInsertFunction(std::move(Name),T,std::move(AttributeList));
#if LLVM_VERSION_MAJOR >= 9
      /* XXX: I will not work with some development versions of 9, I
       * should be replaced/complemented with a configure check.
       */
    return ret.getCallee();
#else
    return ret;
#endif
  }

  bool is_assume(llvm::CallInst *C) {
    llvm::Function *F = C->getCalledFunction();
    return F && F->getName().str() == "__VERIFIER_assume";
  }

  bool is_true(llvm::Value *v) {
    auto *I = llvm::dyn_cast<llvm::Constant>(v);
    return I && I->isAllOnesValue();
  }

  bool is_false(llvm::Value *v) {
    auto *I = llvm::dyn_cast<llvm::Constant>(v);
    return I && I->isZeroValue();
  }

  llvm::Value *simplify_condition(llvm::Value *v, bool *negate) {
    if (auto *ret = llvm::dyn_cast<llvm::ICmpInst>(v)) {
      /* ret could be the condition we're looking for, but it could also
       * be a comparison of a truth value with a constant. Check for
       * this case before settling for ret */
      if (ret->getPredicate() == llvm::CmpInst::ICMP_EQ
          || ret->getPredicate() == llvm::CmpInst::ICMP_NE) {
        for (unsigned otheri = 0; otheri < 2; ++otheri) {
          llvm::Value *other = ret->getOperand(otheri);
          llvm::Value *constant = ret->getOperand(1-otheri);
          if (is_false(constant)) *negate ^= true;
          else if (!is_true(constant)) continue;
          if (ret->getPredicate() == llvm::CmpInst::ICMP_NE)
            *negate ^= true;
          return simplify_condition(other, negate);
        }
      }
      return ret;
    }
    if (auto *op = llvm::dyn_cast<llvm::ZExtOperator>(v))
      return simplify_condition(op->getOperand(0), negate);
    if (auto *op = llvm::dyn_cast<llvm::Instruction>(v)) {
      if (op->getOpcode() == llvm::Instruction::ZExt
          || op->getOpcode() == llvm::Instruction::SExt) {
        return simplify_condition(op->getOperand(0), negate);
      } else if(op->getOpcode() == llvm::Instruction::Xor) {
        if (is_true(op->getOperand(0)) || is_true(op->getOperand(1))) {
          *negate ^= true;
          unsigned opi = is_true(op->getOperand(0)) ? 1 : 0;
          return simplify_condition(op->getOperand(opi), negate);
        }
      }
    }
    return v;
  }

  llvm::ICmpInst *get_condition(llvm::Value *v, bool *negate, llvm::Value **monitor) {
    if (auto *ret = llvm::dyn_cast<llvm::ICmpInst>(v)) {
      /* ret could be the condition we're looking for, but it could also
       * be a comparison of a truth value with a constant. Check for
       * this case before settling for ret */
      if (ret->getPredicate() == llvm::CmpInst::ICMP_EQ) {
        for (unsigned otheri = 0; otheri < 2; ++otheri) {
          bool negate2 = false; // Don't affect *negate until we're sure
          llvm::Value *other = ret->getOperand(otheri);
          llvm::Value *constant = ret->getOperand(1-otheri);
          if (is_false(constant)) negate2 ^= true;
          else if (!is_true(constant)) continue;
          if (auto *ret2 = get_condition(other, &negate2, monitor)) {
            *negate ^= negate2;
            return ret2;
          }
        }
      }
      return ret;
    }
    if (auto *op = llvm::dyn_cast<llvm::ZExtOperator>(v))
      return get_condition(op->getOperand(0), negate, monitor);
    if (auto *op = llvm::dyn_cast<llvm::Instruction>(v)) {
      if (op->getOpcode() == llvm::Instruction::ZExt
          || op->getOpcode() == llvm::Instruction::SExt) {
        return get_condition(op->getOperand(0), negate, monitor);
      } else if(op->getOpcode() == llvm::Instruction::Xor) {
        if (is_true(op->getOperand(0)) || is_true(op->getOperand(1))) {
          *negate ^= true;
          return get_condition(op->getOperand(is_true(op->getOperand(0)) ? 1 : 0),
                               negate, monitor);
        }
      }
    }
    if (auto *op = llvm::dyn_cast<llvm::BinaryOperator>(v)) {
      if (op->getOpcode() != llvm::BinaryOperator::Or) return nullptr;
      if (*monitor) return nullptr;
      llvm::Value *Other = op->getOperand(0);
      auto *MonCand = llvm::dyn_cast<llvm::ICmpInst>(op->getOperand(1));
      if (!MonCand
          || MonCand->getOperand(1) != llvm::ConstantInt::getTrue(v->getContext())
          || MonCand->getPredicate() != llvm::CmpInst::ICMP_NE) {
        /* Try other way */
        MonCand = llvm::dyn_cast<llvm::ICmpInst>(Other);
        Other = op->getOperand(1);
        if (!MonCand
            || MonCand->getOperand(1) != llvm::ConstantInt::getTrue(v->getContext())
            || MonCand->getPredicate() != llvm::CmpInst::ICMP_NE) {
          return nullptr;
        }
      }
      *monitor = MonCand->getOperand(0);
      llvm::ICmpInst *res = get_condition(Other, negate, monitor);
      if (!res) *monitor = nullptr;
      return res;
    }
    return nullptr;
  }

  AwaitCond::Op get_op(llvm::CmpInst::Predicate p) {
    switch(p) {
    case llvm::CmpInst::ICMP_EQ:  return AwaitCond::EQ;
    case llvm::CmpInst::ICMP_NE:  return AwaitCond::NE;
    case llvm::CmpInst::ICMP_UGT: return AwaitCond::UGT;
    case llvm::CmpInst::ICMP_UGE: return AwaitCond::UGE;
    case llvm::CmpInst::ICMP_ULT: return AwaitCond::ULT;
    case llvm::CmpInst::ICMP_ULE: return AwaitCond::ULE;
    case llvm::CmpInst::ICMP_SGT: return AwaitCond::SGT;
    case llvm::CmpInst::ICMP_SGE: return AwaitCond::SGE;
    case llvm::CmpInst::ICMP_SLT: return AwaitCond::SLT;
    case llvm::CmpInst::ICMP_SLE: return AwaitCond::SLE;
    default: return AwaitCond::None;
    }
  }

  std::uint8_t get_mode(llvm::Instruction *Load) {
    llvm::AtomicOrdering order;
    if (llvm::LoadInst *L = llvm::dyn_cast<llvm::LoadInst>(Load)) {
      order = L->getOrdering();
    } else if (auto *RMW = llvm::dyn_cast<llvm::AtomicRMWInst>(Load)) {
      order = RMW->getOrdering();
    } else {
      order = llvm::cast<llvm::AtomicCmpXchgInst>(Load)->getSuccessOrdering();
    }
    switch (order) {
      /* TODO: lift these modes to an enum somewhere */
    case llvm::AtomicOrdering::NotAtomic:
    case llvm::AtomicOrdering::Unordered:
    case llvm::AtomicOrdering::Monotonic:
      return 0; /* Races are bugs */
    case llvm::AtomicOrdering::Acquire:
    case llvm::AtomicOrdering::AcquireRelease:
      return 1; /* Release/Acquire */
    case llvm::AtomicOrdering::SequentiallyConsistent:
      return 2;
    default:
      llvm_unreachable("No other access modes allowed for Loads");
    }
  }

  llvm::Instruction *is_permissible_load(llvm::Value *V, llvm::BasicBlock *BB) {
    llvm::Instruction *I = llvm::dyn_cast<llvm::Instruction>(V);
    if (!I || I->getParent() != BB) return nullptr;
    if (auto *Load = llvm::dyn_cast<llvm::LoadInst>(I)) {
      return Load;
    }
    if (auto *AtomicRmw = llvm::dyn_cast<llvm::AtomicRMWInst>(I)) {
      if (AtomicRmw->getOperation() == llvm::AtomicRMWInst::Xchg
          && !cl_no_assume_xchgawait) {
        return AtomicRmw;
      }
    }
    return nullptr;
  }

  bool is_permissible_arg(llvm::DominatorTree &DT, llvm::Value *V, llvm::Instruction *Load) {
    if (llvm::isa<llvm::Constant>(V)) return true;
    if (llvm::isa<llvm::Argument>(V)) return true;
    if (llvm::Instruction *VI = llvm::dyn_cast<llvm::Instruction>(V))
      return DT.dominates(VI, Load);
    /* TODO: What about operators applied to permissible_arg(s)? */
    return false;
  }

  bool is_permissible_monitor(llvm::DominatorTree &DT, llvm::Instruction *Load, llvm::Value *Monitor) {
    if (!Monitor) return true;
    auto *MonI = llvm::cast<llvm::Instruction>(Monitor);
    return DT.dominates(MonI, Load);
  }

  bool is_safe_intermediary(llvm::Instruction *I) {
    return !I->mayHaveSideEffects();
  }

  bool is_safe_to_rewrite(llvm::Instruction *Load, llvm::CallInst *Call) {
    if (Load->getParent() != Call->getParent()) {
#ifndef NDEBUG
      Debug::warn("assume-await-no-traversal")
        << "WARNING: assume-await: We don't yet implement BB traversal";
#endif
      return false;
    }
    llvm::BasicBlock::iterator li(Load), ci(Call);
    while (--ci != li) {
      if (!is_safe_intermediary(&*ci)) return false;
    }
    return true;
  }
}  // namespace

bool AssumeAwaitPass::doInitialization(llvm::Module &M){
  bool modified_M = false;
  // CheckModule::check_await(&M);

  for (unsigned i = 0; i < no_sizes; ++i) {
    std::string lname = "__VERIFIER_load_await_i" + std::to_string(sizes[i]);
    std::string xname = "__VERIFIER_xchg_await_i" + std::to_string(sizes[i]);
    llvm::FunctionType *loadAwaitTy, *xchgAwaitTy;
    {
      llvm::Type *i8Ty = llvm::Type::getInt8Ty(M.getContext());
      llvm::Type *ixTy = llvm::IntegerType::get(M.getContext(),sizes[i]);
      llvm::Type *ixPTy = llvm::PointerType::getUnqual(ixTy);
      loadAwaitTy = llvm::FunctionType::get(ixTy,{ixPTy, i8Ty, ixTy, i8Ty},false);
      xchgAwaitTy = llvm::FunctionType::get(ixTy,{ixPTy, ixTy, i8Ty, ixTy, i8Ty},false);
    }
    F_load_await[i] = {M.getFunction(lname), loadAwaitTy};
    F_xchg_await[i] = {M.getFunction(xname), xchgAwaitTy};
    if(!std::get<0>(F_load_await[i]) || !std::get<0>(F_xchg_await[i])){
      AttributeList assumeAttrs =
        AttributeList::get(M.getContext(),AttributeList::FunctionIndex,
                           std::vector<llvm::Attribute::AttrKind>({llvm::Attribute::NoUnwind}));
      F_load_await[i] = {getOrInsertFunction(M,lname,loadAwaitTy,assumeAttrs), loadAwaitTy};
      F_xchg_await[i] = {getOrInsertFunction(M,xname,xchgAwaitTy,assumeAttrs), xchgAwaitTy};
      modified_M = true;
    }
  }
  return modified_M;
}

bool AssumeAwaitPass::runOnFunction(llvm::Function &F) {
  bool changed = false;

  for (llvm::BasicBlock &BB : F) {
    for (llvm::Instruction *it = &*BB.begin(), *end = BB.getTerminator(); it != end;) {
      if (tryRewriteAssume(&F, &BB, &*it)) {
        changed = true;
        it = (it->use_empty()) ? &*it->eraseFromParent() : it->getNextNode();
        assert(!llvm::verifyFunction(F, &llvm::dbgs()));
      } else {
        it = it->getNextNode();
      }
    }
  }

  return changed;
}

static llvm::Twine concat_if(const llvm::StringRef &first, const char *suffix) {
  if (first.empty()) return "";
  else return first + suffix;
}

bool AssumeAwaitPass::tryRewriteAssume(llvm::Function *F, llvm::BasicBlock *BB, llvm::Instruction *I) const {
  llvm::CallInst *Call = llvm::dyn_cast<llvm::CallInst>(I);
  if (!Call || !is_assume(Call)) return false;
  if (tryRewriteAssumeCmpXchg(F, BB, Call)) return true;
  bool negate = false;
  llvm::Value *Monitor = nullptr;
  llvm::ICmpInst *Cond = get_condition(Call->getArgOperand(0), &negate, &Monitor);
  if (!Cond) return false;
  for (unsigned load_index = 0; load_index < 2; ++load_index) {
    llvm::Instruction *Load = is_permissible_load(Cond->getOperand(load_index),BB);
    if (!Load) continue;
    llvm::Value *ArgVal = Cond->getOperand(1-load_index);
    llvm::CmpInst::Predicate pred = Cond->getPredicate();
    if (load_index == 1) pred = llvm::CmpInst::getSwappedPredicate(pred);
    if (negate) pred = llvm::CmpInst::getInversePredicate(pred);
    llvm::DominatorTree &DT = getAnalysis<llvm::LLVM_DOMINATOR_TREE_PASS>().getDomTree();
    if (!is_permissible_arg(DT, ArgVal, Load)) continue;
    if (!is_permissible_monitor(DT, Load, Monitor)) continue;
    if (!is_safe_to_rewrite(Load, Call)) continue;
    AwaitCond::Op op = get_op(pred);
    if (op == AwaitCond::None) continue;
    llvm::Value *AwaitFunction;
    llvm::FunctionType *AwaitFunctionType;
    std::tie(AwaitFunction, AwaitFunctionType) = getAwaitFunction(Load);
    if (!AwaitFunction) continue;

    /* Do the rewrite */
    llvm::BasicBlock *First = BB, *Last, *NoAwaitBB, *AwaitBB;
    llvm::PHINode *Phi;
    if (Monitor) {
      /* We have to split the block twice, and not replace the load but
       * insert the await in another new block */
      NoAwaitBB = BB->splitBasicBlock(Load, concat_if(BB->getName(), ".noawait"));
      Last = NoAwaitBB->splitBasicBlock(Load->getNextNode(),
                                        concat_if(BB->getName(), ".cont"));
      AwaitBB = llvm::BasicBlock::Create
        (Load->getContext(), concat_if(BB->getName(), ".await"), F, Last);
      llvm::BranchInst::Create(Last, AwaitBB);
      Phi = llvm::PHINode::Create(Load->getType(), 2,
                                  concat_if(Load->getName(), ".merge"),
                                  &*Last->getFirstInsertionPt());
      Load->replaceAllUsesWith(Phi);
      Phi->addIncoming(Load, NoAwaitBB);
      llvm::BranchInst *BadBr = llvm::cast<llvm::BranchInst>(First->getTerminator());
      llvm::BranchInst::Create(AwaitBB, NoAwaitBB, Monitor, First);
      BadBr->eraseFromParent();
    }
    llvm::IntegerType *i8Ty = llvm::Type::getInt8Ty(F->getContext());
    llvm::ConstantInt *COp = llvm::ConstantInt::get(i8Ty, op);
    llvm::ConstantInt *CMode = llvm::ConstantInt::get(i8Ty, get_mode(Load));
    llvm::Value *Address = Load->getOperand(0);
    llvm::BasicBlock::iterator LI(Load);
    llvm::SmallVector<llvm::Value*, 5> args;
    if (llvm::isa<llvm::LoadInst>(Load)) {
      args = {Address, COp, ArgVal, CMode};
    } else {
      llvm::AtomicRMWInst *RMW = llvm::cast<llvm::AtomicRMWInst>(Load);
      args = {Address, RMW->getValOperand(), COp, ArgVal, CMode};
    }
    llvm::CallInst *Await = llvm::CallInst::Create(AwaitFunctionType, AwaitFunction, args);
    if (!Monitor) {
      if (cl_asaw_dbgp >= 1) {
        Load->printAsOperand(llvm::dbgs() << "Replacing load ", false);
        BB->printAsOperand(llvm::dbgs() << " in ", false);
        llvm::dbgs()<< " with load-await\n";
      }
      llvm::ReplaceInstWithInst(Load->getParent()->getInstList(), LI, Await);
    } else {
      Await->insertBefore(AwaitBB->getTerminator());
      Phi->addIncoming(Await, AwaitBB);
      if (cl_asaw_dbgp >= 1) {
        Load->printAsOperand(llvm::dbgs() << "Supplementing load ", false);
        BB->printAsOperand(llvm::dbgs() << " in ", false);
        llvm::dbgs()  << " with load-await\n";
      }
      if (cl_asaw_dbgp >= 2) {
        First->print(llvm::dbgs());
        AwaitBB->print(llvm::dbgs());
        NoAwaitBB->print(llvm::dbgs());
        Last->print(llvm::dbgs());
      }
    }
    return true;
  }
  return false;
}

bool AssumeAwaitPass::tryRewriteAssumeCmpXchg
(llvm::Function *F, llvm::BasicBlock *BB, llvm::CallInst *Call) const {
  bool negate = false;
  llvm::Value *CondVal = simplify_condition(Call->getArgOperand(0), &negate);
  llvm::ExtractValueInst *EV = llvm::dyn_cast<llvm::ExtractValueInst>(CondVal);
  if (!EV || EV->getNumIndices() != 1 || EV->getIndices()[0] != 1) return false;
  if (negate) return false;
  llvm::AtomicCmpXchgInst *CmpXchg
    = llvm::dyn_cast<llvm::AtomicCmpXchgInst>(EV->getAggregateOperand());
  if (!CmpXchg) return false;
  if (!is_safe_to_rewrite(CmpXchg, Call)) return false;

  llvm::Value *AwaitFunction;
  llvm::FunctionType *AwaitFunctionType;
  std::tie(AwaitFunction, AwaitFunctionType) = getAwaitFunction(CmpXchg);
  if (!AwaitFunction) return false;

  /* Do the rewrite */
  /* Step one; replace uses */
  auto *True = llvm::ConstantInt::getTrue(F->getContext());
  llvm::Value *Expected = CmpXchg->getCompareOperand();
  llvm::Value *RetReplacement;
  if (llvm::Constant *ExpectedC = llvm::dyn_cast<llvm::Constant>(Expected)) {
    RetReplacement = llvm::ConstantStruct::getAnon({ExpectedC, True});
  } else {
    llvm::UndefValue *CS = llvm::UndefValue::get
      (llvm::StructType::get(Expected->getType(), True->getType()));
    RetReplacement = llvm::InsertValueInst::Create
      (CS, Expected, {0}, "", CmpXchg);
    RetReplacement = llvm::InsertValueInst::Create
      (RetReplacement, True, {1}, "", CmpXchg);
  }
  CmpXchg->replaceAllUsesWith(RetReplacement);
  /* Step two, replace cmpxchg */
  llvm::IntegerType *i8Ty = llvm::Type::getInt8Ty(F->getContext());
  llvm::Value *Address = CmpXchg->getPointerOperand();
  llvm::CallInst::Create
    (AwaitFunctionType, AwaitFunction, {Address, CmpXchg->getNewValOperand(),
      llvm::ConstantInt::get(i8Ty, AwaitCond::EQ), Expected,
      llvm::ConstantInt::get(i8Ty, get_mode(CmpXchg))},
      "", CmpXchg);
  if (cl_asaw_dbgp >= 1)
    llvm::dbgs() << "Replacing cmpxchg " << CmpXchg->getName() << " with xchg-await\n";
  CmpXchg->eraseFromParent();
  return true;
}

std::pair<llvm::Value*,llvm::FunctionType*>
AssumeAwaitPass::getAwaitFunction(llvm::Instruction *Load) const {
  llvm::Type *t = Load->getType();
  if (llvm::isa<llvm::AtomicCmpXchgInst>(Load)) {
    t = llvm::cast<llvm::StructType>(t)->getStructElementType(0);
  }
  if (!t->isIntegerTy()) return {nullptr,nullptr};
  unsigned index;
  switch(t->getIntegerBitWidth()) {
  case 8:  index = 0; break;
  case 16: index = 1; break;
  case 32: index = 2; break;
  case 64: index = 3; break;
  default: return {nullptr,nullptr};
  }
  if (llvm::isa<llvm::LoadInst>(Load)) {
    return F_load_await[index];
  } else {
    assert(llvm::isa<llvm::AtomicRMWInst>(Load)
           || llvm::isa<llvm::AtomicCmpXchgInst>(Load));
    return F_xchg_await[index];
  }
}

char AssumeAwaitPass::ID = 0;
static llvm::RegisterPass<AssumeAwaitPass> X("assume-await","Replace simple __VERIFIER_assumes with __VERIFIER_await.");
