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
#include "LoopUnrollPass.h"
#include "SpinAssumePass.h"
#include "vecset.h"

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

void LoopUnrollPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const{
  llvm::LoopPass::getAnalysisUsage(AU);
  AU.addRequired<DeclareAssumePass>();
  AU.addPreserved<DeclareAssumePass>();
}

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
}

bool LoopUnrollPass::runOnLoop(llvm::Loop *L, llvm::LPPassManager &LPM){
  llvm::SmallVector<llvm::BasicBlock*,10> SuccBlocks;
  L->getExitBlocks(SuccBlocks);
  llvm::Function *F = (*L->block_begin())->getParent();
  llvm::BasicBlock *Diverge = make_diverge_block(L);

  /* Each vector in bodies is the blocks of one version of the loop
   * body. bodies[0] is L->getBlocks().
   */
  std::vector<std::vector<llvm::BasicBlock*> > bodies(1,L->getBlocks());
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
#ifdef LLVM_GET_ANALYSIS_LOOP_INFO
      L->addBasicBlockToLoop(B,LPM.getAnalysis<llvm::LoopInfo>().getBase());
#else
      L->addBasicBlockToLoop(B,LPM.getAnalysis<llvm::LoopInfoWrapperPass>().getLoopInfo());
#endif
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

  /* Fix PHI nodes in successor blocks. */
  {
    VecSet<llvm::BasicBlock*> SuccBlocksUnique(SuccBlocks.begin(),SuccBlocks.end());
    for(llvm::BasicBlock *SB : SuccBlocksUnique){
      for(auto I = SB->begin(); llvm::isa<llvm::PHINode>(*I) && I != SB->end(); ++I){
        llvm::PHINode *P = static_cast<llvm::PHINode*>(&*I);

        /* Identify the value/block pairs where the block is inside the
         * loop.
         */
        std::vector<llvm::Value*> inc_vals;
        std::vector<llvm::BasicBlock*> inc_blks;
        std::vector<int> inc_blk_idxs;
        for(unsigned i = 0; i < P->getNumIncomingValues(); ++i){
          llvm::BasicBlock *B = P->getIncomingBlock(i);
          if(loop_block_idx.count(B)){
            inc_vals.push_back(P->getIncomingValue(i));
            inc_blks.push_back(P->getIncomingBlock(i));
            inc_blk_idxs.push_back(loop_block_idx[B]);
          }
        }

        /* Multiply those value/block pairs */
        for(int i = 1; i < int(bodies.size()); ++i){
          for(int j = 0; j < int(inc_blk_idxs.size()); ++j){
            int k = inc_blk_idxs[j];
            if(VMaps[i].count(inc_vals[j])){
              P->addIncoming(VMaps[i][inc_vals[j]],bodies[i][k]);
            }else{
              P->addIncoming(inc_vals[j],bodies[i][k]);
            }
          }
        }
      }
    }
  }
#ifdef HAVE_LLVM_LOOPINFO_MARK_AS_REMOVED
  LPM.getAnalysis<llvm::LoopInfoWrapperPass>().getLoopInfo().markAsRemoved(L);
#else
  LPM.deleteLoopFromQueue(L);
#endif

  return true;
}

char LoopUnrollPass::ID = 0;
