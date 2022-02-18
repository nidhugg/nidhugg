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

#include <config.h>

#include "CheckModule.h"
#include "PartialLoopPurityPass.h"
#include "SpinAssumePass.h"
#include "Debug.h"
#include "vecset.h"

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
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/Debug.h>

#include <unordered_map>
#include <sstream>

#ifdef LLVM_HAS_TERMINATORINST
typedef llvm::TerminatorInst TerminatorInst;
#else
typedef llvm::Instruction TerminatorInst;
#endif

namespace {
  /* Not reentrant */
  static const llvm::DominatorTree *DominatorTree;
  static const std::unordered_map<llvm::Function *, bool> *may_inline;
  static bool inlining_needed = false;

  std::uint_fast8_t icmpop_to_bits(llvm::CmpInst::Predicate pred) {
    using llvm::CmpInst;
    assert(pred >= CmpInst::FIRST_ICMP_PREDICATE
           && pred <= CmpInst::LAST_ICMP_PREDICATE);
    switch(pred) {
    case CmpInst::ICMP_SGT: return 0b00010;
    case CmpInst::ICMP_UGT: return 0b00001;
    case CmpInst::ICMP_SGE: return 0b00110;
    case CmpInst::ICMP_UGE: return 0b00101;
    case CmpInst::ICMP_EQ:  return 0b00100;
    case CmpInst::ICMP_NE:  return 0b11011;
    case CmpInst::ICMP_SLE: return 0b01100;
    case CmpInst::ICMP_ULE: return 0b10100;
    case CmpInst::ICMP_SLT: return 0b01000;
    case CmpInst::ICMP_ULT: return 0b10000;
    default: abort();
    }
  }

  llvm::CmpInst::Predicate bits_to_icmpop(std::uint_fast8_t bits) {
    using llvm::CmpInst;
    if ((bits & 0b01110) == 0b01110) return CmpInst::FCMP_TRUE; // Not expected to happen
    if ((bits & 0b10101) == 0b10101) return CmpInst::FCMP_TRUE; // Not expected to happen
    if ((bits & 0b01100) == 0b01100) return CmpInst::ICMP_SLE;
    if ((bits & 0b00110) == 0b00110) return CmpInst::ICMP_SGE;
    if ((bits & 0b10100) == 0b10100) return CmpInst::ICMP_ULE;
    if ((bits & 0b00101) == 0b00101) return CmpInst::ICMP_UGE;
    if ((bits & 0b01000) == 0b01000) return CmpInst::ICMP_SLT;
    if ((bits & 0b00010) == 0b00010) return CmpInst::ICMP_SGT;
    if ((bits & 0b10000) == 0b10000) return CmpInst::ICMP_ULT;
    if ((bits & 0b00001) == 0b00001) return CmpInst::ICMP_UGT;
    if ((bits & 0b01010) == 0b01010) return CmpInst::ICMP_NE;
    if ((bits & 0b10001) == 0b10001) return CmpInst::ICMP_NE;
    if ((bits & 0b00100) == 0b00100) return CmpInst::ICMP_EQ;
    return CmpInst::FCMP_FALSE;
  }

  llvm::CmpInst::Predicate icmpop_invert_strictness(llvm::CmpInst::Predicate pred) {
    return bits_to_icmpop(0b00100 ^ icmpop_to_bits(pred));
  }

  bool check_predicate_satisfaction(const llvm::APInt &lhs,
                                    llvm::CmpInst::Predicate pred,
                                    const llvm::APInt &rhs) {
    using llvm::CmpInst;
    assert(pred >= CmpInst::FIRST_ICMP_PREDICATE
           && pred <= CmpInst::LAST_ICMP_PREDICATE);
    switch(pred) {
    case CmpInst::ICMP_SGT: return lhs.sgt(rhs);
    case CmpInst::ICMP_UGT: return lhs.ugt(rhs);
    case CmpInst::ICMP_SGE: return lhs.sge(rhs);
    case CmpInst::ICMP_UGE: return lhs.uge(rhs);
    case CmpInst::ICMP_EQ:  return lhs.eq (rhs);
    case CmpInst::ICMP_NE:  return lhs.ne (rhs);
    case CmpInst::ICMP_SLE: return lhs.sle(rhs);
    case CmpInst::ICMP_ULE: return lhs.ule(rhs);
    case CmpInst::ICMP_SLT: return lhs.slt(rhs);
    case CmpInst::ICMP_ULT: return lhs.ult(rhs);
    default: abort();
    }
  }

  struct BinaryPredLoc {
    struct BinaryPredicate {
      llvm::Value *lhs = nullptr, *rhs = nullptr;
      /* FCMP_TRUE and FCMP_FALSE are used as special values indicating
       * unconditional purity
       */
      llvm::CmpInst::Predicate op = llvm::CmpInst::FCMP_TRUE;

      BinaryPredicate() {}
      BinaryPredicate(bool static_value) {
        op = static_value ? llvm::CmpInst::FCMP_TRUE : llvm::CmpInst::FCMP_FALSE;
      }
      BinaryPredicate(llvm::CmpInst::Predicate op, llvm::Value *lhs,
                      llvm::Value *rhs)
        : lhs(lhs), rhs(rhs), op(op) {
        assert(!(is_true() || is_false()) || (lhs == nullptr && rhs == nullptr));
      }

      /* A BinaryPredicate must be normalised after direct modification of its members. */
      void normalise() {
        if (op == llvm::CmpInst::FCMP_TRUE || op == llvm::CmpInst::FCMP_FALSE) {
          lhs = rhs = nullptr;
        } else {
          assert(lhs && rhs);
        }
      }

      bool is_true() const {
        assert(!(op == llvm::CmpInst::FCMP_TRUE && (lhs || rhs)));
        return op == llvm::CmpInst::FCMP_TRUE;
      }
      bool is_false() const {
        assert(!(op == llvm::CmpInst::FCMP_FALSE && (lhs || rhs)));
        return op == llvm::CmpInst::FCMP_FALSE;
      }

      BinaryPredicate negate() const {
        return {llvm::CmpInst::getInversePredicate(op), lhs, rhs};
      }

      BinaryPredicate swap() const {
        return {llvm::CmpInst::getSwappedPredicate(op), rhs, lhs};
      }

