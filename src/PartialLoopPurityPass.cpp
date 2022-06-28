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

#include "PartialLoopPurityPass.h"

#include "CheckModule.h"
#include "Debug.h"
#include "Option.h"
#include "SpinAssumePass.h"
#include "vecset.h"

#include <boost/container/flat_map.hpp>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/Analysis/ValueTracking.h>
#if defined(HAVE_LLVM_IR_DOMINATORS_H)
#include <llvm/IR/Dominators.h>
#elif defined(HAVE_LLVM_ANALYSIS_DOMINATORS_H)
#include <llvm/Analysis/Dominators.h>
#endif
#include <llvm/Config/llvm-config.h>
#if defined(HAVE_LLVM_IR_FUNCTION_H)
#include <llvm/IR/Function.h>
#elif defined(HAVE_LLVM_FUNCTION_H)
#include <llvm/Function.h>
#endif
#include <llvm/IR/Constants.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Verifier.h>
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
#include <llvm/Pass.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/Cloning.h>

#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#ifdef LLVM_HAS_TERMINATORINST
typedef llvm::TerminatorInst TerminatorInst;
#else
typedef llvm::Instruction TerminatorInst;
#endif

namespace {
  const int MAX_CONJUNCTION_TERMS = 5;
  const int MAX_DISJUNCTION_TERMS = 10;

  /* Not reentrant */
  static const llvm::DominatorTree *DominatorTree;
  static const llvm::Loop *Loop;
  static const struct RPO *LoopRPO;
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

  llvm::CmpInst::Predicate bits_to_icmpop(std::uint_fast8_t bits, bool &ua) {
    using llvm::CmpInst;
    if ((bits & 0b01110) == 0b01110) return CmpInst::FCMP_TRUE; // Not expected to happen
    if ((bits & 0b10101) == 0b10101) return CmpInst::FCMP_TRUE; // Not expected to happen
    if ((bits & 0b01100) == 0b01100) { ua |= bits != 0b01100; return CmpInst::ICMP_SLE; }
    if ((bits & 0b00110) == 0b00110) { ua |= bits != 0b00110; return CmpInst::ICMP_SGE; }
    if ((bits & 0b10100) == 0b10100) { ua |= bits != 0b10100; return CmpInst::ICMP_ULE; }
    if ((bits & 0b00101) == 0b00101) { ua |= bits != 0b00101; return CmpInst::ICMP_UGE; }
    if ((bits & 0b01000) == 0b01000) { ua |= bits != 0b01000; return CmpInst::ICMP_SLT; }
    if ((bits & 0b00010) == 0b00010) { ua |= bits != 0b00010; return CmpInst::ICMP_SGT; }
    if ((bits & 0b10000) == 0b10000) { ua |= bits != 0b10000; return CmpInst::ICMP_ULT; }
    if ((bits & 0b00001) == 0b00001) { ua |= bits != 0b00001; return CmpInst::ICMP_UGT; }
    if ((bits & 0b01010) == 0b01010) { ua |= bits != 0b01010; return CmpInst::ICMP_NE; }
    if ((bits & 0b10001) == 0b10001) { ua |= bits != 0b10001; return CmpInst::ICMP_NE; }
    if ((bits & 0b00100) == 0b00100) { ua |= bits != 0b00100; return CmpInst::ICMP_EQ; }
    ua |= bits != 0;
    return CmpInst::FCMP_FALSE;
  }

