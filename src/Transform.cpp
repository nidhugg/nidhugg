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
#include "LoopUnrollPass.h"
#include "SpinAssumePass.h"
#include "StrModule.h"
#include "Transform.h"

#include <llvm/Analysis/Verifier.h>
#include <llvm/PassManager.h>

#include <stdexcept>

namespace Transform {

  std::string transform(const std::string &src, const Configuration &conf){
    llvm::Module *mod = StrModule::read_module_src(src);
    transform(*mod,conf);
    return StrModule::write_module_str(mod);
  };

  void transform(std::string infile, std::string outfile, const Configuration &conf){
    llvm::Module *mod = StrModule::read_module(infile);

    transform(*mod,conf);

    StrModule::write_module(mod,outfile);
  };

  bool transform(llvm::Module &mod, const Configuration &conf){
    llvm::PassRegistry &Registry = *llvm::PassRegistry::getPassRegistry();
    llvm::initializeCore(Registry);
    llvm::initializeScalarOpts(Registry);
    llvm::initializeObjCARCOpts(Registry);
    llvm::initializeVectorization(Registry);
    llvm::initializeIPO(Registry);
    llvm::initializeAnalysis(Registry);
    llvm::initializeIPA(Registry);
    llvm::initializeTransformUtils(Registry);
    llvm::initializeInstCombine(Registry);
    llvm::initializeInstrumentation(Registry);
    llvm::initializeTarget(Registry);

    llvm::PassManager PM;
    if(conf.transform_spin_assume){
      PM.add(new SpinAssumePass());
    }
    if(0 <= conf.transform_loop_unroll){
      PM.add(new LoopUnrollPass(conf.transform_loop_unroll));
    }
    PM.add(new AddLibPass());
    bool modified = PM.run(mod);
    assert(!llvm::verifyModule(mod));
    return modified;
  };

}