      // IDEA: Use operator| for join (and operator& for meet, if needed),
      // unless it's confusing
      BinaryPredicate operator&(const BinaryPredicate &other) const {
        if (other.is_true() || *this == other) return *this;
        if (is_true()) return other;

        using llvm::CmpInst;
        BinaryPredicate res = *this;
        BinaryPredicate o(other);
        if (res.rhs == o.lhs || res.rhs == o.rhs) res = swap();
        if (res.lhs == o.lhs || res.lhs == o.rhs) {
          if (res.lhs != o.lhs) o = o.swap();
          if (res.rhs == o.rhs) {
            if (llvm::CmpInst::isFPPredicate(res.op)
                && llvm::CmpInst::isFPPredicate(o.op)) {
              res.op = CmpInst::Predicate(res.op & o.op);
            } else if (llvm::CmpInst::isIntPredicate(res.op)
                       && llvm::CmpInst::isIntPredicate(o.op)) {
              res.op = bits_to_icmpop(icmpop_to_bits(res.op) & icmpop_to_bits(o.op));
            } else {
              return false; // Mixing fp and int predicates
            }
            res.normalise();
            return res;
          }
          if (llvm::isa<llvm::ConstantInt>(res.rhs) && llvm::isa<llvm::ConstantInt>(o.rhs)) {
            assert((llvm::cast<llvm::ConstantInt>(res.rhs)->getValue()
                    != llvm::cast<llvm::ConstantInt>(o.rhs)->getValue()));
            if (res.op == CmpInst::ICMP_EQ || o.op == CmpInst::ICMP_EQ) {
              if (res.op != CmpInst::ICMP_EQ) std::swap(o, res);
              const llvm::APInt &RR = llvm::cast<llvm::ConstantInt>(res.rhs)->getValue();
              const llvm::APInt &OR = llvm::cast<llvm::ConstantInt>(o.rhs)->getValue();
              if (check_predicate_satisfaction(RR, o.op, OR)) {
                return res;
              } else {
                return false;
              }
            }
            if (res.op == CmpInst::ICMP_NE || o.op == CmpInst::ICMP_NE) {
              if (o.op != CmpInst::ICMP_NE) std::swap(o, res);
              const llvm::APInt &RR = llvm::cast<llvm::ConstantInt>(res.rhs)->getValue();
              const llvm::APInt &OR = llvm::cast<llvm::ConstantInt>(o.rhs)->getValue();
              if (check_predicate_satisfaction(OR, res.op, RR)) {
                return false; /* Possible to refine: we'd have to
                               * exclude OR from res somehow */
              } else {
                return res;
              }
            }
            {
              const llvm::APInt &RR = llvm::cast<llvm::ConstantInt>(res.rhs)->getValue();
              const llvm::APInt &OR = llvm::cast<llvm::ConstantInt>(o.rhs)->getValue();
              assert(res.op != CmpInst::ICMP_EQ && res.op != CmpInst::ICMP_NE);
              if (CmpInst::isIntPredicate(res.op)
                  && (o.op == res.op || o.op == icmpop_invert_strictness(res.op))) {
                  if ((check_predicate_satisfaction(RR, res.op, OR))) return o;
                  else return res;
              }
            }
          }
        }

        return false;
      }
      BinaryPredicate &operator&=(const BinaryPredicate &other) {
        return *this = (*this & other);
      }

      bool operator!=(const BinaryPredicate &other) const { return !(*this == other); }
      bool operator==(const BinaryPredicate &other) const {
        assert(!(is_true() || is_false()) || (lhs == nullptr && rhs == nullptr));
        return lhs == other.lhs && rhs == other.rhs && op == other.op;
      }
      bool operator<(const BinaryPredicate &other) const {
        if (other.is_true() && !is_true()) return true;
        if (is_false() && !other.is_false()) return true;
        return false;
      }
      bool operator<=(const BinaryPredicate &other) const {
        return *this == other || *this < other;
      }
    } pred;

    /* Earliest location that can support an insertion. nullptr means
     * that any location is fine. */
    llvm::Instruction *insertion_point = nullptr;

    BinaryPredLoc() {}
    BinaryPredLoc(bool static_pred, llvm::Instruction *insertion_point = nullptr)
      : pred(static_pred), insertion_point(static_pred ? insertion_point : nullptr) {}
    BinaryPredLoc(BinaryPredicate pred, llvm::Instruction *insertion_point = nullptr)
      : pred(std::move(pred)), insertion_point(pred.is_false() ? nullptr : insertion_point) {}
    BinaryPredLoc(llvm::CmpInst::Predicate op, llvm::Value *lhs,
                    llvm::Value *rhs) : pred(op, lhs, rhs) {}
    // BinaryPredLoc(BinaryPredicate pred, llvm::Instruction *insertion_point)
    //   : pred(std::move(pred)), insertion_point(insertion_point) {}

    /* A BinaryPredLoc must be normalised after direct modification of its members. */
    void normalise() {
      pred.normalise();
      if (pred.is_false()) insertion_point = nullptr;
    }

    bool is_true() const { return pred.is_true() && !insertion_point; }
    bool is_false() const {
      assert(!(pred.is_false() && insertion_point));
      return pred.is_false();
    }
    BinaryPredLoc negate() const {
      BinaryPredLoc res = *this;
      res.pred = pred.negate();
      return res;
    }

    // IDEA: Use operator| for join (and operator& for meet, if needed),
    // unless it's confusing
    BinaryPredLoc operator&(const BinaryPredLoc &other) const {
      BinaryPredLoc res;
      if (!insertion_point) res.insertion_point = other.insertion_point;
      else if (!other.insertion_point) res.insertion_point = insertion_point;
      else if (insertion_point == other.insertion_point) res.insertion_point = insertion_point;
      else if (DominatorTree->dominates(insertion_point, other.insertion_point)) {
        res.insertion_point = other.insertion_point;
      } else if (DominatorTree->dominates(other.insertion_point, insertion_point)) {
        res.insertion_point = insertion_point;
      } else {
        /* TODO: We have to find the least common denominator */
        llvm::dbgs() << "Meeting insertion points general case not implemented:\n";
        llvm::dbgs() << "    " << *insertion_point << "\n"
                     << " and" << *other.insertion_point << "\n";
        // assert(false);
        return false; // Underapproximating for now
      }

      res.pred = pred & other.pred;
      if (res.pred.is_false()) res.insertion_point = nullptr; // consistency
      return res;
    }
    BinaryPredLoc &operator&=(const BinaryPredLoc &other) {
      return *this = (*this & other);
    }