  llvm::CmpInst::Predicate icmpop_invert_strictness(llvm::CmpInst::Predicate pred) {
    bool underapprox = false;
    llvm::CmpInst::Predicate res = bits_to_icmpop(0b00100 ^ icmpop_to_bits(pred),
                                                  underapprox);
    assert(!underapprox);
    return res;
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

  struct BinaryPredicate {
    llvm::Value *lhs = nullptr, *rhs = nullptr;
    /* FCMP_TRUE and FCMP_FALSE are used as special values indicating
     * unconditional purity
     */
    llvm::CmpInst::Predicate op = llvm::CmpInst::FCMP_FALSE;

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

    /* Returns the conjunction of *this and other. Assigns true to
     * underapprox if the result is an underapproximation. */
    BinaryPredicate meet(const BinaryPredicate &other,
                         bool &underapprox) const {
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
            res.op = bits_to_icmpop(icmpop_to_bits(res.op) & icmpop_to_bits(o.op),
                                    underapprox);
          } else {
            underapprox = true;
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
              underapprox = true;
              return false; // Possible to refine: we'd have to
                            // exclude OR from res somehow
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
              underapprox = true; /* Maybe not always? */
              return ((check_predicate_satisfaction(RR, res.op, OR))) ? o : res;
            }
          }
        }
      }

      if (!other.is_false()) underapprox = true;
      return false;
    }

    BinaryPredicate operator&(const BinaryPredicate &other) const {
      bool underapprox = false;
      return meet(other, underapprox);
    }
    BinaryPredicate &operator&=(const BinaryPredicate &other) {
      return *this = (*this & other);
    }

    bool operator!=(const BinaryPredicate &other) const { return !(*this == other); }
    bool operator==(const BinaryPredicate &other) const {
      assert(!(is_true() || is_false()) || (lhs == nullptr && rhs == nullptr));
      return lhs == other.lhs && rhs == other.rhs && op == other.op;
    }
    bool operator<=(const BinaryPredicate &other) const {
      // Special case optimisations, should not be needed
      if (other.is_true() || is_false()) return true;
      if (*this == other) return true;
      bool underapprox = false;
      BinaryPredicate m = meet(other, underapprox);
      return m == *this;
    }
    bool operator<(const BinaryPredicate &other) const {
      return *this != other && *this <= other;
    }
  };

  struct RPO {
    std::vector<llvm::BasicBlock *> blocks;
    boost::container::flat_map<const llvm::BasicBlock *, std::size_t> block_indices;
    bool is_backedge(const llvm::BasicBlock *From, const llvm::BasicBlock *To) const {
      assert(block_indices.count(From) && block_indices.count(To));
      return block_indices.at(From) >= block_indices.at(To);
    }
    RPO(std::size_t capacity) { blocks.reserve(capacity); }
  };

  /* Encodes a restriction on where an assume may be inserted. There is
   * no bottom value, instead, operator& (which is the only operation
   * that may return bottom) uses Option<InsertionPoint>. */
  struct InsertionPoint {
    /* Earliest location that can support an insertion. nullptr means
     * that any location is fine. */
    llvm::Instruction *earliest = nullptr;

    InsertionPoint(llvm::Instruction *earliest = nullptr) : earliest(earliest) {}

    /* Reset to default value (no insertion point requirement) */
    void clear() { earliest = nullptr; }
    operator bool() const { return earliest; }
    bool is_true() const { return *this; }
    bool is_false() const { return !*this; }

    static bool isBefore(llvm::Instruction *I, llvm::Instruction *J) {
      llvm::BasicBlock *IB = I->getParent(), *JB = J->getParent();
      if (IB == JB) {
        while(true) {
          if (J == &*JB->begin()) return false;
          J = J->getPrevNode();
          if (I == J) return true;
        }
      } else {
        std::size_t IBI = LoopRPO->block_indices.at(IB),
          JBI = LoopRPO->block_indices.at(JB);
        assert (IBI != JBI);
        if (IBI > JBI) return false;
        VecSet<std::size_t> predecessors;
        while (true) {
          for (llvm::BasicBlock *Pred : llvm::predecessors(JB)) {
            if (Pred == IB) return true;
            assert(LoopRPO->block_indices.count(Pred)); // Cannot exit loop
            std::size_t PredI = LoopRPO->block_indices.at(Pred);
            // if (it == LoopRPO->block_indices.end()) continue;
            if (PredI >= IBI && PredI < JBI)
              predecessors.insert(PredI);
          }
          if (predecessors.empty()) return false;
          assert(predecessors.back() < JBI);
          JBI = predecessors.back();
          predecessors.pop_back();
          assert(IBI < JBI);
          JB = LoopRPO->blocks[JBI];
        }
        abort();
      }
    }

    Option<InsertionPoint> operator&(InsertionPoint other) const {
      InsertionPoint res;
      if (!earliest) res.earliest = other.earliest;
      else if (!other.earliest) res.earliest = earliest;
      else if (earliest == other.earliest) res.earliest = earliest;
      else if (isBefore(earliest, other.earliest)) res.earliest = other.earliest;
      else if (isBefore(other.earliest, earliest)) res.earliest = earliest;
      else return {};
      return res;
    }

    /* WARNING: Dangerous semantics, should we really do this?
     * Meets *this with other. If the result is bottom, returns false
     * and sets *this to top (sic!)
     */
    bool operator &=(InsertionPoint other) {
      if (auto res = *this & other) {
        *this = *res;
        return true;
      } else {
        clear();
        return false;
      }
    }

    InsertionPoint operator|(InsertionPoint other) const {
      return *this <= other ? other : *this;
    }
    InsertionPoint &operator|=(InsertionPoint other) {
      return *this = *this | other;
    }

    bool operator==(InsertionPoint other) const { return earliest == other.earliest; }
    bool operator!=(InsertionPoint other) const { return !(*this == other); }
    bool operator<=(InsertionPoint other) const {
      if (!earliest && other.earliest) return false;
      if (!other.earliest) return true;
      if (earliest == other.earliest) return true;
      return isBefore(other.earliest, earliest);
    }
    bool operator<(InsertionPoint other) const {
      return *this != other && *this <= other;
    }
  };

  llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const struct ConjunctionLoc &cond);
  llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const struct Disjunction &cond);

  struct ConjunctionLoc {
    ConjunctionLoc(bool static_pred, InsertionPoint insertion_point = nullptr)
      : insertion_point(static_pred ? insertion_point : nullptr)
    { addConjunct(static_pred); }
    ConjunctionLoc(BinaryPredicate pred, InsertionPoint insertion_point = nullptr)
      : insertion_point(pred.is_false() ? nullptr : insertion_point)
    { addConjunct(pred); }
    ConjunctionLoc(InsertionPoint insertion_point)
      : insertion_point(insertion_point) {}

    ConjunctionLoc operator&(const ConjunctionLoc &other) const {
      ConjunctionLoc res = *this;
      if (!(res.insertion_point &= other.insertion_point)) return false;

      res.addConjuncts(other.conjuncts);
      if (res.conjuncts.size() > MAX_CONJUNCTION_TERMS) {
        Debug::warn("plp:conjunct_limit")
          << "Notice: PLP: Bounding size of conjunction " << *this <<"\n"
          << "This does not affect correctness, but might slow down verification or\n"
          << "cause it to require loop bounding to terminate.\n";
        res = false; // It's unlikely we can do better
      }
      if (res.is_false()) res.insertion_point.clear(); // normalise
      return res;
    }
    ConjunctionLoc &operator&=(const ConjunctionLoc &other) {
      return *this = (*this & other);
    }

    bool implies(const ConjunctionLoc &other) const {
      if (!(insertion_point <= other.insertion_point)) return false;

      /* Every conjunct in other must be implied by some in this */
      for (const BinaryPredicate &o : other.conjuncts) {
        bool implied = false;
        for (const BinaryPredicate &t : conjuncts) {
          if (o <= t /* t.implies(o) */) {
            implied = true;
            break;
          }
        }
        if (!implied) return false;
      }
      return true;
    }

    bool operator!=(const ConjunctionLoc &other) const { return !(*this == other); }
    bool operator==(const ConjunctionLoc &other) const {
      return conjuncts == other.conjuncts && insertion_point == other.insertion_point;
    }
    bool operator<=(const ConjunctionLoc &other) const {
      return implies(other);
    }
    bool operator<(const ConjunctionLoc &other) const {
      return *this != other && *this <= other;
    }

    /* A ConjunctionLoc must be normalised after direct modification of its members. */
    void normalise() {
      auto terms = std::move(conjuncts).get_vector();
      conjuncts.clear();
      for (BinaryPredicate &p : terms) p.normalise();
      addConjuncts({std::move(terms)});
      if (is_false()) insertion_point = nullptr;
    }
    bool has_conjuncts() const { return !conjuncts.empty(); }
    bool is_true() const { return !has_conjuncts() && !insertion_point; }
    bool is_false() const { return conjuncts.size() == 1 && conjuncts[0].is_false(); }
    auto begin() const { return conjuncts.begin(); }
    auto end() const { return conjuncts.end(); }

    ConjunctionLoc map(std::function<BinaryPredicate(const BinaryPredicate &)> f) const {
      auto vec = conjuncts.get_vector();
      for (BinaryPredicate &term : vec)
        term = f(term);

      ConjunctionLoc res(true, insertion_point);
      for (BinaryPredicate &term : vec)
        res.addConjunct(std::move(term));
      return res;
    }

    /* Earliest location that can support an insertion. */
    InsertionPoint insertion_point;

  private:
    struct LexicalCompare {
      auto tupleit(const BinaryPredicate &p) const {
        return std::make_tuple(p.lhs, p.op, p.rhs);
      }
      bool operator()(const BinaryPredicate &a, const BinaryPredicate &b) const {
        return tupleit(a) < tupleit(b);
      }
    };

    // &=, if you will
    void addConjunct(BinaryPredicate cond) {
      if (cond.is_true()) return; /* Keep it normalised */
      std::vector<BinaryPredicate> newset;
      for (const BinaryPredicate &c : conjuncts) {
        bool underapprox = false;
        BinaryPredicate m = c.meet(cond, underapprox);
        if (m == c) return;
        if (m == cond) continue;
        if (underapprox) {
          newset.push_back(c); /* Have to keep both */
        } else {
          // Start the loop over!
#ifndef NDEBUG
          /* We're going to meet c with m again, so ensure that meet
           * recognizes that c implies m */
          bool ua2 = false;
          BinaryPredicate m2 = c.meet(m, ua2);
          assert(m2 == m && !ua2);
#endif
          return addConjunct(m);
        }
      }
      conjuncts = std::move(newset);
      conjuncts.insert(cond);
    }
    void addConjuncts(const VecSet<BinaryPredicate, LexicalCompare> &conds) {
      for(const BinaryPredicate &cond : conds)
        addConjunct(cond); /* Can be more efficient */
    }

    VecSet<BinaryPredicate, LexicalCompare> conjuncts;

    friend struct Disjunction;
  };

  struct Disjunction {
    Disjunction(InsertionPoint ip) { disjuncts.insert_gt(ip); }
    Disjunction(bool b = false){ if (b) disjuncts.insert_gt(b); }
    Disjunction(BinaryPredicate cond) {
      if (!cond.is_false()) disjuncts.insert_gt(cond);
    }
    Disjunction(ConjunctionLoc cond) {
      if (!cond.is_false()) disjuncts.insert_gt(cond);
    }

    bool is_true() const { return disjuncts.size() == 1 && disjuncts[0].is_true(); }
    bool is_false() const { return disjuncts.empty(); }
    auto begin() const { return disjuncts.begin(); }
    auto end() const { return disjuncts.end(); }
    bool operator!=(const Disjunction &othr) const { return !(*this == othr); }
    bool operator==(const Disjunction &other) const {
      return disjuncts == other.disjuncts;
    }
    bool operator<(const Disjunction &other) const {
      bool found_smaller = disjuncts.size() < other.disjuncts.size();
      for (const Elem &c : disjuncts) {
        bool found_leq = false;
        for (const Elem &r : other.disjuncts) {
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
    bool operator<=(const Disjunction &other) const {
      return *this == other || *this < other;
    }

    Disjunction operator|(const Disjunction &other) const {
      Disjunction res = *this;
      res.addConds(other.disjuncts);
      res.bound();
      return res;
    }

    Disjunction &operator|=(const Disjunction &other) {
      return *this = (*this | other);
    }

    Disjunction operator&(const Disjunction &other) const {
      /* Step one: pick out common terms */
      auto lhs = disjuncts;
      auto rhs = other.disjuncts;
      Disjunction res(false);
      assert(!res.is_true() && res.is_false());
      for (unsigned l = 0; l < unsigned(lhs.size());) {
        if (rhs.erase(lhs[l])) {
          res.addCond(lhs[l]);
          lhs.erase_at(l);
        } else {
          ++l;
        }
      }

      /* Step two: distribute over the remaining terms */
      for (const Elem &r : rhs) {
        for (const Elem &l : lhs) {
          res.addCond(r & l);
        }
      }

      res.bound();
      return res;
    }

    Disjunction &operator&=(const Disjunction &other) {
      return *this = (*this & other);
    }

    Disjunction map(std::function<BinaryPredicate(const BinaryPredicate &)> f) const {
      auto vec = disjuncts.get_vector();
      for (Elem &term : vec)
        term = term.map(f);

      Disjunction res(false);
      for (Elem &term : vec)
        res.addCond(std::move(term));
      return res;
    }

  private:
    using Elem = ConjunctionLoc;

    void bound() {
      if (disjuncts.size() > MAX_DISJUNCTION_TERMS) {
        Debug::warn("plp:disjunct_bound")
          << "Notice: PLP: Bounding size of disjunction " << *this << "\n"
          << "This does not affect correctness, but might slow down verification or\n"
          << "cause it to require loop bounding to terminate.\n";
        // Delete the terms with the largest number of conjuncts (least
        // likely to be useful)
        auto vec = std::move(disjuncts).get_vector();
        std::sort(vec.begin(), vec.end(), [](const Elem &a, const Elem &b) {
          return a.conjuncts.size() < b.conjuncts.size();
        });
        vec.resize(MAX_DISJUNCTION_TERMS, vec[0]);
        std::sort(vec.begin(), vec.end(), LexicalCompare());
        disjuncts = vec;
      }
    }

    struct LexicalCompare {
      auto tupleit(const Elem &p) const {
        return std::make_tuple(p.insertion_point, p.conjuncts);
      }
      bool operator()(const Elem &a, const Elem &b) const {
        return tupleit(a) < tupleit(b);
      }
    };

    void addCond(const Elem &cond) {
      if (cond.is_false()) return; /* Keep it normalised */
      std::vector<Elem> newset;
      for (const Elem &c : disjuncts) {
        if (cond < c) return;
        if (!(c < cond)) newset.push_back(c);
      }
      disjuncts = std::move(newset);
      disjuncts.insert(cond);
    }
    void addConds(const VecSet<Elem, LexicalCompare> &conds) {
      for(const Elem &cond : conds)
        addCond(cond); /* Can be more efficient */
    }

    static bool erase_greater(VecSet<Elem, LexicalCompare> &set,
                             const Elem &c) {
      bool erased = false;
      for (unsigned i = 0; i < unsigned(set.size());) {
        if (c < set[i]) {
          set.erase_at(i);
          erased = true;
        } else {
          ++i;
        }
      }
      return erased;
    }

    VecSet<Elem, LexicalCompare> disjuncts;
  };
  using PurityCondition = Disjunction;
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

  llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const BinaryPredicate &pred) {
    if (pred.is_true()) {
      os << "true";
    } else if (pred.is_false()) {
      os << "false";
    } else {
      pred.lhs->printAsOperand(os);
      os << " " << getPredicateName(pred.op) << " ";
      pred.rhs->printAsOperand(os);
    }
    return os;
  }
  llvm::raw_ostream &operator<<(llvm::raw_ostream &os, InsertionPoint ip) {
    if (ip.earliest) {
      os << " before " << *ip.earliest;
    }
    return os;
  }
  llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const ConjunctionLoc &cond) {
    if (!cond.has_conjuncts()) {
      os << "true";
    } else {
      for (auto it = cond.begin(); it != cond.end(); ++it) {
        if (it != cond.begin()) os << " && ";
        os << *it;
      }
    }
    return os << cond.insertion_point;
  }
  llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const PurityCondition &cond) {
    if (cond.is_false()) {
      os << "false";
    } else {
      for (auto it = cond.begin(); it != cond.end(); ++it) {
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

  BinaryPredicate collapseTautologies(const BinaryPredicate &cond) {
    if (cond.is_true() || cond.is_false()) return cond;
    if (cond.rhs == cond.lhs) {
      if (llvm::CmpInst::isFalseWhenEqual(cond.op)) return false;
      if (llvm::CmpInst::isTrueWhenEqual(cond.op))  return true;
    }
    if (llvm::ConstantInt *LHS = llvm::dyn_cast_or_null<llvm::ConstantInt>(cond.lhs)) {
      if (llvm::ConstantInt *RHS = llvm::dyn_cast_or_null<llvm::ConstantInt>(cond.rhs)) {
        return check_predicate_satisfaction(LHS->getValue(), cond.op, RHS->getValue());
      }
    }

    return cond;
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

  void RPOVisit(llvm::Loop* L, RPO &rpo,
                std::unordered_set<llvm::BasicBlock *> &visited,
                decltype(RPO::block_indices)::sequence_type &poorder,
                llvm::BasicBlock *BB) {
    if (!visited.insert(BB).second) return;
    for (llvm::BasicBlock *Succ : llvm::successors(BB)) {
      if (!L->contains(Succ)) continue;
      RPOVisit(L, rpo, visited, poorder, Succ);
    }
    rpo.blocks.push_back(BB);
    poorder.emplace_back(BB, L->getNumBlocks() - rpo.blocks.size());
  }

  RPO getLoopRPO(llvm::Loop *L) {
    RPO ret(L->getNumBlocks());
    std::unordered_set<llvm::BasicBlock *> visited(L->getNumBlocks());
    decltype(RPO::block_indices)::sequence_type poorder;
    poorder.reserve(L->getNumBlocks());

    RPOVisit(L, ret, visited, poorder, L->getHeader());
    std::reverse(ret.blocks.begin(), ret.blocks.end());
    std::sort(poorder.begin(), poorder.end());
    ret.block_indices.adopt_sequence
      (boost::container::ordered_unique_range, std::move(poorder));
    assert(ret.blocks.size() == L->getNumBlocks());
#ifndef NDEBUG
    for (std::size_t i = 0; i < ret.blocks.size(); ++i) {
      assert(ret.block_indices.at(ret.blocks[i]) == i);
    }
#endif
    return ret;
  }

  PurityCondition getIn(const llvm::Loop *L, PurityConditions &conds,
                        const RPO &rpo,
                        const llvm::BasicBlock *From,
                        const llvm::BasicBlock *To) {
    if (To == L->getHeader()) {
      /* Check for data leaks along the back edge */
      // llvm::dbgs() << "Checking " << From->getName() << "->" << To->getName()
      //              << " for escaping phis: \n";
      for (const llvm::Instruction *I = &*To->begin(),
             *End = To->getFirstNonPHI(); I != End; I = I->getNextNode()) {
        const llvm::PHINode *Phi = llvm::cast<llvm::PHINode>(I);
        llvm::Value *FromOutside = nullptr;
        for (llvm::BasicBlock *Entering : llvm::predecessors(L->getHeader())) {
          if (L->contains(Entering)) continue;
          if (!FromOutside) {
            FromOutside = Phi->getIncomingValueForBlock(Entering);
            assert(FromOutside);
          } else if (Phi->getIncomingValueForBlock(Entering) != FromOutside) {
            FromOutside = nullptr;
            break;
          }
        }

        llvm::Value *V = Phi->getIncomingValueForBlock(From);
        assert(V);
        if (V == FromOutside || V == Phi) continue;

        // llvm::dbgs() << "  "; Phi->printAsOperand(llvm::dbgs());
        // llvm::dbgs() << ": "; V->printAsOperand(llvm::dbgs()); llvm::dbgs() << "\n";
        llvm::Instruction *VI = maybeFindValueLocation(V);
        if (!VI) return false;
        if (!L->contains(VI) || !isPermissibleLeak(L, Phi, VI)) return false;
      }
      return true;
    }
    if (!L->contains(To)) return false;
    if (rpo.is_backedge(From, To)) return false;
    PurityCondition in = conds[To].map([](BinaryPredicate term) {
      term.normalise();
      return collapseTautologies(term);
    });
    return in;
  }

  BinaryPredicate getBranchCondition(llvm::Value *cond) {
    assert(cond && llvm::isa<llvm::IntegerType>(cond->getType())
           && llvm::cast<llvm::IntegerType>(cond->getType())->getBitWidth() == 1);
    if (auto *cmp = llvm::dyn_cast<llvm::CmpInst>(cond)) {
        return BinaryPredicate(cmp->getPredicate(), cmp->getOperand(0),
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
    return collapseTautologies
      (BinaryPredicate(llvm::CmpInst::ICMP_EQ, cond,
                       llvm::ConstantInt::getTrue(cond->getContext())));
  }

  PurityCondition computeOut(const llvm::Loop *L, PurityConditions &conds,
                             const RPO &rpo, const llvm::BasicBlock *BB) {
    if (auto *branch = llvm::dyn_cast<llvm::BranchInst>(BB->getTerminator())) {
      if (branch->isConditional()) {
        llvm::BasicBlock *trueSucc = branch->getSuccessor(0);
        llvm::BasicBlock *falseSucc = branch->getSuccessor(1);
        PurityCondition trueIn = getIn(L, conds, rpo, BB, trueSucc);
        PurityCondition falseIn = getIn(L, conds, rpo, BB, falseSucc);
        if (trueIn == falseIn) return trueIn;
        // If the operands are in the loop, we can check them for
        // purity; if not, the condition should just be false. That way,
        // we won't waste good conditions in the overapproximations
        BinaryPredicate brCond = getBranchCondition(branch->getCondition());
        if (!L->contains(trueSucc)) {
          assert(trueIn.is_false());
          if (falseIn.is_true()) return brCond.negate();
          return falseIn & InsertionPoint(&*falseSucc->getFirstInsertionPt());
        }
        if (!L->contains(falseSucc)) {
          assert(falseIn.is_false());
          if (trueIn.is_true()) return brCond;
          return trueIn & InsertionPoint(&*trueSucc->getFirstInsertionPt());
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
    PurityCondition cond(true);
    for (const llvm::BasicBlock *s : llvm::successors(BB)) {
      cond &= getIn(L, conds, rpo, BB, s);
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
        return InsertionPoint(I.getNextNode());
      }
    }
    if (!I.mayWriteToMemory()) {
      if (llvm::isSafeToSpeculativelyExecute(&I)) {
        llvm::dbgs() << "Wow, a safe-to-speculate loading inst: " << I << "\n";
        return true;
      } else {
        return InsertionPoint(I.getNextNode());
      }
    }
    if (llvm::AtomicRMWInst *RMW = llvm::dyn_cast<llvm::AtomicRMWInst>(&I)) {
      BinaryPredicate pred(false);
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
      PurityCondition ret(pred);
      if (!isSafeToLoadFromPointer(RMW->getPointerOperand())) {
        ret &= InsertionPoint(I.getNextNode());
      }
      ret = ret.map(collapseTautologies);
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
        BinaryPredicate pred
          (llvm::CmpInst::ICMP_EQ, success,
           llvm::ConstantInt::getFalse(CX->getContext()));
        if (isSafeToLoadFromPointer(CX->getPointerOperand())) {
          return pred;
        } else {
          return PurityCondition(pred) & InsertionPoint(I.getNextNode());
        }
      }
    }

    if (llvm::CallInst *CI = llvm::dyn_cast<llvm::CallInst>(&I)) {
      if (CI->getCalledFunction() &&
          CI->getCalledFunction()->getName() == "__VERIFIER_assume") return true;
      llvm::Value *CalledValue;
      /* llvm::CallBase::getCalledValue() was renamed to
       * getCalledOperand() in llvm 8, but the old function remained at
       * least until llvm 10
       */
#if LLVM_VERSION_MAJOR > 8
      CalledValue = CI->getCalledOperand();
#else
      CalledValue = CI->getCalledValue();
#endif

      if (llvm::InlineAsm *IA = llvm::dyn_cast<llvm::InlineAsm>(CalledValue)) {
        /* Inline assembly is pure iff the string is empty; i.e., if
         * it's used as a compiler barrier
         */
        return (IA->getAsmString() == "");
      }
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

  void debugPrintInstructionPCs(llvm::Loop *L, PurityConditions &conds) {
    llvm::dbgs() << "Results of analysing " << L->getHeader()->getParent()->getName()
                 << ":" << *L;
    std::map<llvm::Instruction*,PurityCondition> iconds;
    for (llvm::BasicBlock *BB : LoopRPO->blocks) {
      PurityCondition cond = computeOut(L, conds, *LoopRPO, BB);
      iconds[BB->getTerminator()] = cond;
      for (auto it = BB->rbegin(); ++it != BB->rend();) {
        iconds[&*it] = cond &= instructionPurity(L, *it);
      }
    }
    llvm::formatted_raw_ostream os(llvm::errs());
    std::string indent(2, ' ');
    for (llvm::BasicBlock *BB : LoopRPO->blocks) {
      os << indent << BB->getName() << ":";
      os.PadToColumn(30+indent.size());
      os.changeColor(llvm::raw_ostream::YELLOW, false, false)
        << " ; preds = ";
      for (auto it = pred_begin(BB); it != pred_end(BB); ++it) {
        if (it != pred_begin(BB)) os << ", ";
        os << "%" << (*it)->getName();
      }
      os.resetColor() << "\n";
      for (llvm::Instruction &I : *BB) {
        I.print(os << indent, true);
        os.PadToColumn(70+indent.size());
        os.changeColor(llvm::raw_ostream::BLUE, false, false)
          << " ; " << iconds[&I];
        os.resetColor() << "\n";
      }
      os << "\n";
    }
  }

  PurityConditions analyseLoop(llvm::Loop *L) {
    PurityConditions conds;
    const RPO &rpo = *LoopRPO;
    // llvm::dbgs() << "Analysing " << L->getHeader()->getParent()->getName()
    //              << ":" << *L;
#ifndef NDEBUG
    std::set<llvm::BasicBlock*> visited;
#endif
    for (auto it = rpo.blocks.rbegin(); it != rpo.blocks.rend(); ++it) {
      llvm::BasicBlock *BB = *it;
#ifndef NDEBUG
      for (llvm::BasicBlock *S : llvm::successors(BB)) {
        assert(visited.count(S) || L->getHeader() == S || !L->contains(S)
               || rpo.is_backedge(BB, S));
      }
      visited.insert(BB);
#endif
      PurityCondition cond = computeOut(L, conds, rpo, BB);
      // llvm::dbgs() << "  out of " << BB->getName() << ": " << cond << "\n";
      /* Skip the terminator */
      for (auto it = BB->rbegin(); ++it != BB->rend();) {
        cond &= instructionPurity(L, *it);
      }
      if (conds[BB] != cond) {
        // llvm::dbgs() << " " << BB->getName() << ": " << cond << "\n";
        conds[BB] = cond;
      }
    }
#ifndef NDEBUG
    /* Check that a second pass would not change anything */
    for (llvm::BasicBlock *BB : rpo.blocks) {
      PurityCondition cond = computeOut(L, conds, rpo, BB);
      for (auto it = BB->rbegin(); ++it != BB->rend();)
        cond &= instructionPurity(L, *it);
      assert(conds.at(BB) == cond);
    }
#endif
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
        if (F->empty()) return false;
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
          if (callee.second == CG.getCallsExternalNode()
              || !visit(callee.second->getFunction())) {
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
    assert(!Loop && !LoopRPO);
    Loop = L;
    RPO rpo = getLoopRPO(L);
    LoopRPO = &rpo;
    changed |= f(L);
    Loop = nullptr;
    LoopRPO = nullptr;
    for (auto it = L->begin(); it != L->end(); ++it) {
      changed |= recurseLoops(*it, f);
    }
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

  llvm::Instruction *findInsertionPoint(llvm::Loop *L,
                                        const ConjunctionLoc &cond) {
    InsertionPoint IPT(&*L->getHeader()->getFirstInsertionPt());
    if (!(IPT &= cond.insertion_point)) abort(); // Not possible
    for (const BinaryPredicate &term : cond) {
      for (llvm::Value *Value : {term.lhs, term.rhs}) {
        if (llvm::Instruction *Loc = maybeFindUserLocationOrNull
            (llvm::dyn_cast_or_null<llvm::User>(Value))) {
          if (!L->contains(Loc->getParent())) {
            /* Must dominate header */
          } else if (!(IPT &= Loc->getNextNode())) {
            return nullptr;
          }
          llvm::dbgs() << "  IPT:" << IPT << "\n";
        }
      }
    }
    assert(IPT.earliest);
    return IPT.earliest;
  }

  /* Compute path conditions to all blocks in the loop, i.e. conditions
   * that must hold if we are in that block. */
  auto getPathConds(llvm::Loop *L, const RPO &rpo) {
    std::map<const llvm::BasicBlock*, PurityCondition> ret;
    for (const llvm::BasicBlock *BB : rpo.blocks) {
      PurityCondition &BBImp = ret[BB];
      if (BB == L->getHeader()) continue;
      BBImp = true;
      for (const llvm::BasicBlock *Pred : llvm::predecessors(BB)) {
        /* Note: gets false for backedges */
        PurityCondition PredImp = ret[Pred];
        if (const llvm::BranchInst *Br = llvm::dyn_cast<llvm::BranchInst>
            (Pred->getTerminator())) {
          if (Br->isConditional()) {
            BinaryPredicate BrCond = getBranchCondition(Br->getCondition());
            if (BB != Br->getSuccessor(0)) BrCond = BrCond.negate();
            PredImp |= BrCond;
          }
        }
        BBImp &= PredImp;
      }
    }
    return ret;
  }

  llvm::Value *constantZeroValue(llvm::Type *T) {
    return llvm::Constant::getNullValue(T);
    abort();
  }

  bool phiSameAs
  (llvm::PHINode *PHI,
   llvm::ArrayRef<std::pair<llvm::BasicBlock*,llvm::Value*>> Vals) {
    assert(PHI->getNumIncomingValues() == Vals.size());
    for (const auto &pair : Vals) {
      if (PHI->getIncomingValueForBlock(pair.first) != pair.second) return false;
    }
    return true;
  }

  /* Make a value available further down in the loop DAG. If it does not
   * dominate BB, insert PHI-nodes that supply its value if it was
   * executed in the current pure loop iteration, and some arbitrary
   * value otherwise. */
  llvm::Value *bringDown(llvm::Value *V, llvm::BasicBlock *BB,
                         const RPO &rpo, const llvm::DominatorTree &DT) {
    llvm::Instruction *I = maybeFindValueLocation(V);
    if (!I) return V; // No location
    if (I->getParent() == BB) return I; // Already in BB
    if (DT.dominates(I, BB)) return I; // No "bringing" required
    std::size_t II = rpo.block_indices.at(I->getParent());
    bool needPhi = false, allSame = true;
    llvm::SmallVector<std::pair<llvm::BasicBlock*,llvm::Value*>, 4> prev;
    for (llvm::BasicBlock *Pred : llvm::predecessors(BB)) {
      llvm::Value *PV;
      if (rpo.is_backedge(Pred, BB) || rpo.block_indices.at(Pred) < II) {
        PV = constantZeroValue(V->getType());
        needPhi = true;
      } else {
        PV = bringDown(V, Pred, rpo, DT);
      }
      prev.emplace_back(Pred, PV);
    }
    allSame = std::all_of(prev.begin(), prev.end(), [&](const auto &pair) {
      return pair.second == prev[0].second;
    });
    if (allSame && !needPhi && prev.size()) return prev[0].second;
    for (auto it = BB->begin();
         it != BB->end() && llvm::isa<llvm::PHINode>(*it); ++it) {
      if (phiSameAs(llvm::cast<llvm::PHINode>(&*it), prev)) return &*it;
    }
    llvm::PHINode *PHI = llvm::PHINode::Create
      (V->getType(), prev.size(), V->getName() + ".in." + BB->getName(),
       &*BB->getFirstInsertionPt());
    for (const auto &pair : prev) PHI->addIncoming(pair.second, pair.first);
    return PHI;
  }

  bool runOnLoop(llvm::Loop *L) {
    // Debug::warn(("plp.code." + L->getHeader()->getParent()->getName()).str())
    //   << "Analysing " << L->getHeader()->getParent()->getName() << ":\n"
    //   << *L->getHeader()->getParent();
    assert(!may_inline);
    assert(!inlining_needed);
    PurityConditions conditions = analyseLoop(L);
    // debugPrintInstructionPCs(L, conditions);
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

    auto pathConds = getPathConds(L, *LoopRPO);
    for (const ConjunctionLoc &conj : headerCond) {
      llvm::Instruction *I = findInsertionPoint(L, conj);
      if (!I) {
        // llvm::dbgs() << "Not elimiating loop purity: No valid insertion location found\n";
        continue;
      }
      //llvm::dbgs() << " Insertion point: " << *I << "\n";
      llvm::Value *Cond = nullptr;
      for (const BinaryPredicate &term : conj) {
        if (PurityCondition(term) <= pathConds.at(I->getParent())) continue;
        llvm::Value *LHS = bringDown(term.lhs, I->getParent(), *LoopRPO, *DominatorTree);
        llvm::Value *RHS = bringDown(term.rhs, I->getParent(), *LoopRPO, *DominatorTree);
        llvm::Value *TermCond = llvm::ICmpInst::Create
          (getPredicateOpcode(term.op),
           llvm::CmpInst::getInversePredicate(term.op),
           LHS, RHS, "pp.term.negated", I);
        if (!Cond) Cond = TermCond;
        else
          Cond = llvm::BinaryOperator::Create
            (llvm::BinaryOperator::BinaryOps::Or, Cond, TermCond,
             "pp.conj.negated", I);
      }
      if (!Cond) Cond = llvm::ConstantInt::getTrue(L->getHeader()->getContext());
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
    // llvm::dbgs() << *L->getHeader()->getParent();

    return true;
  }
}  // namespace

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
