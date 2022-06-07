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
#include "LoopBoundPass.h"
#include "SpinAssumePass.h"
#include "DeadCodeElimPass.h"
#include "CastElimPass.h"
#include "PartialLoopPurityPass.h"
#include "AssumeAwaitPass.h"
#include "StrModule.h"
#include "Transform.h"
#include "llvm/IR/LLVMContext.h"

#if defined(HAVE_LLVM_ANALYSIS_VERIFIER_H)
#include <llvm/Analysis/Verifier.h>
#elif defined(HAVE_LLVM_IR_VERIFIER_H)
#include <llvm/IR/Verifier.h>
#endif
#if defined(HAVE_LLVM_PASSMANAGER_H)
#include <llvm/PassManager.h>
#elif defined(HAVE_LLVM_IR_PASSMANAGER_H)
#include <llvm/IR/PassManager.h>
#endif
#if defined(HAVE_LLVM_IR_LEGACYPASSMANAGER_H) && defined(LLVM_PASSMANAGER_TEMPLATE)
#include <llvm/IR/LegacyPassManager.h>
#endif
#include <llvm/InitializePasses.h>
#ifdef HAVE_LLVM_TRANSFORMS_UTILS_H
#  include <llvm/Transforms/Utils.h>
#else
#  include <llvm/Transforms/Scalar.h>
#endif

#include <stdexcept>

namespace Transform {

  std::string transform(const std::string &src, const Configuration &conf){
    llvm::LLVMContext context;
    llvm::Module *mod = StrModule::read_module_src(src, context);
    transform(*mod,conf);
    return StrModule::write_module_str(mod);
  }

  void transform(std::string infile, std::string outfile, const Configuration &conf){
    llvm::LLVMContext context;
    llvm::Module *mod = StrModule::read_module(infile, context);

    transform(*mod,conf);

    StrModule::write_module(mod,outfile);
  }

  namespace {
    struct ClearOptnonePass : public llvm::FunctionPass {
      static char ID;
      ClearOptnonePass() : llvm::FunctionPass(ID) {}
      bool runOnFunction(llvm::Function &F) override {
        if (F.hasFnAttribute(llvm::Attribute::OptimizeNone)) {
          F.removeFnAttr(llvm::Attribute::OptimizeNone);
          return true;
        }
        return false;
      }
    };
    char ClearOptnonePass::ID = 0;
  }  // namespace

  bool transform(llvm::Module &mod, const Configuration &conf){
    llvm::PassRegistry &Registry = *llvm::PassRegistry::getPassRegistry();
    llvm::initializeCore(Registry);
    llvm::initializeScalarOpts(Registry);
    llvm::initializeObjCARCOpts(Registry);
    llvm::initializeVectorization(Registry);
    llvm::initializeIPO(Registry);
    llvm::initializeAnalysis(Registry);
#ifdef HAVE_LLVM_INITIALIZE_IPA
    llvm::initializeIPA(Registry);
#endif
    llvm::initializeTransformUtils(Registry);
    llvm::initializeInstCombine(Registry);
    llvm::initializeInstrumentation(Registry);
    llvm::initializeTarget(Registry);

#ifdef LLVM_PASSMANAGER_TEMPLATE
    using PassManager = llvm::legacy::PassManager;
#else
    using PassManager = llvm::PassManager;
#endif
    PassManager PM;
    /* Run some safe simplifications that both improve applicability
     * of our passes, and speed up model checking.
     * We need to clear the "optnone" attribute set by clang, or all the
     * optimizers will no-op.
     */
    PM.add(new ClearOptnonePass());
    PM.add(llvm::createPromoteMemoryToRegisterPass());
    if (conf.transform_cast_elim) {
      PM.add(new CastElimPass());
    }
    if (conf.transform_dead_code_elim) {
      PM.add(new DeadCodeElimPass());
    }
    if (conf.transform_partial_loop_purity) {
      PM.add(new PartialLoopPurityPass());
    }
    if (conf.transform_spin_assume){
      PM.add(new SpinAssumePass());
    }
    if (conf.transform_loop_unroll >= 0) {
      PM.add(new LoopBoundPass(conf.transform_loop_unroll));
    }
    if(conf.transform_assume_await){
      PM.add(new AssumeAwaitPass());
    }
    PM.add(new AddLibPass());
    bool modified = PM.run(mod);
    assert(!llvm::verifyModule(mod, &llvm::dbgs()));
    return modified;
  }

}  // namespace Transform