  private:
    llvm::Instruction *join_insertion_points(llvm::Instruction *other) const {
      llvm::Instruction *me = this->insertion_point;
      if (!me || !other) return nullptr;
      if (DominatorTree->dominates(other, me)) {
        assert(other != me);
        return other;
      }
      return me;
    }
  protected:
    friend struct PurityCondition;
    BinaryPredLoc operator|(const BinaryPredLoc &other) const {
      if (other.is_false()) return *this;
      if (is_false()) return other;
      if (pred.lhs == other.pred.lhs && pred.rhs == other.pred.rhs) {
        /* Maybe this can be made more precise. F.ex. <= | >= is also
         * true. There might be such a check in LLVM already. */
        if (llvm::CmpInst::getInversePredicate(pred.op) == other.pred.op) {
          return BinaryPredLoc(true, join_insertion_points(other.insertion_point));
        } else if (pred == other.pred) {
          BinaryPredLoc res = *this;
          res.insertion_point = join_insertion_points(other.insertion_point);
          return res;
        }
      }
      /* XXX: We have to underapproximate :( */
      if (*this < other) return other;
      else return *this;
    }
    BinaryPredLoc &operator|=(const BinaryPredLoc &other) {
      return *this = (*this | other);
    }

  public:
    bool operator!=(const BinaryPredLoc &other) const { return !(*this == other); }
    bool operator==(const BinaryPredLoc &other) const {
      return pred == other.pred && insertion_point == other.insertion_point;
    }
    bool operator<(const BinaryPredLoc &other) const {
      if (is_false()) return !other.is_false();
      else if (!other.insertion_point && insertion_point /* < */) return pred <= other.pred;
      else if (!insertion_point && other.insertion_point /* > */) return false;
      else if (insertion_point == other.insertion_point /* == */) return pred < other.pred;
      else if (insertion_point && other.insertion_point) {
        if (DominatorTree->dominates(other.insertion_point, insertion_point) /* < */) {
          return pred <= other.pred;
        }
        return false; /* Incomparable */
      }

      llvm_unreachable("All cases of insertion_point comparision covered above");
    }

  };

  struct PurityCondition {
    PurityCondition(BinaryPredLoc cond = true) {
      if (!cond.is_false()) condset.insert_gt(cond);
    }
    PurityCondition(bool b){
      if (b) condset.insert_gt(b);
    }

    bool is_true() const { return condset.size() == 1 && condset[0].is_true(); }
    bool is_false() const { return condset.empty(); }
    auto begin() const { return condset.begin(); }
    auto end() const { return condset.end(); }
    bool operator!=(const PurityCondition &other) { return !(*this == other); }
    bool operator==(const PurityCondition &other) {
      return condset == other.condset;
    }
    bool operator<(const PurityCondition &other) const {
      bool found_smaller = condset.size() < other.condset.size();
      for (const BinaryPredLoc &c : condset) {
        bool found_leq = false;
        for (const BinaryPredLoc &r : other.condset) {
          if (r < c) return false;
          if (c < r) {
            found_smaller = found_leq = true;
            break;
          }
          if (c == r) {
            found_leq = true;
            break;
          }
        }
        if (!found_leq) return false;
      }
      return found_smaller;
    }

    PurityCondition operator|(const PurityCondition &other) const {
      PurityCondition res = *this;
      res.addConds(other.condset);
      return res;
    }

    PurityCondition operator&(const PurityCondition &other) const {
      /* Oh god */

      /* Step one, pick out common conditions */
      auto lhs = condset;
      auto rhs = other.condset;
      PurityCondition res(false);
      assert(!res.is_true() && res.is_false());
      for (unsigned l = 0; l < unsigned(lhs.size());) {
        if (rhs.erase(lhs[l]) // || erase_greater(rhs, lhs[l])
            ) {
          res.addCond(lhs[l]);
          lhs.erase_at(l);
        } else {
          ++l;
        }
      }
      // for (unsigned r = 0; r < unsigned(rhs.size());) {
      //   if (erase_greater(lhs, rhs[r])) {
      //     res.addCond(rhs[r]);
      //     rhs.erase_at(r);
      //   } else {
      //     ++r;
      //   }
      // }

      /* If either is now false, we can short-circuit */
      if (lhs.empty() || rhs.empty()) return res;

      if (lhs.size() == 1) {
        for (const BinaryPredLoc &r : rhs)
          res.addCond(r & lhs[0]);
      } else {
        /* TODO: Improve step 2 */
        BinaryPredLoc rhsdisj = false;
        for (const BinaryPredLoc &r : rhs) rhsdisj |= r;
        for (const BinaryPredLoc &l : lhs)
          res.addCond(l & rhsdisj);
      }

      return res;
    }

    PurityCondition &operator&=(const PurityCondition &other) {
      return *this = (*this & other);
    }


    PurityCondition map(std::function<BinaryPredLoc(const BinaryPredLoc &)> f) const {
      auto vec = condset.get_vector();
      for (BinaryPredLoc &term : vec)
        term = f(term);

      PurityCondition res(false);
      for (BinaryPredLoc &term : vec)
        res.addCond(std::move(term));
      return res;
    };

  private:
    struct LexicalCompare {
      auto tupleit(const BinaryPredLoc &p) const {
        return std::make_tuple(p.insertion_point, p.pred.lhs, p.pred.op, p.pred.rhs);
      }
      bool operator()(const BinaryPredLoc &a, const BinaryPredLoc &b) const {
        return tupleit(a) < tupleit(b);
      };
    };

    void addCond(const BinaryPredLoc &cond) {
      if (cond.is_false()) return; /* Keep it normalised */
      std::vector<BinaryPredLoc> newset;
      for (const BinaryPredLoc &c : condset) {
        if (cond < c) return;
        if (!(c < cond)) newset.push_back(c);
      }
      condset = std::move(newset);
      condset.insert(cond);
    };
    void addConds(const VecSet<BinaryPredLoc, LexicalCompare> &conds) {
      for(const BinaryPredLoc &cond : conds)
        addCond(cond); /* Can be more efficient */
    };

