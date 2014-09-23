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

#include "CheckModule.h"
#include "LoopUnrollPass.h"
#include "SpinAssumePass.h"

#include <sstream>

#include <llvm/IR/Instructions.h>
#include <llvm/Transforms/Utils/Cloning.h>

void LoopUnrollPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const{
  llvm::LoopPass::getAnalysisUsage(AU);
  AU.addRequired<SpinAssumePass>();
  AU.addPreserved<SpinAssumePass>();
};

llvm::BasicBlock *LoopUnrollPass::make_diverge_block(llvm::Loop *L){
  llvm::Function *F = (*L->block_begin())->getParent();
  llvm::BasicBlock *Diverge =
    llvm::BasicBlock::Create(F->getContext(),"diverge",F);
  llvm::Function *F_assume = F->getParent()->getFunction("__VERIFIER_assume");
  assert(F_assume);
  llvm::Type *assume_arg0_ty = F_assume->arg_begin()->getType();
  llvm::CallInst::Create(F_assume,{llvm::ConstantInt::get(assume_arg0_ty,0)},"",Diverge);
  llvm::BranchInst::Create(Diverge,Diverge);
  return Diverge;
};

bool LoopUnrollPass::runOnLoop(llvm::Loop *L, llvm::LPPassManager &LPM){
  llvm::Function *F = (*L->block_begin())->getParent();
  llvm::BasicBlock *Diverge = make_diverge_block(L);

  /* Each vector in bodies is the blocks of one version of the loop
   * body. bodies[0] is L->getBlocksVector().
   */
  std::vector<std::vector<llvm::BasicBlock*> > bodies(1,L->getBlocksVector());
  std::vector<llvm::ValueToValueMapTy> VMaps(unroll_depth);
  /* Maps BasicBlocks in bodies[0] to their indices in that vector. */
  std::map<llvm::BasicBlock const *,int> loop_block_idx;
  for(unsigned i = 0; i < bodies[0].size(); ++i){
    loop_block_idx[bodies[0][i]] = i;
  }

  /* Create the new clones of the loop body */
  for(int unroll_idx = 1; unroll_idx < unroll_depth; ++unroll_idx){
    std::stringstream ss;
    ss << "." << unroll_idx;
    /* Create another clone of the loop body */
    bodies.push_back({});
    for(auto it = bodies[0].begin(); it != bodies[0].end(); ++it){
      llvm::BasicBlock *B = llvm::CloneBasicBlock(*it,VMaps[unroll_idx],ss.str());
      F->getBasicBlockList().push_back(B);
      bodies.back().push_back(B);
      L->addBasicBlockToLoop(B,LPM.getAnalysis<llvm::LoopInfo>().getBase());
    }
  }

  /* Redirect all branches, value uses, and phi nodes to the correct
   * version of the loop body
   */
  for(int i = 0; i < unroll_depth; ++i){
    for(int j = 0; j < int(bodies[i].size()); ++j){
      /* Branches */
      llvm::TerminatorInst *TI = bodies[i][j]->getTerminator();
      int nsucc = TI->getNumSuccessors();
      for(int k = 0; k < nsucc; ++k){
        llvm::BasicBlock *succ0 = TI->getSuccessor(k);
        if(loop_block_idx.count(succ0)){
          int tgt = loop_block_idx[succ0];
          if(tgt == 0){
            // Edge to header
            if(i < unroll_depth-1){
              // Redirect to next loop body
              TI->setSuccessor(k,bodies[i+1][tgt]);
            }else{
              // End of the line -> Diverge
              TI->setSuccessor(k,Diverge);
            }
          }else{
            // Edge to some block inside the same version of loop body
            TI->setSuccessor(k,bodies[i][tgt]);
          }
        }else{
          // Edge out from loop. Leave it be.
        }
      }

      /* PHI nodes and value uses */
      for(auto I = bodies[i][j]->begin(); I != bodies[i][j]->end(); ++I){
        if(llvm::isa<llvm::PHINode>(*I)){
          llvm::PHINode *P = static_cast<llvm::PHINode*>(&*I);
          int nvals = P->getNumIncomingValues();
          for(int k = 0; k < nvals; ++k){
            if(loop_block_idx.count(P->getIncomingBlock(k))){
              int tgt = loop_block_idx[P->getIncomingBlock(k)];
              if(j == 0){
                // Incoming to header from previous loop iteration
                if(i == 0){
                  // Remove incoming to initial body
                  P->removeIncomingValue(k);
                  --k;
                  --nvals;
                }else{
                  P->setIncomingBlock(k,bodies[i-1][tgt]);
                  if(i-1 != 0 && VMaps[i].count(P->getIncomingValue(k))){
                    P->setIncomingValue(k,VMaps[i-1][P->getIncomingValue(k)]);
                  }
                }
              }else{
                // Incoming from inside this loop
                P->setIncomingBlock(k,bodies[i][tgt]);
                if(i != 0 && VMaps[i].count(P->getIncomingValue(k))){
                  P->setIncomingValue(k,VMaps[i][P->getIncomingValue(k)]);
                }
              }
            }else{
              // Incoming from outside of loop
              if(i == 0){
                // Leave it be.
              }else{
                // Remove
                P->removeIncomingValue(k);
                --k;
                --nvals;
              }
            }
          }
        }else{ // Not a PHI node
          int nops = I->getNumOperands();
          for(int k = 0; k < nops; ++k){
            if(VMaps[i].count(I->getOperand(k))){
              I->setOperand(k,VMaps[i][I->getOperand(k)]);
            }
          }
        }
      }
    }
  }

  LPM.deleteLoopFromQueue(L);

  return true;
};

char LoopUnrollPass::ID = 0;