    static bool erase_greater(VecSet<BinaryPredLoc, LexicalCompare> &set,
                             const BinaryPredLoc &c) {
      bool erased = false;
      for (unsigned i = 0; i < unsigned(set.size());) {
        if (c < set[i]) {
          set.erase_at(i);
          erased = true;
        } else ++i;
      }
      return erased;
    }

    VecSet<BinaryPredLoc, LexicalCompare> condset;
  };
  typedef std::unordered_map<const llvm::BasicBlock*,PurityCondition> PurityConditions;

  const char *getPredicateName(llvm::CmpInst::Predicate pred) {
    using llvm::ICmpInst; using llvm::FCmpInst;
    switch (pred) {
    default:                   return "unknown";
    case FCmpInst::FCMP_FALSE: return "false";
    case FCmpInst::FCMP_OEQ:   return "oeq";
    case FCmpInst::FCMP_OGT:   return "ogt";
    case FCmpInst::FCMP_OGE:   return "oge";
    case FCmpInst::FCMP_OLT:   return "olt";
    case FCmpInst::FCMP_OLE:   return "ole";
    case FCmpInst::FCMP_ONE:   return "one";
    case FCmpInst::FCMP_ORD:   return "ord";
    case FCmpInst::FCMP_UNO:   return "uno";
    case FCmpInst::FCMP_UEQ:   return "ueq";
    case FCmpInst::FCMP_UGT:   return "ugt";
    case FCmpInst::FCMP_UGE:   return "uge";
    case FCmpInst::FCMP_ULT:   return "ult";
    case FCmpInst::FCMP_ULE:   return "ule";
    case FCmpInst::FCMP_UNE:   return "une";
    case FCmpInst::FCMP_TRUE:  return "true";
    case ICmpInst::ICMP_EQ:    return "eq";
    case ICmpInst::ICMP_NE:    return "ne";
    case ICmpInst::ICMP_SGT:   return "sgt";
    case ICmpInst::ICMP_SGE:   return "sge";
    case ICmpInst::ICMP_SLT:   return "slt";
    case ICmpInst::ICMP_SLE:   return "sle";
    case ICmpInst::ICMP_UGT:   return "ugt";
    case ICmpInst::ICMP_UGE:   return "uge";
    case ICmpInst::ICMP_ULT:   return "ult";
    case ICmpInst::ICMP_ULE:   return "ule";
    }
  }

  llvm::Instruction::OtherOps getPredicateOpcode(llvm::CmpInst::Predicate pred) {
    using llvm::ICmpInst; using llvm::FCmpInst;
    switch (pred) {
    case ICmpInst::ICMP_EQ:  case ICmpInst::ICMP_NE:
    case ICmpInst::ICMP_SGT: case ICmpInst::ICMP_SGE:
    case ICmpInst::ICMP_SLT: case ICmpInst::ICMP_SLE:
    case ICmpInst::ICMP_UGT: case ICmpInst::ICMP_UGE:
    case ICmpInst::ICMP_ULT: case ICmpInst::ICMP_ULE:
      return llvm::Instruction::OtherOps::ICmp;
    case FCmpInst::FCMP_FALSE: case FCmpInst::FCMP_TRUE:
      assert(false && "Predicates false & true should not generate cmp insts");
    case FCmpInst::FCMP_OEQ: case FCmpInst::FCMP_OGT:
    case FCmpInst::FCMP_OGE: case FCmpInst::FCMP_OLT:
    case FCmpInst::FCMP_OLE: case FCmpInst::FCMP_ONE:
    case FCmpInst::FCMP_ORD: case FCmpInst::FCMP_UNO:
    case FCmpInst::FCMP_UEQ: case FCmpInst::FCMP_UGT:
    case FCmpInst::FCMP_UGE: case FCmpInst::FCMP_ULT:
    case FCmpInst::FCMP_ULE: case FCmpInst::FCMP_UNE:
      return llvm::Instruction::OtherOps::FCmp;
    case ICmpInst::BAD_ICMP_PREDICATE:
    case FCmpInst::BAD_FCMP_PREDICATE:
      (void)0; // fallthrough
    }
    llvm::dbgs() << "Predicate " << pred << " unknown!\n";
    assert(false && "unknown predicate"); abort();
  }

  llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const BinaryPredLoc &cond) {
    if (cond.pred.is_true()) os << "true";
    else if (cond.pred.is_false()) os << "false";
    else {
      cond.pred.lhs->printAsOperand(os);
      os << " " << getPredicateName(cond.pred.op) << " ";
      cond.pred.rhs->printAsOperand(os);
    }
    if (cond.insertion_point) {
      os << " before " << *cond.insertion_point;
    }
    return os;
  }
  llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const PurityCondition &cond) {
    if (cond.is_false()) os << "false";
    else {
      for (auto it = cond.begin(); it != cond.end();++it) {
        if (it != cond.begin()) os << " || ";
        os << *it;
      }
    }
    return os;
  }

  llvm::Instruction *maybeFindUserLocationOrNull(llvm::User *U) {
    return llvm::dyn_cast_or_null<llvm::Instruction>(U);
  }

  llvm::Instruction *maybeFindValueLocation(llvm::Value *V) {
    return llvm::dyn_cast<llvm::Instruction>(V);
  }

  void maybeResolvePhi(llvm::Value *&V, const llvm::BasicBlock *From,
                       const llvm::BasicBlock *To) {
    llvm::PHINode *N = llvm::dyn_cast_or_null<llvm::PHINode>(V);
    if (!N || N->getParent() != To) return;
    V = N->getIncomingValueForBlock(From);
  }

  void collapseTautologies(BinaryPredLoc &cond) {
    if (cond.pred.is_true() || cond.pred.is_false()) return;
    if (cond.pred.rhs == cond.pred.lhs) {
      if (llvm::CmpInst::isFalseWhenEqual(cond.pred.op)) cond.pred = false;
      if (llvm::CmpInst::isTrueWhenEqual(cond.pred.op))  cond.pred = true;
    }
    if (llvm::ConstantInt *LHS = llvm::dyn_cast_or_null<llvm::ConstantInt>(cond.pred.lhs)) {
      if (llvm::ConstantInt *RHS = llvm::dyn_cast_or_null<llvm::ConstantInt>(cond.pred.rhs)) {
        if (check_predicate_satisfaction(LHS->getValue(), cond.pred.op, RHS->getValue())) {
          cond.pred = true;
        } else  {
          cond = false;
        }
      }
    }

    cond.normalise();
  }

  llvm::Value *maybeGetExtractValueAggregate
  (llvm::Value *I, unsigned OnlyIfIndex) {
    auto *EV = llvm::dyn_cast<llvm::ExtractValueInst>(I);
    if (!EV || EV->getNumIndices() != 1 || EV->getIndices()[0] != OnlyIfIndex)
      return nullptr;
    return EV->getAggregateOperand();
  }

  /* Does not check that def-use order is respected, only for global
   * side effects */
  bool mayReorder(const llvm::Instruction *I, const llvm::Instruction *J) {
    /* Crude but hopefully safe */
    if (J->mayReadOrWriteMemory()) return false;
    return true;
  }

  bool deepPointerComparison(const llvm::Value *P1, const llvm::Value *P2) {
    if (P1 == P2) return true;

    /* Hail-mary */
    if (auto *I1 = llvm::dyn_cast<llvm::Instruction>(P1)) {
      if (I1->mayReadOrWriteMemory()) return false; /* Two identical loads, f.ex. */
      auto *I2 = llvm::dyn_cast<llvm::Instruction>(P2);
      return I2 && I1->isIdenticalTo(I2);
    }
    return false;
  }

  bool isPermissibleLeak(const llvm::Loop *L, const llvm::PHINode *Phi,
                         llvm::Instruction *LoopCarried) {
    auto *CmpXchg = llvm::dyn_cast_or_null<llvm::AtomicCmpXchgInst>
      (maybeGetExtractValueAggregate(LoopCarried, 0));
    if (!CmpXchg) {
      /* Sometimes, clang generates this odd pattern where the expected
       * value is only assigned when the cmpxchg fails */
      if (auto *CPhi = llvm::dyn_cast<llvm::PHINode>(LoopCarried)) {
        if (CPhi->getNumIncomingValues() != 2) return false;
        for (unsigned predi = 0; predi < 2; ++predi) {
          CmpXchg = llvm::dyn_cast_or_null<llvm::AtomicCmpXchgInst>
            (maybeGetExtractValueAggregate(CPhi->getIncomingValue(predi), 0));
          if (!CmpXchg) continue;
          llvm::BasicBlock *Pred = CPhi->getIncomingBlock(predi);
          llvm::BasicBlock *PPred = Pred->getSinglePredecessor();
          if (!PPred) return false;
          auto *Term = llvm::dyn_cast<llvm::BranchInst>(PPred->getTerminator());
          if (!Term || !Term->isConditional()) return false;
          if (maybeGetExtractValueAggregate(Term->getCondition(), 1) != CmpXchg)
            return false;
        }
      }
      /* Do we need else if (auto *Select = ...) ? */
    }
    if (!CmpXchg) return false;
    llvm::Value *Ptr = CmpXchg->getPointerOperand();
    /* Note, we do not check the location of the cmpxchg because no more
     * than one value can permissibly be carried along a backedge, and
     * so we are always allowed to do the "code motion" */

    for (llvm::BasicBlock *Entering : llvm::predecessors(L->getHeader())) {
      if (L->contains(Entering)) continue; // Not an entering block
      llvm::Value *V = Phi->getIncomingValueForBlock(Entering);
      llvm::LoadInst *Ld = llvm::dyn_cast<llvm::LoadInst>(V);
      if (!Ld) return false;
      if (!deepPointerComparison(Ld->getPointerOperand(), Ptr)) return false;
      if (Ld->getParent() != Entering) return false;
      for (llvm::Instruction *I = Ld; I != Entering->getTerminator();){
        I = I->getNextNode();
        if (!mayReorder(Ld, I)) return false;
      }
    }
    return true;
  }

  PurityCondition getIn(const llvm::Loop *L, PurityConditions &conds,
                        const llvm::BasicBlock *From,
                        const llvm::BasicBlock *To) {
    if (To == L->getHeader()) {
      /* Check for data leaks along the back edge */
      // llvm::dbgs() << "Checking " << From->getName() << "->" << To->getName()
      //              << " for escaping phis: \n";
      for (const llvm::Instruction *I = &*To->begin();
           I != To->getFirstNonPHI(); I = I->getNextNode()) {
        const llvm::PHINode *Phi = llvm::cast<llvm::PHINode>(I);
        llvm::Value *V = Phi->getIncomingValueForBlock(From);
        // llvm::dbgs() << "  "; Phi->printAsOperand(llvm::dbgs());
        // llvm::dbgs() << ": "; V->printAsOperand(llvm::dbgs()); llvm::dbgs() << "\n";
        if (llvm::Instruction *VI = maybeFindValueLocation(V)) {
          if (L->contains(VI)) {
            if (isPermissibleLeak(L, Phi, VI)) {
              continue;
            }
            // llvm::dbgs() << "  leak: "; V->printAsOperand(llvm::dbgs());
            // llvm::dbgs() << " through "; Phi->printAsOperand(llvm::dbgs());
            // llvm::dbgs() << "\n";
            return false; /* Leak */
          }
        }
      }
      return true;
    }
    if (!L->contains(To)) return false;
    PurityCondition in = conds[To].map([From, To](BinaryPredLoc term) {
      maybeResolvePhi(term.pred.rhs, From, To);
      maybeResolvePhi(term.pred.lhs, From, To);
      term.normalise();
      collapseTautologies(term);
      return term;
    });
    return in;
  }

  BinaryPredLoc getBranchCondition(llvm::Value *cond) {
    assert(cond && llvm::isa<llvm::IntegerType>(cond->getType())
           && llvm::cast<llvm::IntegerType>(cond->getType())->getBitWidth() == 1);
    if (auto *cmp = llvm::dyn_cast<llvm::CmpInst>(cond)) {
        return BinaryPredLoc(cmp->getPredicate(), cmp->getOperand(0),
                             cmp->getOperand(1));
    } else if (auto *bop = llvm::dyn_cast<llvm::BinaryOperator>(cond)) {
      if (bop->getOpcode() == llvm::Instruction::Xor) {
        llvm::Value *lhs = bop->getOperand(0), *rhs = bop->getOperand(1);
        if (llvm::isa<llvm::ConstantInt>(lhs) || llvm::isa<llvm::ConstantInt>(rhs)) {
          if (!llvm::isa<llvm::ConstantInt>(rhs)) std::swap(lhs, rhs);
          llvm::ConstantInt *rhsi = llvm::cast<llvm::ConstantInt>(rhs);
          if (rhsi->getValue() == 0) {
            return getBranchCondition(lhs);
          } else {
            assert(rhsi->getValue() == 1);
            return getBranchCondition(lhs).negate();
          }
        }
      }
    }
    return BinaryPredLoc(llvm::CmpInst::ICMP_EQ, cond,
                         llvm::ConstantInt::getTrue(cond->getContext()));
  }

  PurityCondition computeOut(const llvm::Loop *L, PurityConditions &conds,
                             const llvm::BasicBlock *BB) {
    if (auto *branch = llvm::dyn_cast<llvm::BranchInst>(BB->getTerminator())) {
      if (branch->isConditional()) {
        llvm::BasicBlock *trueSucc = branch->getSuccessor(0);
        llvm::BasicBlock *falseSucc = branch->getSuccessor(1);
        PurityCondition trueIn = getIn(L, conds, BB, trueSucc);
        PurityCondition falseIn = getIn(L, conds, BB, falseSucc);
        if (trueIn == falseIn) return trueIn;
        // If the operands are in the loop, we can check them for
        // purity; if not, the condition should just be false. That way,
        // we won't waste good conditions in the overapproximations
        BinaryPredLoc brCond = getBranchCondition(branch->getCondition());
        collapseTautologies(brCond);
        if (!L->contains(trueSucc)) {
          assert(trueIn.is_false());
          if (falseIn.is_true()) return brCond.negate();
          return falseIn & BinaryPredLoc(true, &*falseSucc->getFirstInsertionPt());
        }
        if (!L->contains(falseSucc)) {
          assert(falseIn.is_false());
          if (trueIn.is_true()) return brCond;
          return trueIn & BinaryPredLoc(true, &*trueSucc->getFirstInsertionPt());
        }
        // if (trueIn.is_true()) return falseIn | brCond;
        // if (falseIn.is_true()) return trueIn | brCond.negate();
        // llvm::dbgs() << "   lhs: " << (falseIn | brCond) << "\n";
        // llvm::dbgs() << "    trueIn: " << trueIn << "\n";
        // llvm::dbgs() << "    brCond.negate(): " << brCond.negate() << "\n";
        // llvm::dbgs() << "   rhs: " << (trueIn | brCond.negate()) << "\n";
        // assert(!(trueIn | brCond.negate()).is_false() || (trueIn.is_false() && brCond.is_false()));
        return (falseIn | brCond) & (trueIn | brCond.negate());
      }
    }

    /* Generic implementation; no concern for branch conditions */
    PurityCondition cond;
    for (const llvm::BasicBlock *s : llvm::successors(BB)) {
      cond &= getIn(L, conds, BB, s);
    }
    return cond;
  }

  bool isSafeToLoadFromPointer(const llvm::Value *V) {
    return llvm::isa<llvm::GlobalValue>(V)
      || llvm::isa<llvm::AllocaInst>(V);
  }

  PurityCondition instructionPurity(llvm::Loop *L, llvm::Instruction &I) {
    if (!I.mayReadOrWriteMemory()
        && llvm::isSafeToSpeculativelyExecute(&I)) return true;
    if (llvm::isa<llvm::PHINode>(I)) return true;
    if (llvm::isa<llvm::FenceInst>(I)) return true;

    if (const llvm::LoadInst *Ld = llvm::dyn_cast<llvm::LoadInst>(&I)) {
      /* No-segfaulting */
      if (isSafeToLoadFromPointer(Ld->getPointerOperand())) {
        return true;
      } else {
        return BinaryPredLoc(true, I.getNextNode());
      }
    }
    if (!I.mayWriteToMemory()) {
      if (llvm::isSafeToSpeculativelyExecute(&I)) {
        llvm::dbgs() << "Wow, a safe-to-speculate loading inst: " << I << "\n";
        return true;
      } else {
        return BinaryPredLoc(true, I.getNextNode());
      }
    }
    if (llvm::AtomicRMWInst *RMW = llvm::dyn_cast<llvm::AtomicRMWInst>(&I)) {
      BinaryPredLoc::BinaryPredicate pred(false);
      llvm::Value *arg = RMW->getValOperand();
      switch (RMW->getOperation()) {
      case llvm::AtomicRMWInst::BinOp::Add:
      case llvm::AtomicRMWInst::BinOp::Or:
      case llvm::AtomicRMWInst::BinOp::Sub:
      case llvm::AtomicRMWInst::BinOp::UMax:
      case llvm::AtomicRMWInst::BinOp::Xor:
        /* arg == 0 */
        pred = {llvm::CmpInst::ICMP_EQ, arg,
          llvm::ConstantInt::get(RMW->getType(), 0)};
        break;
      case llvm::AtomicRMWInst::BinOp::And:
      case llvm::AtomicRMWInst::BinOp::UMin:
        /* arg == 0b111...1 */
        pred = {llvm::CmpInst::ICMP_EQ, arg,
          llvm::ConstantInt::getAllOnesValue(RMW->getType())};
        break;
      case llvm::AtomicRMWInst::BinOp::Xchg:
        /* ret == arg */
        pred = {llvm::CmpInst::ICMP_EQ, &I, arg};
        break;
      }
      BinaryPredLoc ret;
      if (isSafeToLoadFromPointer(RMW->getPointerOperand())) {
        ret = BinaryPredLoc(pred);
      } else {
        ret = BinaryPredLoc(pred, I.getNextNode());
      }
      collapseTautologies(ret);
      return ret;
    }

    /* Extension: We do not yet handle when the return value of the cmpxchg is used for  */
    if (llvm::AtomicCmpXchgInst *CX = llvm::dyn_cast<llvm::AtomicCmpXchgInst>(&I)) {
      /* Try to find a ExtractValue instruction that extracts the
       * success bit */
      llvm::ExtractValueInst *success = nullptr;
      for (llvm::User *U : CX->users()) {
        if (llvm::ExtractValueInst *EV = llvm::dyn_cast<llvm::ExtractValueInst>(U)) {
          if (EV->getNumIndices() == 1 && EV->idx_begin()[0] == 1) {
            success = EV;
            break;
          }
        }
      }
      if (success) {
        BinaryPredLoc::BinaryPredicate pred
          (llvm::CmpInst::ICMP_EQ, success,
           llvm::ConstantInt::getFalse(CX->getContext()));
        if (isSafeToLoadFromPointer(CX->getPointerOperand())) {
          return BinaryPredLoc(pred);
        } else {
          return BinaryPredLoc(pred, I.getNextNode());
        }
      }
    }

    if (llvm::CallInst *CI = llvm::dyn_cast<llvm::CallInst>(&I)) {
      if (CI->getCalledFunction()->getName() == "__VERIFIER_assume") return true;
      if (may_inline) {
        auto it = may_inline->find(CI->getCalledFunction());
        if (it != may_inline->end() && it->second) {
          /* We need inlining to do this; optimistically assume purity. If
           * needed, the call will be inlined and the analysis redone. */
          inlining_needed = true;
          return true;
        }
      }
    }
    /* Extension point: We could add more instructions here */

    return false;
  }

  PurityConditions analyseLoop(llvm::Loop *L) {
    PurityConditions conds;
    const auto &blocks = L->getBlocks();
    bool changed;
    unsigned count = 0;
    // llvm::dbgs() << "Analysing " << *L;
    do {
      changed = false;
      // Assume it's in reverse postorder, or some suitable order
      for (auto it = blocks.rbegin(); it != blocks.rend(); ++it) {
        llvm::BasicBlock *BB = *it;
        PurityCondition cond = computeOut(L, conds, BB);
        // llvm::dbgs() << "  out of " << BB->getName() << ": " << cond << "\n";
        /* Skip the terminator */
        for (auto it = BB->rbegin(); ++it != BB->rend();) {
          cond &= instructionPurity(L, *it);
        }
        if (conds[BB] != cond) {
          // llvm::dbgs() << " " << BB->getName() << ": " << cond << "\n";
          changed = true;
          conds[BB] = cond;
        }
      }
      if (++count > 1000) {
        llvm::dbgs() << "Analysis of loop in "
                     << L->getHeader()->getParent()->getName()
                     << " did not terminate after 1000 iterations\n";
        abort();
      }
    } while(changed);
    return conds;
  }

  bool functionMayInline(llvm::Function &F) {
    return true;
  }

  auto getInliningCandidates(llvm::Module &M) {
    std::unordered_map<llvm::Function *, bool> local_may_inline;
    may_inline = &local_may_inline;

    llvm::CallGraph CG(M);
    std::vector<llvm::Function *> stack;

    std::function<bool(llvm::Function*)> visit
      = [&](llvm::Function *F) -> bool {
        if (F->empty())return false;
        if (std::count(stack.begin(), stack.end(), F) != 0) {
          return false; /* Circular references aren't allowed */
        }
        struct ppg {
          llvm::Function *F;
          std::vector<llvm::Function *> &S;
          ppg(llvm::Function *F, std::vector<llvm::Function *> &S)
            : F(F), S(S) { S.push_back(F); }
          ~ppg() { assert(S.back() == F); S.pop_back(); }
        } push_pop_guard(F, stack);
        for (llvm::CallGraphNode::CallRecord &callee : *CG[F]) {
          if (!visit(callee.second->getFunction())) {
            return local_may_inline[F] = false;
          }
        }

        return local_may_inline[F] = functionMayInline(*F);
      };

    for (llvm::Function &F : M.functions()) {
      if (F.empty()) continue;
      visit(&F);
    }
    may_inline = nullptr;
    return local_may_inline;
  }

  void findCallsInLoop(llvm::SmallPtrSet<llvm::CallInst*, 4> &calls,
                       llvm::Loop *L) {
    for (llvm::BasicBlock *BB : L->blocks()) {
      for (llvm::Instruction *II = &*BB->begin(); II;) {
        llvm::Instruction *I = II;
        II = II->getNextNode();
        if (llvm::CallInst *CI = llvm::dyn_cast<llvm::CallInst>(I)) {
          auto it = may_inline->find(CI->getCalledFunction());
          if (it != may_inline->end() && it->second) {
            calls.insert(CI);
          }
        }
      }
    }
  }

  bool recurseLoops(llvm::Loop *L, const std::function<bool(llvm::Loop*)> &f) {
    bool changed = false;
    for (auto it = L->begin(); it != L->end(); ++it) {
      changed |= recurseLoops(*it, f);
    }
    changed |= f(L);
    return changed;
  }

  bool foreachLoop(const llvm::LoopInfo &LI,
                   const std::function<bool(llvm::Loop*)> &f) {
    bool changed = false;
    for (auto it = LI.begin(); it != LI.end(); ++it) {
      changed |= recurseLoops(*it, f);
    }
    return changed;
  }

  auto analyseInliningNeeds(llvm::Function &F) {
    llvm::SmallPtrSet<llvm::CallInst*, 4> calls;
    llvm::LoopInfo LI(*DominatorTree);

    foreachLoop(LI, [&](llvm::Loop *L) {
      assert(!inlining_needed);
      PurityConditions conditions = analyseLoop(L);
      PurityCondition headerCond = conditions[L->getHeader()];
      if (!headerCond.is_false() && inlining_needed) {
        findCallsInLoop(calls, L);
      }
      inlining_needed = false;
      return false;
    });

    return calls;
  }

  bool doInline(llvm::Function &F,
                llvm::SmallPtrSet<llvm::CallInst*, 4> &calls) {
    for (llvm::CallInst *CI : calls) {
      llvm::InlineFunctionInfo ifi;
      // llvm::dbgs() << "Inlining call to " << CI->getCalledFunction()->getName()
      //              << " in " << F.getName() << "\n";
#if LLVM_VERSION_MAJOR >= 11
      llvm::InlineResult res = llvm::InlineFunction(*CI, ifi);
      assert(res.isSuccess());
#else
      bool success = llvm::InlineFunction(CI, ifi);
      assert(success);
#endif
      assert(ifi.InlinedCalls.size() == 0);
    }
    return !calls.empty();
  }

  bool dominates_or_equals(const llvm::DominatorTree &DT,
                           llvm::Instruction *Def, llvm::Instruction *User) {
    return Def == User || DT.dominates(Def, User);
  }

  llvm::Instruction *findInsertionPoint(llvm::Loop *L,
                                        const llvm::DominatorTree &DT,
                                        const BinaryPredLoc &cond) {
    if (llvm::Instruction *IPT = cond.insertion_point) {
      if (llvm::Instruction *LHS = maybeFindUserLocationOrNull
          (llvm::dyn_cast_or_null<llvm::User>(cond.pred.lhs))) {
        if (llvm::Instruction *RHS = maybeFindUserLocationOrNull
            (llvm::dyn_cast_or_null<llvm::User>(cond.pred.rhs))) {
          if (DT.dominates(LHS, IPT) &&
              DT.dominates(RHS, IPT)) return IPT;
          if (dominates_or_equals(DT, IPT, RHS->getNextNode()) &&
              dominates_or_equals(DT, LHS, RHS)) return RHS->getNextNode();
          if (dominates_or_equals(DT, IPT, LHS->getNextNode()) &&
              dominates_or_equals(DT, RHS, LHS)) return LHS->getNextNode();
          /* uh oh, hard case */
          assert(false && "Implement me"); abort();
        } else {
          if (DT.dominates(LHS, IPT)) return IPT;
          if (dominates_or_equals(DT, IPT, LHS->getNextNode())) return LHS->getNextNode();
          /* uh oh, hard case */
          assert(false && "Implement me"); abort();
        }
      } else if (llvm::Instruction *RHS = maybeFindUserLocationOrNull
            (llvm::dyn_cast_or_null<llvm::User>(cond.pred.rhs))) {
          if (DT.dominates(LHS, IPT)) return IPT;
          if (dominates_or_equals(DT, IPT, LHS->getNextNode())) return LHS->getNextNode();
          /* uh oh, hard case */
          assert(false && "Implement me"); abort();
      } else {
        return IPT;
      }
    } else {
      if (llvm::Instruction *LHS = maybeFindUserLocationOrNull
          (llvm::dyn_cast_or_null<llvm::User>(cond.pred.lhs))) {
        if (llvm::Instruction *RHS = maybeFindUserLocationOrNull
            (llvm::dyn_cast_or_null<llvm::User>(cond.pred.rhs))) {
          if (dominates_or_equals(DT, LHS, RHS)) return RHS->getNextNode();
          if (dominates_or_equals(DT, RHS, LHS)) return LHS->getNextNode();
          /* uh oh, hard case */
          assert(false && "Implement me"); abort();
        } else {
          return LHS->getNextNode();
        }
      } else if (llvm::Instruction *RHS = maybeFindUserLocationOrNull
                 (llvm::dyn_cast_or_null<llvm::User>(cond.pred.rhs))) {
        return RHS->getNextNode();
      } else {
        return &*L->getHeader()->getFirstInsertionPt();
      }
    }
    llvm_unreachable("All cases covered in findInsertionPoint");
  }

  bool runOnLoop(llvm::Loop *L) {
    // Debug::warn(("plp.code." + L->getHeader()->getParent()->getName()).str())
    //   << "Analysing " << L->getHeader()->getParent()->getName() << ":\n"
    //   << *L->getHeader()->getParent();
    assert(!may_inline);
    assert(!inlining_needed);
    PurityConditions conditions = analyseLoop(L);
    PurityCondition headerCond = conditions[L->getHeader()];
    assert(!inlining_needed);
    if (headerCond.is_false()) {
      // llvm::dbgs() << "Loop " << L->getHeader()->getParent()->getName() << ":"
      //              << *L << " isn't pure\n";
      return false;
    } else {
      // llvm::dbgs() << "Partially pure loop found in "
      //              << L->getHeader()->getParent()->getName() << "():\n";
      // L->print(llvm::dbgs(), 2);
      // llvm::dbgs() << " Purity condition: " << headerCond << "\n";
    }

    for (const BinaryPredLoc &term : headerCond) {
      llvm::Instruction *I = findInsertionPoint(L, *DominatorTree, term);
      //llvm::dbgs() << " Insertion point: " << *I << "\n";
      llvm::Value *Cond;
      if (term.pred.is_true()) {
        Cond = llvm::ConstantInt::getTrue(L->getHeader()->getContext());
      } else {
        Cond = llvm::ICmpInst::Create
          (getPredicateOpcode(term.pred.op),
           llvm::CmpInst::getInversePredicate(term.pred.op),
           term.pred.lhs, term.pred.rhs, "negated.pp.cond", I);
      }
      llvm::Function *F_assume = L->getHeader()->getParent()->getParent()
        ->getFunction("__VERIFIER_assume");
      {
        llvm::Type *arg_ty = F_assume->arg_begin()->getType();
        assert(arg_ty->isIntegerTy());
        if(arg_ty->getIntegerBitWidth() != 1){
          Cond = new llvm::ZExtInst(Cond, arg_ty,"",I);
        }
      }
      llvm::CallInst::Create(F_assume,{Cond},"",I);
    }

    // llvm::dbgs() << "Rewritten:\n";
    // llvm::dbgs() << *L->getHeader()->getParent();;

    return true;
  }
}

void PartialLoopPurityPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const{
  AU.addRequired<DeclareAssumePass>();
  AU.addPreserved<DeclareAssumePass>();
}

bool PartialLoopPurityPass::runOnModule(llvm::Module &M) {
  bool changed = false;

  auto local_may_inline = getInliningCandidates(M);
  may_inline = &local_may_inline;

  for (llvm::Function &F : M.functions()) {
    if (F.empty()) continue; /* We do nothing on declarations */
    while(true) {
      llvm::DominatorTree DT(F);
      DominatorTree = &DT;
      auto calls = analyseInliningNeeds(F);
      if (!doInline(F, calls)) break;
      changed = true;
    }
    assert(!llvm::verifyFunction(F, &llvm::dbgs()));
  }

  may_inline = nullptr;

  for (llvm::Function &F : M.functions()) {
    if (F.empty()) continue; /* We do nothing on declarations */
    llvm::DominatorTree DT(F);
    DominatorTree = &DT;
    llvm::LoopInfo LI(DT);
    changed |= foreachLoop(LI, runOnLoop);
    assert(!llvm::verifyFunction(F, &llvm::dbgs()));
  }

  DominatorTree = nullptr;
  return changed;
}

char PartialLoopPurityPass::ID = 0;
static llvm::RegisterPass<PartialLoopPurityPass> X
("partial-loop-purity",
 "Bound pure and partially pure loops with __VERIFIER_assumes.");
