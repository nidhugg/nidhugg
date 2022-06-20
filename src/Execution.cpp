// For the parts of the code originating in LLVM:
//===-- Execution.cpp - Implement code to simulate the program ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LLVMLICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file contains the actual instruction interpreter.
//
//===----------------------------------------------------------------------===//

/* Copyright (C) 2014-2017 Carl Leonardsson
 * (For the modifications made in this file to the code from LLVM.)
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

#include "Interpreter.h"
#include "Debug.h"

#include <cstdint>
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/CodeGen/IntrinsicLowering.h>
#if defined(HAVE_LLVM_IR_CONSTANTS_H)
#include <llvm/IR/Constants.h>
#elif defined(HAVE_LLVM_CONSTANTS_H)
#include <llvm/Constants.h>
#endif
#if defined(HAVE_LLVM_IR_DERIVEDTYPES_H)
#include <llvm/IR/DerivedTypes.h>
#elif defined(HAVE_LLVM_DERIVEDTYPES_H)
#include <llvm/DerivedTypes.h>
#endif
#if defined(HAVE_LLVM_IR_INLINEASM_H)
#include <llvm/IR/InlineAsm.h>
#elif defined(HAVE_LLVM_INLINEASM_H)
#include <llvm/InlineAsm.h>
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
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/ErrorHandling.h>
#if defined(HAVE_LLVM_SUPPORT_GETELEMENTPTRTYPEITERATOR_H)
#include <llvm/Support/GetElementPtrTypeIterator.h>
#elif defined(HAVE_LLVM_IR_GETELEMENTPTRTYPEITERATOR_H)
#include <llvm/IR/GetElementPtrTypeIterator.h>
#endif
#include <llvm/Support/Host.h>
#include <llvm/Support/MathExtras.h>
#include <algorithm>
#include <cmath>
using namespace llvm;

//===----------------------------------------------------------------------===//
//                     Various Helper Functions
//===----------------------------------------------------------------------===//

static void SetValue(Value *V, GenericValue Val, ExecutionContext &SF) {
  SF.Values[V] = Val;
}

static GenericValue tid_to_pthread_t(const Type *pthrtty, int tid) {
  if (pthrtty->isIntegerTy()) {
    GenericValue TIDVal;
    TIDVal.IntVal = APInt(pthrtty->getIntegerBitWidth(),tid);
    return TIDVal;
  } else {
    return PTOGV((void*)(intptr_t)tid);
  }
}

static int pthread_t_to_tid(const Type *pthrtty, const GenericValue &TID) {
  if (pthrtty->isIntegerTy()) {
    return TID.IntVal.getLimitedValue(std::numeric_limits<int>::max());
  } else {
    intptr_t ptri = (intptr_t)GVTOP(TID);
    if (ptri > std::numeric_limits<int>::max())
      return std::numeric_limits<int>::max();
    else
      return (int)ptri;
  }
}

//===----------------------------------------------------------------------===//
//                    Binary Instruction Implementations
//===----------------------------------------------------------------------===//

#define IMPLEMENT_BINARY_OPERATOR(OP, TY) \
   case Type::TY##TyID: \
     Dest.TY##Val = Src1.TY##Val OP Src2.TY##Val; \
     break

static void executeFAddInst(GenericValue &Dest, GenericValue Src1,
                            GenericValue Src2, Type *Ty) {
  switch (Ty->getTypeID()) {
    IMPLEMENT_BINARY_OPERATOR(+, Float);
    IMPLEMENT_BINARY_OPERATOR(+, Double);
  default:
    dbgs() << "Unhandled type for FAdd instruction: " << *Ty << "\n";
    llvm_unreachable(0);
  }
}

static void executeFSubInst(GenericValue &Dest, GenericValue Src1,
                            GenericValue Src2, Type *Ty) {
  switch (Ty->getTypeID()) {
    IMPLEMENT_BINARY_OPERATOR(-, Float);
    IMPLEMENT_BINARY_OPERATOR(-, Double);
  default:
    dbgs() << "Unhandled type for FSub instruction: " << *Ty << "\n";
    llvm_unreachable(0);
  }
}

static void executeFMulInst(GenericValue &Dest, GenericValue Src1,
                            GenericValue Src2, Type *Ty) {
  switch (Ty->getTypeID()) {
    IMPLEMENT_BINARY_OPERATOR(*, Float);
    IMPLEMENT_BINARY_OPERATOR(*, Double);
  default:
    dbgs() << "Unhandled type for FMul instruction: " << *Ty << "\n";
    llvm_unreachable(0);
  }
}

static void executeFDivInst(GenericValue &Dest, GenericValue Src1,
                            GenericValue Src2, Type *Ty) {
  switch (Ty->getTypeID()) {
    IMPLEMENT_BINARY_OPERATOR(/, Float);
    IMPLEMENT_BINARY_OPERATOR(/, Double);
  default:
    dbgs() << "Unhandled type for FDiv instruction: " << *Ty << "\n";
    llvm_unreachable(0);
  }
}

static void executeFRemInst(GenericValue &Dest, GenericValue Src1,
                            GenericValue Src2, Type *Ty) {
  switch (Ty->getTypeID()) {
  case Type::FloatTyID:
    Dest.FloatVal = fmod(Src1.FloatVal, Src2.FloatVal);
    break;
  case Type::DoubleTyID:
    Dest.DoubleVal = fmod(Src1.DoubleVal, Src2.DoubleVal);
    break;
  default:
    dbgs() << "Unhandled type for Rem instruction: " << *Ty << "\n";
    llvm_unreachable(0);
  }
}

#define IMPLEMENT_INTEGER_ICMP(OP, TY) \
   case Type::IntegerTyID:  \
      Dest.IntVal = APInt(1,Src1.IntVal.OP(Src2.IntVal)); \
      break;

#define IMPLEMENT_VECTOR_INTEGER_ICMP(OP, TY)                        \
  LLVM_VECTOR_TYPEID_CASES {                                         \
    assert(Src1.AggregateVal.size() == Src2.AggregateVal.size());    \
    Dest.AggregateVal.resize( Src1.AggregateVal.size() );            \
    for (uint32_t _i = 0; _i < Src1.AggregateVal.size(); _i++)       \
      Dest.AggregateVal[_i].IntVal = APInt(1,                        \
      Src1.AggregateVal[_i].IntVal.OP(Src2.AggregateVal[_i].IntVal));\
  } break;

// Handle pointers specially because they must be compared with only as much
// width as the host has.  We _do not_ want to be comparing 64 bit values when
// running on a 32-bit target, otherwise the upper 32 bits might mess up
// comparisons if they contain garbage.
#define IMPLEMENT_POINTER_ICMP(OP) \
   case Type::PointerTyID: \
      Dest.IntVal = APInt(1,(void*)(intptr_t)Src1.PointerVal OP \
                            (void*)(intptr_t)Src2.PointerVal); \
      break;

GenericValue Interpreter::executeICMP_EQ(GenericValue Src1, GenericValue Src2,
                                         Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(eq,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(eq,Ty);
    IMPLEMENT_POINTER_ICMP(==);
  default:
    dbgs() << "Unhandled type for ICMP_EQ predicate: " << *Ty << "\n";
    llvm_unreachable(0);
  }
  return Dest;
}

static GenericValue executeICMP_NE(GenericValue Src1, GenericValue Src2,
                                   Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(ne,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(ne,Ty);
    IMPLEMENT_POINTER_ICMP(!=);
  default:
    dbgs() << "Unhandled type for ICMP_NE predicate: " << *Ty << "\n";
    llvm_unreachable(0);
  }
  return Dest;
}

static GenericValue executeICMP_ULT(GenericValue Src1, GenericValue Src2,
                                    Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(ult,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(ult,Ty);
    IMPLEMENT_POINTER_ICMP( < );
  default:
    dbgs() << "Unhandled type for ICMP_ULT predicate: " << *Ty << "\n";
    llvm_unreachable(0);
  }
  return Dest;
}

static GenericValue executeICMP_SLT(GenericValue Src1, GenericValue Src2,
                                    Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(slt,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(slt,Ty);
    IMPLEMENT_POINTER_ICMP( < );
  default:
    dbgs() << "Unhandled type for ICMP_SLT predicate: " << *Ty << "\n";
    llvm_unreachable(0);
  }
  return Dest;
}

static GenericValue executeICMP_UGT(GenericValue Src1, GenericValue Src2,
                                    Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(ugt,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(ugt,Ty);
    IMPLEMENT_POINTER_ICMP( > );
  default:
    dbgs() << "Unhandled type for ICMP_UGT predicate: " << *Ty << "\n";
    llvm_unreachable(0);
  }
  return Dest;
}

static GenericValue executeICMP_SGT(GenericValue Src1, GenericValue Src2,
                                    Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(sgt,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(sgt,Ty);
    IMPLEMENT_POINTER_ICMP( > );
  default:
    dbgs() << "Unhandled type for ICMP_SGT predicate: " << *Ty << "\n";
    llvm_unreachable(0);
  }
  return Dest;
}

static GenericValue executeICMP_ULE(GenericValue Src1, GenericValue Src2,
                                    Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(ule,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(ule,Ty);
    IMPLEMENT_POINTER_ICMP( <= );
  default:
    dbgs() << "Unhandled type for ICMP_ULE predicate: " << *Ty << "\n";
    llvm_unreachable(0);
  }
  return Dest;
}

static GenericValue executeICMP_SLE(GenericValue Src1, GenericValue Src2,
                                    Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(sle,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(sle,Ty);
    IMPLEMENT_POINTER_ICMP(<=);
  default:
    dbgs() << "Unhandled type for ICMP_SLE predicate: " << *Ty << "\n";
    llvm_unreachable(0);
  }
  return Dest;
}

static GenericValue executeICMP_UGE(GenericValue Src1, GenericValue Src2,
                                    Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(uge,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(uge,Ty);
    IMPLEMENT_POINTER_ICMP(>=);
  default:
    dbgs() << "Unhandled type for ICMP_UGE predicate: " << *Ty << "\n";
    llvm_unreachable(0);
  }
  return Dest;
}

static GenericValue executeICMP_SGE(GenericValue Src1, GenericValue Src2,
                                    Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(sge,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(sge,Ty);
    IMPLEMENT_POINTER_ICMP(>=);
  default:
    dbgs() << "Unhandled type for ICMP_SGE predicate: " << *Ty << "\n";
    llvm_unreachable(0);
  }
  return Dest;
}

void Interpreter::visitICmpInst(ICmpInst &I) {
  ExecutionContext &SF = ECStack()->back();
  Type *Ty    = I.getOperand(0)->getType();
  GenericValue Src1 = getOperandValue(I.getOperand(0), SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue R;   // Result

  switch (I.getPredicate()) {
  case ICmpInst::ICMP_EQ:  R = executeICMP_EQ(Src1,  Src2, Ty); break;
  case ICmpInst::ICMP_NE:  R = executeICMP_NE(Src1,  Src2, Ty); break;
  case ICmpInst::ICMP_ULT: R = executeICMP_ULT(Src1, Src2, Ty); break;
  case ICmpInst::ICMP_SLT: R = executeICMP_SLT(Src1, Src2, Ty); break;
  case ICmpInst::ICMP_UGT: R = executeICMP_UGT(Src1, Src2, Ty); break;
  case ICmpInst::ICMP_SGT: R = executeICMP_SGT(Src1, Src2, Ty); break;
  case ICmpInst::ICMP_ULE: R = executeICMP_ULE(Src1, Src2, Ty); break;
  case ICmpInst::ICMP_SLE: R = executeICMP_SLE(Src1, Src2, Ty); break;
  case ICmpInst::ICMP_UGE: R = executeICMP_UGE(Src1, Src2, Ty); break;
  case ICmpInst::ICMP_SGE: R = executeICMP_SGE(Src1, Src2, Ty); break;
  default:
    dbgs() << "Don't know how to handle this ICmp predicate!\n-->" << I;
    llvm_unreachable(0);
  }

  SetValue(&I, R, SF);
}

#define IMPLEMENT_FCMP(OP, TY) \
   case Type::TY##TyID: \
     Dest.IntVal = APInt(1,Src1.TY##Val OP Src2.TY##Val); \
     break

#define IMPLEMENT_VECTOR_FCMP_T(OP, TY)                             \
  assert(Src1.AggregateVal.size() == Src2.AggregateVal.size());     \
  Dest.AggregateVal.resize( Src1.AggregateVal.size() );             \
  for (uint32_t _i = 0; _i < Src1.AggregateVal.size(); _i++)        \
    Dest.AggregateVal[_i].IntVal = APInt(1,                         \
    Src1.AggregateVal[_i].TY##Val OP Src2.AggregateVal[_i].TY##Val);\
  break;

#define IMPLEMENT_VECTOR_FCMP(OP)                                   \
  LLVM_VECTOR_TYPEID_CASES                                          \
    if(dyn_cast<VectorType>(Ty)->getElementType()->isFloatTy()) {   \
      IMPLEMENT_VECTOR_FCMP_T(OP, Float);                           \
    } else {                                                        \
        IMPLEMENT_VECTOR_FCMP_T(OP, Double);                        \
    }

static GenericValue executeFCMP_OEQ(GenericValue Src1, GenericValue Src2,
                                   Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(==, Float);
    IMPLEMENT_FCMP(==, Double);
    IMPLEMENT_VECTOR_FCMP( == );
  default:
    dbgs() << "Unhandled type for FCmp EQ instruction: " << *Ty << "\n";
    llvm_unreachable(0);
  }
  return Dest;
}

#define IMPLEMENT_SCALAR_NANS(TY, X,Y)                                      \
  if (TY->isFloatTy()) {                                                    \
    if (X.FloatVal != X.FloatVal || Y.FloatVal != Y.FloatVal) {             \
      Dest.IntVal = APInt(1,false);                                         \
      return Dest;                                                          \
    }                                                                       \
  } else {                                                                  \
    if (X.DoubleVal != X.DoubleVal || Y.DoubleVal != Y.DoubleVal) {         \
      Dest.IntVal = APInt(1,false);                                         \
      return Dest;                                                          \
    }                                                                       \
  }

#define MASK_VECTOR_NANS_T(X,Y, TZ, FLAG)                                   \
  assert(X.AggregateVal.size() == Y.AggregateVal.size());                   \
  Dest.AggregateVal.resize( X.AggregateVal.size() );                        \
  for (uint32_t _i = 0; _i < X.AggregateVal.size(); _i++) {                 \
    if (X.AggregateVal[_i].TZ##Val != X.AggregateVal[_i].TZ##Val ||         \
        Y.AggregateVal[_i].TZ##Val != Y.AggregateVal[_i].TZ##Val) {         \
      Dest.AggregateVal[_i].IntVal = APInt(1,FLAG);                         \
    } else {                                                                \
      Dest.AggregateVal[_i].IntVal = APInt(1,!FLAG);                        \
    }                                                                       \
  }

#define MASK_VECTOR_NANS(TY, X,Y, FLAG)                                     \
  if (TY->isVectorTy()) {                                                   \
    if (dyn_cast<VectorType>(TY)->getElementType()->isFloatTy()) {          \
      MASK_VECTOR_NANS_T(X, Y, Float, FLAG)                                 \
    } else {                                                                \
      MASK_VECTOR_NANS_T(X, Y, Double, FLAG)                                \
    }                                                                       \
  }                                                                         \


static GenericValue executeFCMP_ONE(GenericValue Src1, GenericValue Src2,
                                    Type *Ty)
{
  GenericValue Dest;
  // if input is scalar value and Src1 or Src2 is NaN return false
  IMPLEMENT_SCALAR_NANS(Ty, Src1, Src2)
  // if vector input detect NaNs and fill mask
  MASK_VECTOR_NANS(Ty, Src1, Src2, false)
  GenericValue DestMask = Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(!=, Float);
    IMPLEMENT_FCMP(!=, Double);
    IMPLEMENT_VECTOR_FCMP( != );
    default:
      dbgs() << "Unhandled type for FCmp NE instruction: " << *Ty << "\n";
      llvm_unreachable(0);
  }
  // in vector case mask out NaN elements
  if (Ty->isVectorTy()) {
    for (size_t _i = 0; _i < Src1.AggregateVal.size(); _i++)
      if (DestMask.AggregateVal[_i].IntVal == false)
        Dest.AggregateVal[_i].IntVal = APInt(1,false);
  }
  return Dest;
}

static GenericValue executeFCMP_OLE(GenericValue Src1, GenericValue Src2,
                                   Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(<=, Float);
    IMPLEMENT_FCMP(<=, Double);
    IMPLEMENT_VECTOR_FCMP( <= );
  default:
    dbgs() << "Unhandled type for FCmp LE instruction: " << *Ty << "\n";
    llvm_unreachable(0);
  }
  return Dest;
}

static GenericValue executeFCMP_OGE(GenericValue Src1, GenericValue Src2,
                                   Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(>=, Float);
    IMPLEMENT_FCMP(>=, Double);
    IMPLEMENT_VECTOR_FCMP( >= );
  default:
    dbgs() << "Unhandled type for FCmp GE instruction: " << *Ty << "\n";
    llvm_unreachable(0);
  }
  return Dest;
}

static GenericValue executeFCMP_OLT(GenericValue Src1, GenericValue Src2,
                                   Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(<, Float);
    IMPLEMENT_FCMP(<, Double);
    IMPLEMENT_VECTOR_FCMP( < );
  default:
    dbgs() << "Unhandled type for FCmp LT instruction: " << *Ty << "\n";
    llvm_unreachable(0);
  }
  return Dest;
}

static GenericValue executeFCMP_OGT(GenericValue Src1, GenericValue Src2,
                                     Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(>, Float);
    IMPLEMENT_FCMP(>, Double);
    IMPLEMENT_VECTOR_FCMP( > );
  default:
    dbgs() << "Unhandled type for FCmp GT instruction: " << *Ty << "\n";
    llvm_unreachable(0);
  }
  return Dest;
}

#define IMPLEMENT_UNORDERED(TY, X,Y)                                     \
  if (TY->isFloatTy()) {                                                 \
    if (X.FloatVal != X.FloatVal || Y.FloatVal != Y.FloatVal) {          \
      Dest.IntVal = APInt(1,true);                                       \
      return Dest;                                                       \
    }                                                                    \
  } else if (X.DoubleVal != X.DoubleVal || Y.DoubleVal != Y.DoubleVal) { \
    Dest.IntVal = APInt(1,true);                                         \
    return Dest;                                                         \
  }

#define IMPLEMENT_VECTOR_UNORDERED(TY, X,Y, _FUNC)                       \
  if (TY->isVectorTy()) {                                                \
    GenericValue DestMask = Dest;                                        \
    Dest = _FUNC(Src1, Src2, Ty);                                        \
      for (size_t _i = 0; _i < Src1.AggregateVal.size(); _i++)           \
        if (DestMask.AggregateVal[_i].IntVal == true)                    \
          Dest.AggregateVal[_i].IntVal = APInt(1,true);                  \
      return Dest;                                                       \
  }

static GenericValue executeFCMP_UEQ(GenericValue Src1, GenericValue Src2,
                                   Type *Ty) {
  GenericValue Dest;
  IMPLEMENT_UNORDERED(Ty, Src1, Src2)
  MASK_VECTOR_NANS(Ty, Src1, Src2, true)
  IMPLEMENT_VECTOR_UNORDERED(Ty, Src1, Src2, executeFCMP_OEQ)
  return executeFCMP_OEQ(Src1, Src2, Ty);
}

static GenericValue executeFCMP_UNE(GenericValue Src1, GenericValue Src2,
                                   Type *Ty) {
  GenericValue Dest;
  IMPLEMENT_UNORDERED(Ty, Src1, Src2)
  MASK_VECTOR_NANS(Ty, Src1, Src2, true)
  IMPLEMENT_VECTOR_UNORDERED(Ty, Src1, Src2, executeFCMP_ONE)
  return executeFCMP_ONE(Src1, Src2, Ty);
}

static GenericValue executeFCMP_ULE(GenericValue Src1, GenericValue Src2,
                                   Type *Ty) {
  GenericValue Dest;
  IMPLEMENT_UNORDERED(Ty, Src1, Src2)
  MASK_VECTOR_NANS(Ty, Src1, Src2, true)
  IMPLEMENT_VECTOR_UNORDERED(Ty, Src1, Src2, executeFCMP_OLE)
  return executeFCMP_OLE(Src1, Src2, Ty);
}

static GenericValue executeFCMP_UGE(GenericValue Src1, GenericValue Src2,
                                   Type *Ty) {
  GenericValue Dest;
  IMPLEMENT_UNORDERED(Ty, Src1, Src2)
  MASK_VECTOR_NANS(Ty, Src1, Src2, true)
  IMPLEMENT_VECTOR_UNORDERED(Ty, Src1, Src2, executeFCMP_OGE)
  return executeFCMP_OGE(Src1, Src2, Ty);
}

static GenericValue executeFCMP_ULT(GenericValue Src1, GenericValue Src2,
                                   Type *Ty) {
  GenericValue Dest;
  IMPLEMENT_UNORDERED(Ty, Src1, Src2)
  MASK_VECTOR_NANS(Ty, Src1, Src2, true)
  IMPLEMENT_VECTOR_UNORDERED(Ty, Src1, Src2, executeFCMP_OLT)
  return executeFCMP_OLT(Src1, Src2, Ty);
}

static GenericValue executeFCMP_UGT(GenericValue Src1, GenericValue Src2,
                                     Type *Ty) {
  GenericValue Dest;
  IMPLEMENT_UNORDERED(Ty, Src1, Src2)
  MASK_VECTOR_NANS(Ty, Src1, Src2, true)
  IMPLEMENT_VECTOR_UNORDERED(Ty, Src1, Src2, executeFCMP_OGT)
  return executeFCMP_OGT(Src1, Src2, Ty);
}

static GenericValue executeFCMP_ORD(GenericValue Src1, GenericValue Src2,
                                     Type *Ty) {
  GenericValue Dest;
  if(Ty->isVectorTy()) {
    assert(Src1.AggregateVal.size() == Src2.AggregateVal.size());
    Dest.AggregateVal.resize( Src1.AggregateVal.size() );
    if(dyn_cast<VectorType>(Ty)->getElementType()->isFloatTy()) {
      for (size_t _i = 0; _i < Src1.AggregateVal.size(); _i++)
        Dest.AggregateVal[_i].IntVal = APInt(1,
        ( (Src1.AggregateVal[_i].FloatVal ==
        Src1.AggregateVal[_i].FloatVal) &&
        (Src2.AggregateVal[_i].FloatVal ==
        Src2.AggregateVal[_i].FloatVal)));
    } else {
      for (size_t _i = 0; _i < Src1.AggregateVal.size(); _i++)
        Dest.AggregateVal[_i].IntVal = APInt(1,
        ( (Src1.AggregateVal[_i].DoubleVal ==
        Src1.AggregateVal[_i].DoubleVal) &&
        (Src2.AggregateVal[_i].DoubleVal ==
        Src2.AggregateVal[_i].DoubleVal)));
    }
  } else if (Ty->isFloatTy()) {
    Dest.IntVal = APInt(1,(Src1.FloatVal == Src1.FloatVal &&
                           Src2.FloatVal == Src2.FloatVal));
  } else {
    Dest.IntVal = APInt(1,(Src1.DoubleVal == Src1.DoubleVal &&
                           Src2.DoubleVal == Src2.DoubleVal));
  }
  return Dest;
}

static GenericValue executeFCMP_UNO(GenericValue Src1, GenericValue Src2,
                                     Type *Ty) {
  GenericValue Dest;
  if(Ty->isVectorTy()) {
    assert(Src1.AggregateVal.size() == Src2.AggregateVal.size());
    Dest.AggregateVal.resize( Src1.AggregateVal.size() );
    if(dyn_cast<VectorType>(Ty)->getElementType()->isFloatTy()) {
      for (size_t _i = 0; _i < Src1.AggregateVal.size(); _i++)
        Dest.AggregateVal[_i].IntVal = APInt(1,
        ( (Src1.AggregateVal[_i].FloatVal !=
           Src1.AggregateVal[_i].FloatVal) ||
          (Src2.AggregateVal[_i].FloatVal !=
           Src2.AggregateVal[_i].FloatVal)));
      } else {
        for (size_t _i = 0; _i < Src1.AggregateVal.size(); _i++)
          Dest.AggregateVal[_i].IntVal = APInt(1,
          ( (Src1.AggregateVal[_i].DoubleVal !=
             Src1.AggregateVal[_i].DoubleVal) ||
            (Src2.AggregateVal[_i].DoubleVal !=
             Src2.AggregateVal[_i].DoubleVal)));
      }
  } else if (Ty->isFloatTy()) {
    Dest.IntVal = APInt(1,(Src1.FloatVal != Src1.FloatVal ||
                           Src2.FloatVal != Src2.FloatVal));
  } else {
    Dest.IntVal = APInt(1,(Src1.DoubleVal != Src1.DoubleVal ||
                           Src2.DoubleVal != Src2.DoubleVal));
  }
  return Dest;
}

static GenericValue executeFCMP_BOOL(GenericValue Src1, GenericValue Src2,
                                    const Type *Ty, const bool val) {
  GenericValue Dest;
    if(Ty->isVectorTy()) {
      assert(Src1.AggregateVal.size() == Src2.AggregateVal.size());
      Dest.AggregateVal.resize( Src1.AggregateVal.size() );
      for (size_t _i = 0; _i < Src1.AggregateVal.size(); _i++)
        Dest.AggregateVal[_i].IntVal = APInt(1,val);
    } else {
      Dest.IntVal = APInt(1, val);
    }

    return Dest;
}

void Interpreter::visitFCmpInst(FCmpInst &I) {
  ExecutionContext &SF = ECStack()->back();
  Type *Ty    = I.getOperand(0)->getType();
  GenericValue Src1 = getOperandValue(I.getOperand(0), SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue R;   // Result

  switch (I.getPredicate()) {
  default:
    dbgs() << "Don't know how to handle this FCmp predicate!\n-->" << I;
    llvm_unreachable(0);
  break;
  case FCmpInst::FCMP_FALSE: R = executeFCMP_BOOL(Src1, Src2, Ty, false);
  break;
  case FCmpInst::FCMP_TRUE:  R = executeFCMP_BOOL(Src1, Src2, Ty, true);
  break;
  case FCmpInst::FCMP_ORD:   R = executeFCMP_ORD(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_UNO:   R = executeFCMP_UNO(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_UEQ:   R = executeFCMP_UEQ(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_OEQ:   R = executeFCMP_OEQ(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_UNE:   R = executeFCMP_UNE(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_ONE:   R = executeFCMP_ONE(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_ULT:   R = executeFCMP_ULT(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_OLT:   R = executeFCMP_OLT(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_UGT:   R = executeFCMP_UGT(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_OGT:   R = executeFCMP_OGT(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_ULE:   R = executeFCMP_ULE(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_OLE:   R = executeFCMP_OLE(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_UGE:   R = executeFCMP_UGE(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_OGE:   R = executeFCMP_OGE(Src1, Src2, Ty); break;
  }

  SetValue(&I, R, SF);
}

GenericValue Interpreter::executeCmpInst(unsigned predicate, GenericValue Src1,
                                         GenericValue Src2, Type *Ty) {
  GenericValue Result;
  switch (predicate) {
  case ICmpInst::ICMP_EQ:    return executeICMP_EQ(Src1, Src2, Ty);
  case ICmpInst::ICMP_NE:    return executeICMP_NE(Src1, Src2, Ty);
  case ICmpInst::ICMP_UGT:   return executeICMP_UGT(Src1, Src2, Ty);
  case ICmpInst::ICMP_SGT:   return executeICMP_SGT(Src1, Src2, Ty);
  case ICmpInst::ICMP_ULT:   return executeICMP_ULT(Src1, Src2, Ty);
  case ICmpInst::ICMP_SLT:   return executeICMP_SLT(Src1, Src2, Ty);
  case ICmpInst::ICMP_UGE:   return executeICMP_UGE(Src1, Src2, Ty);
  case ICmpInst::ICMP_SGE:   return executeICMP_SGE(Src1, Src2, Ty);
  case ICmpInst::ICMP_ULE:   return executeICMP_ULE(Src1, Src2, Ty);
  case ICmpInst::ICMP_SLE:   return executeICMP_SLE(Src1, Src2, Ty);
  case FCmpInst::FCMP_ORD:   return executeFCMP_ORD(Src1, Src2, Ty);
  case FCmpInst::FCMP_UNO:   return executeFCMP_UNO(Src1, Src2, Ty);
  case FCmpInst::FCMP_OEQ:   return executeFCMP_OEQ(Src1, Src2, Ty);
  case FCmpInst::FCMP_UEQ:   return executeFCMP_UEQ(Src1, Src2, Ty);
  case FCmpInst::FCMP_ONE:   return executeFCMP_ONE(Src1, Src2, Ty);
  case FCmpInst::FCMP_UNE:   return executeFCMP_UNE(Src1, Src2, Ty);
  case FCmpInst::FCMP_OLT:   return executeFCMP_OLT(Src1, Src2, Ty);
  case FCmpInst::FCMP_ULT:   return executeFCMP_ULT(Src1, Src2, Ty);
  case FCmpInst::FCMP_OGT:   return executeFCMP_OGT(Src1, Src2, Ty);
  case FCmpInst::FCMP_UGT:   return executeFCMP_UGT(Src1, Src2, Ty);
  case FCmpInst::FCMP_OLE:   return executeFCMP_OLE(Src1, Src2, Ty);
  case FCmpInst::FCMP_ULE:   return executeFCMP_ULE(Src1, Src2, Ty);
  case FCmpInst::FCMP_OGE:   return executeFCMP_OGE(Src1, Src2, Ty);
  case FCmpInst::FCMP_UGE:   return executeFCMP_UGE(Src1, Src2, Ty);
  case FCmpInst::FCMP_FALSE: return executeFCMP_BOOL(Src1, Src2, Ty, false);
  case FCmpInst::FCMP_TRUE:  return executeFCMP_BOOL(Src1, Src2, Ty, true);
  default:
    dbgs() << "Unhandled Cmp predicate\n";
    llvm_unreachable(0);
  }
}

void Interpreter::visitBinaryOperator(BinaryOperator &I) {
  ExecutionContext &SF = ECStack()->back();
  Type *Ty    = I.getOperand(0)->getType();
  GenericValue Src1 = getOperandValue(I.getOperand(0), SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue R;   // Result

  // First process vector operation
  if (Ty->isVectorTy()) {
    assert(Src1.AggregateVal.size() == Src2.AggregateVal.size());
    R.AggregateVal.resize(Src1.AggregateVal.size());

    // Macros to execute binary operation 'OP' over integer vectors
#define INTEGER_VECTOR_OPERATION(OP)                               \
    for (unsigned i = 0; i < R.AggregateVal.size(); ++i)           \
      R.AggregateVal[i].IntVal =                                   \
      Src1.AggregateVal[i].IntVal OP Src2.AggregateVal[i].IntVal;

    // Additional macros to execute binary operations udiv/sdiv/urem/srem since
    // they have different notation.
#define INTEGER_VECTOR_FUNCTION(OP)                                \
    for (unsigned i = 0; i < R.AggregateVal.size(); ++i)           \
      R.AggregateVal[i].IntVal =                                   \
        Src1.AggregateVal[i].IntVal.OP(Src2.AggregateVal[i].IntVal);

    // Macros to execute binary operation 'OP' over floating point type TY
    // (float or double) vectors
#define FLOAT_VECTOR_FUNCTION(OP, TY)                               \
      for (unsigned i = 0; i < R.AggregateVal.size(); ++i)          \
        R.AggregateVal[i].TY =                                      \
          Src1.AggregateVal[i].TY OP Src2.AggregateVal[i].TY;

    // Macros to choose appropriate TY: float or double and run operation
    // execution
#define FLOAT_VECTOR_OP(OP) {                                         \
  if (dyn_cast<VectorType>(Ty)->getElementType()->isFloatTy()) {      \
    FLOAT_VECTOR_FUNCTION(OP, FloatVal)                               \
  } else {                                                            \
    if (dyn_cast<VectorType>(Ty)->getElementType()->isDoubleTy()) {   \
      FLOAT_VECTOR_FUNCTION(OP, DoubleVal)                            \
    } else {                                                            \
      dbgs() << "Unhandled type for OP instruction: " << *Ty << "\n"; \
      llvm_unreachable(0);                                            \
    }                                                                 \
  }                                                                   \
}

    switch(I.getOpcode()){
    default:
      dbgs() << "Don't know how to handle this binary operator!\n-->" << I;
      llvm_unreachable(0);
      break;
    case Instruction::Add:   INTEGER_VECTOR_OPERATION(+) break;
    case Instruction::Sub:   INTEGER_VECTOR_OPERATION(-) break;
    case Instruction::Mul:   INTEGER_VECTOR_OPERATION(*) break;
    case Instruction::UDiv:  INTEGER_VECTOR_FUNCTION(udiv) break;
    case Instruction::SDiv:  INTEGER_VECTOR_FUNCTION(sdiv) break;
    case Instruction::URem:  INTEGER_VECTOR_FUNCTION(urem) break;
    case Instruction::SRem:  INTEGER_VECTOR_FUNCTION(srem) break;
    case Instruction::And:   INTEGER_VECTOR_OPERATION(&) break;
    case Instruction::Or:    INTEGER_VECTOR_OPERATION(|) break;
    case Instruction::Xor:   INTEGER_VECTOR_OPERATION(^) break;
    case Instruction::FAdd:  FLOAT_VECTOR_OP(+) break;
    case Instruction::FSub:  FLOAT_VECTOR_OP(-) break;
    case Instruction::FMul:  FLOAT_VECTOR_OP(*) break;
    case Instruction::FDiv:  FLOAT_VECTOR_OP(/) break;
    case Instruction::FRem:
      if (dyn_cast<VectorType>(Ty)->getElementType()->isFloatTy()) {
        for (unsigned i = 0; i < R.AggregateVal.size(); ++i)
          R.AggregateVal[i].FloatVal =
          fmod(Src1.AggregateVal[i].FloatVal, Src2.AggregateVal[i].FloatVal);
      } else {
        if (dyn_cast<VectorType>(Ty)->getElementType()->isDoubleTy()) {
          for (unsigned i = 0; i < R.AggregateVal.size(); ++i)
            R.AggregateVal[i].DoubleVal =
            fmod(Src1.AggregateVal[i].DoubleVal, Src2.AggregateVal[i].DoubleVal);
        } else {
          dbgs() << "Unhandled type for Rem instruction: " << *Ty << "\n";
          llvm_unreachable(0);
        }
      }
      break;
    }
  } else {
    switch (I.getOpcode()) {
    default:
      dbgs() << "Don't know how to handle this binary operator!\n-->" << I;
      llvm_unreachable(0);
      break;
    case Instruction::Add:   R.IntVal = Src1.IntVal + Src2.IntVal; break;
    case Instruction::Sub:   R.IntVal = Src1.IntVal - Src2.IntVal; break;
    case Instruction::Mul:   R.IntVal = Src1.IntVal * Src2.IntVal; break;
    case Instruction::FAdd:  executeFAddInst(R, Src1, Src2, Ty); break;
    case Instruction::FSub:  executeFSubInst(R, Src1, Src2, Ty); break;
    case Instruction::FMul:  executeFMulInst(R, Src1, Src2, Ty); break;
    case Instruction::FDiv:  executeFDivInst(R, Src1, Src2, Ty); break;
    case Instruction::FRem:  executeFRemInst(R, Src1, Src2, Ty); break;
    case Instruction::UDiv:  R.IntVal = Src1.IntVal.udiv(Src2.IntVal); break;
    case Instruction::SDiv:  R.IntVal = Src1.IntVal.sdiv(Src2.IntVal); break;
    case Instruction::URem:  R.IntVal = Src1.IntVal.urem(Src2.IntVal); break;
    case Instruction::SRem:  R.IntVal = Src1.IntVal.srem(Src2.IntVal); break;
    case Instruction::And:   R.IntVal = Src1.IntVal & Src2.IntVal; break;
    case Instruction::Or:    R.IntVal = Src1.IntVal | Src2.IntVal; break;
    case Instruction::Xor:   R.IntVal = Src1.IntVal ^ Src2.IntVal; break;
    }
  }
  SetValue(&I, R, SF);
}

static GenericValue executeSelectInst(GenericValue Src1, GenericValue Src2,
                                      GenericValue Src3, const Type *Ty) {
    GenericValue Dest;
    if(Ty->isVectorTy()) {
      assert(Src1.AggregateVal.size() == Src2.AggregateVal.size());
      assert(Src2.AggregateVal.size() == Src3.AggregateVal.size());
      Dest.AggregateVal.resize( Src1.AggregateVal.size() );
      for (size_t i = 0; i < Src1.AggregateVal.size(); ++i)
        Dest.AggregateVal[i] = (Src1.AggregateVal[i].IntVal == 0) ?
          Src3.AggregateVal[i] : Src2.AggregateVal[i];
    } else {
      Dest = (Src1.IntVal == 0) ? Src3 : Src2;
    }
    return Dest;
}

void Interpreter::visitSelectInst(SelectInst &I) {
  ExecutionContext &SF = ECStack()->back();
  const Type * Ty = I.getOperand(0)->getType();
  GenericValue Src1 = getOperandValue(I.getOperand(0), SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue Src3 = getOperandValue(I.getOperand(2), SF);
  GenericValue R = executeSelectInst(Src1, Src2, Src3, Ty);
  SetValue(&I, R, SF);
}

//===----------------------------------------------------------------------===//
//                     Terminator Instruction Implementations
//===----------------------------------------------------------------------===//

void Interpreter::exitCalled(GenericValue GV) {
  // runAtExitHandlers() assumes there are no stack frames, but
  // if exit() was called, then it had a stack frame. Blow away
  // the stack before interpreting atexit handlers.
  ECStack()->clear();
  runAtExitHandlers();
  exit(GV.IntVal.zextOrTrunc(32).getZExtValue());
}

/// Pop the last stack frame off of ECStack and then copy the result
/// back into the result variable if we are not returning void. The
/// result variable may be the ExitValue, or the Value of the calling
/// CallInst if there was a previous stack frame. This method may
/// invalidate any ECStack iterators you have. This method also takes
/// care of switching to the normal destination BB, if we are returning
/// from an invoke.
///
void Interpreter::popStackAndReturnValueToCaller(Type *RetTy,
                                                 GenericValue Result) {
  // Pop the current stack frame.
  ECStack()->pop_back();

  if (ECStack()->empty()) {  // Finished main.  Put result into exit code...
    if (RetTy && !RetTy->isVoidTy()) {          // Nonvoid return type?
      ExitValue = Result;   // Capture the exit value of the program
    } else {
      memset(&ExitValue.Untyped, 0, sizeof(ExitValue.Untyped));
    }
    terminate(RetTy,Result);
  } else {
    // If we have a previous stack frame, and we have a previous call,
    // fill in the return value...
    ExecutionContext &CallingSF = ECStack()->back();
    if (Instruction *I = &CallingSF.Caller) {
      // Save result...
      if (!(&CallingSF.Caller)->getType()->isVoidTy())
        SetValue(I, Result, CallingSF);
      if (InvokeInst *II = dyn_cast<InvokeInst> (I))
        SwitchToNewBasicBlock (II->getNormalDest (), CallingSF);
      CallingSF.Caller = AnyCallInst();          // We returned from the call...
    }
  }
}

void Interpreter::returnValueToCaller(Type *RetTy,
                                      GenericValue Result) {
  assert(!ECStack()->empty());
  // fill in the return value...
  ExecutionContext &CallingSF = ECStack()->back();
  if (Instruction *I = &CallingSF.Caller) {
    // Save result...
    if (!(&CallingSF.Caller)->getType()->isVoidTy())
      SetValue(I, Result, CallingSF);
    if (InvokeInst *II = dyn_cast<InvokeInst> (I))
      SwitchToNewBasicBlock (II->getNormalDest (), CallingSF);
    CallingSF.Caller = AnyCallInst();          // We returned from the call...
  }
}

void Interpreter::visitReturnInst(ReturnInst &I) {
  ExecutionContext &SF = ECStack()->back();
  Type *RetTy = Type::getVoidTy(I.getContext());
  GenericValue Result;

  // Save away the return value... (if we are not 'ret void')
  if (I.getNumOperands()) {
    RetTy  = I.getReturnValue()->getType();
    Result = getOperandValue(I.getReturnValue(), SF);
  }

  popStackAndReturnValueToCaller(RetTy, Result);
}

void Interpreter::visitUnreachableInst(UnreachableInst &I) {
  report_fatal_error("Program executed an 'unreachable' instruction!");
}

void Interpreter::visitBranchInst(BranchInst &I) {
  ExecutionContext &SF = ECStack()->back();
  BasicBlock *Dest;

  Dest = I.getSuccessor(0);          // Uncond branches have a fixed dest...
  if (!I.isUnconditional()) {
    Value *Cond = I.getCondition();
    bool condVal = (getOperandValue(Cond, SF).IntVal != 0);
    if(!TB.cond_branch(condVal)){
      abort();
      return;
    }
    if (!condVal) // If false cond...
      Dest = I.getSuccessor(1);
  }
  SwitchToNewBasicBlock(Dest, SF);
}

void Interpreter::visitSwitchInst(SwitchInst &I) {
  ExecutionContext &SF = ECStack()->back();
  Value* Cond = I.getCondition();
  Type *ElTy = Cond->getType();
  GenericValue CondVal = getOperandValue(Cond, SF);

  // Check to see if any of the cases match...
  BasicBlock *Dest = 0;
  for (SwitchInst::CaseIt i = I.case_begin(), e = I.case_end(); i != e; ++i) {
#ifdef LLVM_SWITCHINST_CASEIT_NEEDS_DEREFERENCE
    auto &v = *i;
#else
    auto &v = i;
#endif
    GenericValue CaseVal = getOperandValue(v.getCaseValue(), SF);
    if (executeICMP_EQ(CondVal, CaseVal, ElTy).IntVal != 0) {
      Dest = cast<BasicBlock>(v.getCaseSuccessor());
      break;
    }
  }
  if (!Dest) Dest = I.getDefaultDest();   // No cases matched: use default
  SwitchToNewBasicBlock(Dest, SF);
}

void Interpreter::visitIndirectBrInst(IndirectBrInst &I) {
  ExecutionContext &SF = ECStack()->back();
  void *Dest = GVTOP(getOperandValue(I.getAddress(), SF));
  SwitchToNewBasicBlock((BasicBlock*)Dest, SF);
}


// SwitchToNewBasicBlock - This method is used to jump to a new basic block.
// This function handles the actual updating of block and instruction iterators
// as well as execution of all of the PHI nodes in the destination block.
//
// This method does this because all of the PHI nodes must be executed
// atomically, reading their inputs before any of the results are updated.  Not
// doing this can cause problems if the PHI nodes depend on other PHI nodes for
// their inputs.  If the input PHI node is updated before it is read, incorrect
// results can happen.  Thus we use a two phase approach.
//
void Interpreter::SwitchToNewBasicBlock(BasicBlock *Dest, ExecutionContext &SF){
  BasicBlock *PrevBB = SF.CurBB;      // Remember where we came from...
  SF.CurBB   = Dest;                  // Update CurBB to branch destination
  SF.CurInst = SF.CurBB->begin();     // Update new instruction ptr...

  if (!isa<PHINode>(SF.CurInst)) return;  // Nothing fancy to do

  // Loop over all of the PHI nodes in the current block, reading their inputs.
  std::vector<GenericValue> ResultValues;

  for (; PHINode *PN = dyn_cast<PHINode>(SF.CurInst); ++SF.CurInst) {
    // Search for the value corresponding to this previous bb...
    int i = PN->getBasicBlockIndex(PrevBB);
    assert(i != -1 && "PHINode doesn't contain entry for predecessor??");
    Value *IncomingValue = PN->getIncomingValue(i);

    // Save the incoming value for this PHI node...
    ResultValues.push_back(getOperandValue(IncomingValue, SF));
  }

  // Now loop over all of the PHI nodes setting their values...
  SF.CurInst = SF.CurBB->begin();
  for (unsigned i = 0; isa<PHINode>(SF.CurInst); ++SF.CurInst, ++i) {
    PHINode *PN = cast<PHINode>(SF.CurInst);
    SetValue(PN, ResultValues[i], SF);
  }
}

//===----------------------------------------------------------------------===//
//                     Memory Instruction Implementations
//===----------------------------------------------------------------------===//

void Interpreter::visitAllocaInst(AllocaInst &I) {
  ExecutionContext &SF = ECStack()->back();

  Type *Ty = I.getType()->getPointerElementType();  // Type to be allocated

  // Get the number of elements being allocated by the array...
  unsigned NumElements =
    getOperandValue(I.getOperand(0), SF).IntVal.getZExtValue();

  unsigned TypeSize = (size_t)TD.getTypeAllocSize(Ty);

  // Avoid malloc-ing zero bytes, use max()...
  unsigned MemToAlloc = std::max(1U, NumElements * TypeSize);

  // Allocate enough memory to hold the type...
  void *Memory = malloc(MemToAlloc);

  GenericValue Result = PTOGV(Memory);
  assert(Result.PointerVal != 0 && "Null pointer returned by malloc!");
  SetValue(&I, Result, SF);

  if (I.getOpcode() == Instruction::Alloca){
    AllocatedMemStack.insert(Memory);
    SymMBlock mb = SymMBlock::Stack(CurrentThread, StackAllocCount[CurrentThread]++);
    AllocatedMem.emplace(Memory, SymMBlockSize(std::move(mb), MemToAlloc));
  }
}

// getElementOffset - The workhorse for getelementptr.
//
GenericValue Interpreter::executeGEPOperation(Value *Ptr, gep_type_iterator I,
                                              gep_type_iterator E,
                                              ExecutionContext &SF) {
  assert(Ptr->getType()->isPointerTy() &&
         "Cannot getElementOffset of a nonpointer type!");

  uint64_t Total = 0;

  for (; I != E; ++I) {
#ifdef LLVM_NEW_GEP_TYPE_ITERATOR_API
    if (StructType *STy = I.getStructTypeOrNull()) {
#else
    if (StructType *STy = dyn_cast<StructType>(*I)) {
#endif
      const StructLayout *SLO = TD.getStructLayout(STy);

      const ConstantInt *CPU = cast<ConstantInt>(I.getOperand());
      unsigned Index = unsigned(CPU->getZExtValue());

      Total += SLO->getElementOffset(Index);
    } else {
      // Get the index number for the array... which must be long type...
      GenericValue IdxGV = getOperandValue(I.getOperand(), SF);

      int64_t Idx;
      unsigned BitWidth =
        cast<IntegerType>(I.getOperand()->getType())->getBitWidth();
      if (BitWidth == 32) {
        Idx = (int64_t)(int32_t)IdxGV.IntVal.getZExtValue();
      } else {
        assert(BitWidth == 64 && "Invalid index type for getelementptr");
        Idx = (int64_t)IdxGV.IntVal.getZExtValue();
      }
      Total += TD.getTypeAllocSize
#ifdef LLVM_NEW_GEP_TYPE_ITERATOR_API
        (I.getIndexedType()
#else
        (cast<SequentialType>(*I)->getElementType()
#endif
         )*Idx;
    }
  }

  GenericValue Result;
  Result.PointerVal = ((char*)getOperandValue(Ptr, SF).PointerVal) + Total;
  return Result;
}

void Interpreter::visitGetElementPtrInst(GetElementPtrInst &I) {
  ExecutionContext &SF = ECStack()->back();
  SetValue(&I, executeGEPOperation(I.getPointerOperand(),
                                   gep_type_begin(I), gep_type_end(I), SF), SF);
}

void Interpreter::DryRunLoadValueFromMemory(GenericValue &Val,
                                            GenericValue *Src,
                                            SymAddrSize Src_sas,
                                            Type *Ty){
  int sz = Src_sas.size;
  char *buf = new char[sz];

  // Copy value from memory to buf
  for(int i = 0; i < sz; ++i){
    buf[i] = ((char*)Src)[i];
  }

  // Overwrite with values from DryRunMem
  for(auto it = DryRunMem.begin(); it != DryRunMem.end(); ++it){
    if (!it->get_ref().overlaps(Src_sas)) continue;
    unsigned start = std::max(it->get_ref().addr.offset,
                              Src_sas.addr.offset);
    unsigned end = std::min(it->get_ref().addr.offset + it->get_ref().size,
                            Src_sas.addr.offset + Src_sas.size);
    for (unsigned o = start; o < end; ++o){
      buf[o - Src_sas.addr.offset]
        = ((char*)it->get_block())[o - it->get_ref().addr.offset];
    }
  }

  LoadValueFromMemory(Val,(GenericValue*)&buf[0],Ty);
  delete[] buf;
}

void Interpreter::visitLoadInst(LoadInst &I) {
  ExecutionContext &SF = ECStack()->back();
  GenericValue SRC = getOperandValue(I.getPointerOperand(), SF);
  GenericValue *Ptr = (GenericValue*)GVTOP(SRC);
  GenericValue Result;

  Option<SymAddrSize> Ptr_sas = GetSymAddrSize(Ptr,I.getType());
  if (!Ptr_sas) return;
  if (!conf.c11 || I.isVolatile() || I.getOrdering() != llvm::AtomicOrdering::NotAtomic) {
    if (!TB.load(*Ptr_sas)) {
      abort();
      return;
    }
  }

  if(DryRun && DryRunMem.size()){
    DryRunLoadValueFromMemory(Result, Ptr, *Ptr_sas, I.getType());
    SetValue(&I, Result, SF);
    return;
  }

  LoadValueFromMemory(Result, Ptr, I.getType());
  SetValue(&I, Result, SF);
}

void Interpreter::visitStoreInst(StoreInst &I) {
  ExecutionContext &SF = ECStack()->back();
  GenericValue Val = getOperandValue(I.getOperand(0), SF);
  GenericValue *Ptr = (GenericValue *)GVTOP(getOperandValue(I.getPointerOperand(), SF));
  Option<SymAddrSize> Ptr_sas = GetSymAddrSize(Ptr,I.getOperand(0)->getType());
  if (!Ptr_sas) return;

  SymData sd = GetSymData(*Ptr_sas, I.getOperand(0)->getType(), Val);
  if (!conf.c11 || I.isVolatile() || I.getOrdering() != llvm::AtomicOrdering::NotAtomic) {
    if(!TB.atomic_store(sd)) {
      abort();
      return;
    }
  }

  if(DryRun){
    DryRunMem.emplace_back(std::move(sd));
    return;
  }

  StoreValueToMemory(Val, Ptr, I.getOperand(0)->getType());
  CheckAwaitWakeup(Val, Ptr, *Ptr_sas);
}

void Interpreter::visitAtomicCmpXchgInst(AtomicCmpXchgInst &I){
  ExecutionContext &SF = ECStack()->back();
  GenericValue CmpVal = getOperandValue(I.getCompareOperand(),SF);
  GenericValue NewVal = getOperandValue(I.getNewValOperand(),SF);
  GenericValue SRC = getOperandValue(I.getPointerOperand(), SF);
  GenericValue *Ptr = (GenericValue*)GVTOP(SRC);
  Type *Ty = I.getCompareOperand()->getType();
  GenericValue Result;

  Option<SymAddrSize> Ptr_sas = GetSymAddrSize(Ptr,Ty);
  if (!Ptr_sas) return;
  SymData::block_type expected = SymData::alloc_block(Ptr_sas->size);
  StoreValueToMemory(CmpVal,static_cast<GenericValue*>((void*)expected.get()),Ty);

#if defined(LLVM_CMPXCHG_SEPARATE_SUCCESS_FAILURE_ORDERING)
  // Return a tuple (oldval,success)
  Result.AggregateVal.resize(2);
  if(DryRun && DryRunMem.size()){
    DryRunLoadValueFromMemory(Result.AggregateVal[0], Ptr, *Ptr_sas, Ty);
  }else{
    LoadValueFromMemory(Result.AggregateVal[0], Ptr, Ty);
  }
  GenericValue CmpRes = executeICMP_EQ(Result.AggregateVal[0],CmpVal,Ty);
#else
  // Return only the old value oldval
  if(DryRun && DryRunMem.size()){
    DryRunLoadValueFromMemory(Result, Ptr, *Ptr_sas, Ty);
  }else{
    LoadValueFromMemory(Result, Ptr, Ty);
  }
  GenericValue CmpRes = executeICMP_EQ(Result,CmpVal,Ty);
#endif
  SymData sd = GetSymData(*Ptr_sas,Ty,NewVal);
  if(!TB.compare_exchange(sd, expected, CmpRes.IntVal.getBoolValue())){
    abort();
    return;
  }
  if(CmpRes.IntVal.getBoolValue()){
#if defined(LLVM_CMPXCHG_SEPARATE_SUCCESS_FAILURE_ORDERING)
    Result.AggregateVal[1].IntVal = 1;
#endif
    SetValue(&I, Result, SF);
    if(DryRun){
      DryRunMem.emplace_back(std::move(sd));
      return;
    }
    StoreValueToMemory(NewVal,Ptr,Ty);
    CheckAwaitWakeup(NewVal, Ptr, *Ptr_sas);
  }else{
#if defined(LLVM_CMPXCHG_SEPARATE_SUCCESS_FAILURE_ORDERING)
    Result.AggregateVal[1].IntVal = 0;
#endif
    SetValue(&I,Result,SF);
  }
}

void Interpreter::visitAtomicRMWInst(AtomicRMWInst &I){
  ExecutionContext &SF = ECStack()->back();
  GenericValue *Ptr = (GenericValue*)GVTOP(getOperandValue(I.getPointerOperand(), SF));
  GenericValue Val = getOperandValue(I.getValOperand(), SF);
  GenericValue OldVal, NewVal;

  assert(I.getType()->isIntegerTy());
  assert(I.getOrdering() != llvm::AtomicOrdering::NotAtomic);

  Option<SymAddrSize> Ptr_sas = GetSymAddrSize(Ptr,I.getType());
  if (!Ptr_sas) return;

  /* Load old value at *Ptr */
  if(DryRun && DryRunMem.size()){
    DryRunLoadValueFromMemory(OldVal, Ptr, *Ptr_sas, I.getType());
  }else{
    LoadValueFromMemory(OldVal, Ptr, I.getType());
  }

  SetValue(&I, OldVal, SF);

  RmwAction::Kind kind;

  /* Compute NewVal */
  switch(I.getOperation()){
  case llvm::AtomicRMWInst::Xchg:
    NewVal = Val; kind = RmwAction::XCHG; break;
  case llvm::AtomicRMWInst::Add:
    NewVal.IntVal = OldVal.IntVal + Val.IntVal; kind = RmwAction::ADD; break;
  case llvm::AtomicRMWInst::Sub:
    NewVal.IntVal = OldVal.IntVal - Val.IntVal; kind = RmwAction::SUB; break;
  case llvm::AtomicRMWInst::And:
    NewVal.IntVal = OldVal.IntVal & Val.IntVal; kind = RmwAction::AND; break;
  case llvm::AtomicRMWInst::Nand:
    NewVal.IntVal = ~(OldVal.IntVal & Val.IntVal); kind = RmwAction::NAND; break;
  case llvm::AtomicRMWInst::Or:
    NewVal.IntVal = OldVal.IntVal | Val.IntVal; kind = RmwAction::OR; break;
  case llvm::AtomicRMWInst::Xor:
    NewVal.IntVal = OldVal.IntVal ^ Val.IntVal; kind = RmwAction::XOR; break;
  case llvm::AtomicRMWInst::Max:
    NewVal.IntVal = APIntOps::smax(OldVal.IntVal,Val.IntVal);
    kind = RmwAction::MAX; break;
  case llvm::AtomicRMWInst::Min:
    NewVal.IntVal = APIntOps::smin(OldVal.IntVal,Val.IntVal);
    kind = RmwAction::MIN; break;
  case llvm::AtomicRMWInst::UMax:
    NewVal.IntVal = APIntOps::umax(OldVal.IntVal,Val.IntVal);
    kind = RmwAction::UMAX; break;
  case llvm::AtomicRMWInst::UMin:
    NewVal.IntVal = APIntOps::umin(OldVal.IntVal,Val.IntVal);
    kind = RmwAction::UMIN; break;
  default:
    throw std::logic_error("Unsupported operation in RMW instruction.");
  }

  SymData sd = GetSymData(*Ptr_sas,I.getType(),NewVal);
  SymData operand = GetSymData(*Ptr_sas,I.getType(),Val);
  if(!TB.atomic_rmw(sd, RmwAction{kind, std::move(operand.get_shared_block()),
                                  !I.use_empty()})){
    abort();
    return;
  }

  /* Store NewVal */
  if(DryRun){
    DryRunMem.emplace_back(std::move(sd));
    return;
  }
  StoreValueToMemory(NewVal,Ptr,I.getType());
  CheckAwaitWakeup(NewVal, Ptr, *Ptr_sas);
}

void Interpreter::visitInlineAsm(llvm::CallInst &CI, const std::string &asmstr){
  if(asmstr == "mfence"){ // Do nothing
  }else if(asmstr == ""){ // Do nothing
  }else{
    throw std::logic_error("Unsupported inline assembly: " + asmstr);
  }
}

//===----------------------------------------------------------------------===//
//                 Miscellaneous Instruction Implementations
//===----------------------------------------------------------------------===//

void Interpreter::visitAnyCallInst(AnyCallInst CI) {
  {
    std::string asmstr;
    if(isInlineAsm(CI,&asmstr)){
      visitInlineAsm(cast<CallInst>(CI),asmstr);
      return;
    }
  }

  ExecutionContext &SF = ECStack()->back();

  // Check to see if this is an intrinsic function call...
  Function *F = CI.getCalledFunction();
  if (F && F->isDeclaration()){
    switch (F->getIntrinsicID()) {
    case Intrinsic::not_intrinsic:
      break;
    case Intrinsic::vastart: { // va_start
      GenericValue ArgIndex;
      ArgIndex.UIntPairVal.first = ECStack()->size() - 1;
      ArgIndex.UIntPairVal.second = 0;
      SetValue(&CI, ArgIndex, SF);
      return;
    }
    case Intrinsic::vaend:    // va_end is a noop for the interpreter
      return;
    case Intrinsic::vacopy:   // va_copy: dest = src
      SetValue(&CI, getOperandValue(*CI.arg_begin(), SF), SF);
      return;
    default:
      {
        if(F->getName().str() == "llvm.dbg.value"){
          /* Ignore this intrinsic function */
          return;
        }

        /* Other processes with program counter inside the same basic
         * block as this one may be invalidated when the intrinsic
         * function is lowered. This goes not only for the program
         * counter in the topmost stack frame, but for all program
         * counters on the stack. For each such program counter, store
         * it as an integer during rewriting and restore it
         * afterwards.
         */
        std::map<ExecutionContext*,int> pcs;
        for(unsigned i = 0; i < Threads.size(); ++i){ // Other thread
          int smax = Threads[i].ECStack.size();
          if(i == (unsigned)CurrentThread){
            // Don't change the top-most stack-frame of the current thread.
            --smax;
          }
          for(int j = 0; j < smax; ++j){ // Stack frame
            ExecutionContext *EC = &Threads[i].ECStack[j];
            if(EC->CurBB == SF.CurBB){ // Pointing into this basic block
              int c = 0; // PC as offset from beginning of basic block
              while(EC->CurInst != EC->CurBB->begin()){
                --EC->CurInst;
                ++c;
              }
              pcs[EC] = c;
            }
          }
        }

        // If it is an unknown intrinsic function, use the intrinsic lowering
        // class to transform it into hopefully tasty LLVM code.
        //
        BasicBlock::iterator me(&CI);
        BasicBlock *Parent = (&CI)->getParent();
        bool atBegin(Parent->begin() == me);
        if (!atBegin)
          --me;
        IL->LowerIntrinsicCall(cast<CallInst>(&CI));

        // Restore the CurInst pointer to the first instruction newly inserted, if
        // any.
        if (atBegin) {
          SF.CurInst = Parent->begin();
        } else {
          SF.CurInst = me;
          ++SF.CurInst;
        }

        /* Restore the program counters for other stack frames in the
         * same basic block.
         */
        for(auto it : pcs){
          ExecutionContext *EC = it.first;
          int c = it.second;
          EC->CurInst = EC->CurBB->begin();
          while(c--) ++EC->CurInst;
        }
        return;
      }
    }
  } else if (!F) {
    // To handle indirect calls, we must get the pointer value from the argument
    // and treat it as a function pointer.
    GenericValue SRC = getOperandValue(CI.getCalledOperand(), SF);
    F = (Function*)GVTOP(SRC);
    if (!ValidateFunctionPointer(F)) return;
  }


  SF.Caller = CI;
  std::vector<GenericValue> ArgVals;
  const unsigned NumArgs = SF.Caller.arg_size();
  ArgVals.reserve(NumArgs);
  uint16_t pNum = 1;
  for (auto i = SF.Caller.arg_begin(),
         e = SF.Caller.arg_end(); i != e; ++i, ++pNum) {
    Value *V = *i;
    ArgVals.push_back(getOperandValue(V, SF));
  }

  callFunction(F, ArgVals);
}

// auxilary function for shift operations
static unsigned getShiftAmount(uint64_t orgShiftAmount,
                               llvm::APInt valueToShift) {
  unsigned valueWidth = valueToShift.getBitWidth();
  if (orgShiftAmount < (uint64_t)valueWidth)
    return orgShiftAmount;
  // according to the llvm documentation, if orgShiftAmount > valueWidth,
  // the result is undfeined. but we do shift by this rule:
  return (NextPowerOf2(valueWidth-1) - 1) & orgShiftAmount;
}


void Interpreter::visitShl(BinaryOperator &I) {
  ExecutionContext &SF = ECStack()->back();
  GenericValue Src1 = getOperandValue(I.getOperand(0), SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue Dest;
  const Type *Ty = I.getType();

  if (Ty->isVectorTy()) {
    uint32_t src1Size = uint32_t(Src1.AggregateVal.size());
    assert(src1Size == Src2.AggregateVal.size());
    for (unsigned i = 0; i < src1Size; i++) {
      GenericValue Result;
      uint64_t shiftAmount = Src2.AggregateVal[i].IntVal.getZExtValue();
      llvm::APInt valueToShift = Src1.AggregateVal[i].IntVal;
      Result.IntVal = valueToShift.shl(getShiftAmount(shiftAmount, valueToShift));
      Dest.AggregateVal.push_back(Result);
    }
  } else {
    // scalar
    uint64_t shiftAmount = Src2.IntVal.getZExtValue();
    llvm::APInt valueToShift = Src1.IntVal;
    Dest.IntVal = valueToShift.shl(getShiftAmount(shiftAmount, valueToShift));
  }

  SetValue(&I, Dest, SF);
}

void Interpreter::visitLShr(BinaryOperator &I) {
  ExecutionContext &SF = ECStack()->back();
  GenericValue Src1 = getOperandValue(I.getOperand(0), SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue Dest;
  const Type *Ty = I.getType();

  if (Ty->isVectorTy()) {
    uint32_t src1Size = uint32_t(Src1.AggregateVal.size());
    assert(src1Size == Src2.AggregateVal.size());
    for (unsigned i = 0; i < src1Size; i++) {
      GenericValue Result;
      uint64_t shiftAmount = Src2.AggregateVal[i].IntVal.getZExtValue();
      llvm::APInt valueToShift = Src1.AggregateVal[i].IntVal;
      Result.IntVal = valueToShift.lshr(getShiftAmount(shiftAmount, valueToShift));
      Dest.AggregateVal.push_back(Result);
    }
  } else {
    // scalar
    uint64_t shiftAmount = Src2.IntVal.getZExtValue();
    llvm::APInt valueToShift = Src1.IntVal;
    Dest.IntVal = valueToShift.lshr(getShiftAmount(shiftAmount, valueToShift));
  }

  SetValue(&I, Dest, SF);
}

void Interpreter::visitAShr(BinaryOperator &I) {
  ExecutionContext &SF = ECStack()->back();
  GenericValue Src1 = getOperandValue(I.getOperand(0), SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue Dest;
  const Type *Ty = I.getType();

  if (Ty->isVectorTy()) {
    size_t src1Size = Src1.AggregateVal.size();
    assert(src1Size == Src2.AggregateVal.size());
    for (unsigned i = 0; i < src1Size; i++) {
      GenericValue Result;
      uint64_t shiftAmount = Src2.AggregateVal[i].IntVal.getZExtValue();
      llvm::APInt valueToShift = Src1.AggregateVal[i].IntVal;
      Result.IntVal = valueToShift.ashr(getShiftAmount(shiftAmount, valueToShift));
      Dest.AggregateVal.push_back(Result);
    }
  } else {
    // scalar
    uint64_t shiftAmount = Src2.IntVal.getZExtValue();
    llvm::APInt valueToShift = Src1.IntVal;
    Dest.IntVal = valueToShift.ashr(getShiftAmount(shiftAmount, valueToShift));
  }

  SetValue(&I, Dest, SF);
}

GenericValue Interpreter::executeTruncInst(Value *SrcVal, Type *DstTy,
                                           ExecutionContext &SF) {
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);
  Type *SrcTy = SrcVal->getType();
  if (SrcTy->isVectorTy()) {
    Type *DstVecTy = DstTy->getScalarType();
    unsigned DBitWidth = cast<IntegerType>(DstVecTy)->getBitWidth();
    unsigned NumElts = Src.AggregateVal.size();
    // the sizes of src and dst vectors must be equal
    Dest.AggregateVal.resize(NumElts);
    for (unsigned i = 0; i < NumElts; i++)
      Dest.AggregateVal[i].IntVal = Src.AggregateVal[i].IntVal.trunc(DBitWidth);
  } else {
    IntegerType *DITy = cast<IntegerType>(DstTy);
    unsigned DBitWidth = DITy->getBitWidth();
    Dest.IntVal = Src.IntVal.trunc(DBitWidth);
  }
  return Dest;
}

GenericValue Interpreter::executeSExtInst(Value *SrcVal, Type *DstTy,
                                          ExecutionContext &SF) {
  const Type *SrcTy = SrcVal->getType();
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);
  if (SrcTy->isVectorTy()) {
    const Type *DstVecTy = DstTy->getScalarType();
    unsigned DBitWidth = cast<IntegerType>(DstVecTy)->getBitWidth();
    unsigned size = Src.AggregateVal.size();
    // the sizes of src and dst vectors must be equal.
    Dest.AggregateVal.resize(size);
    for (unsigned i = 0; i < size; i++)
      Dest.AggregateVal[i].IntVal = Src.AggregateVal[i].IntVal.sext(DBitWidth);
  } else {
    const IntegerType *DITy = cast<IntegerType>(DstTy);
    unsigned DBitWidth = DITy->getBitWidth();
    Dest.IntVal = Src.IntVal.sext(DBitWidth);
  }
  return Dest;
}

GenericValue Interpreter::executeZExtInst(Value *SrcVal, Type *DstTy,
                                          ExecutionContext &SF) {
  const Type *SrcTy = SrcVal->getType();
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);
  if (SrcTy->isVectorTy()) {
    const Type *DstVecTy = DstTy->getScalarType();
    unsigned DBitWidth = cast<IntegerType>(DstVecTy)->getBitWidth();

    unsigned size = Src.AggregateVal.size();
    // the sizes of src and dst vectors must be equal.
    Dest.AggregateVal.resize(size);
    for (unsigned i = 0; i < size; i++)
      Dest.AggregateVal[i].IntVal = Src.AggregateVal[i].IntVal.zext(DBitWidth);
  } else {
    const IntegerType *DITy = cast<IntegerType>(DstTy);
    unsigned DBitWidth = DITy->getBitWidth();
    Dest.IntVal = Src.IntVal.zext(DBitWidth);
  }
  return Dest;
}

GenericValue Interpreter::executeFPTruncInst(Value *SrcVal, Type *DstTy,
                                             ExecutionContext &SF) {
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);

  if (isa<VectorType>(SrcVal->getType())) {
    assert(SrcVal->getType()->getScalarType()->isDoubleTy() &&
           DstTy->getScalarType()->isFloatTy() &&
           "Invalid FPTrunc instruction");

    unsigned size = Src.AggregateVal.size();
    // the sizes of src and dst vectors must be equal.
    Dest.AggregateVal.resize(size);
    for (unsigned i = 0; i < size; i++)
      Dest.AggregateVal[i].FloatVal = (float)Src.AggregateVal[i].DoubleVal;
  } else {
    assert(SrcVal->getType()->isDoubleTy() && DstTy->isFloatTy() &&
           "Invalid FPTrunc instruction");
    Dest.FloatVal = (float)Src.DoubleVal;
  }

  return Dest;
}

GenericValue Interpreter::executeFPExtInst(Value *SrcVal, Type *DstTy,
                                           ExecutionContext &SF) {
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);

  if (isa<VectorType>(SrcVal->getType())) {
    assert(SrcVal->getType()->getScalarType()->isFloatTy() &&
           DstTy->getScalarType()->isDoubleTy() && "Invalid FPExt instruction");

    unsigned size = Src.AggregateVal.size();
    // the sizes of src and dst vectors must be equal.
    Dest.AggregateVal.resize(size);
    for (unsigned i = 0; i < size; i++)
      Dest.AggregateVal[i].DoubleVal = (double)Src.AggregateVal[i].FloatVal;
  } else {
    assert(SrcVal->getType()->isFloatTy() && DstTy->isDoubleTy() &&
           "Invalid FPExt instruction");
    Dest.DoubleVal = (double)Src.FloatVal;
  }

  return Dest;
}

GenericValue Interpreter::executeFPToUIInst(Value *SrcVal, Type *DstTy,
                                            ExecutionContext &SF) {
  Type *SrcTy = SrcVal->getType();
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);

  if (isa<VectorType>(SrcTy)) {
    const Type *DstVecTy = DstTy->getScalarType();
    const Type *SrcVecTy = SrcTy->getScalarType();
    uint32_t DBitWidth = cast<IntegerType>(DstVecTy)->getBitWidth();
    unsigned size = Src.AggregateVal.size();
    // the sizes of src and dst vectors must be equal.
    Dest.AggregateVal.resize(size);

    if (SrcVecTy->getTypeID() == Type::FloatTyID) {
      assert(SrcVecTy->isFloatingPointTy() && "Invalid FPToUI instruction");
      for (unsigned i = 0; i < size; i++)
        Dest.AggregateVal[i].IntVal = APIntOps::RoundFloatToAPInt(
            Src.AggregateVal[i].FloatVal, DBitWidth);
    } else {
      for (unsigned i = 0; i < size; i++)
        Dest.AggregateVal[i].IntVal = APIntOps::RoundDoubleToAPInt(
            Src.AggregateVal[i].DoubleVal, DBitWidth);
    }
  } else {
    // scalar
    uint32_t DBitWidth = cast<IntegerType>(DstTy)->getBitWidth();
    assert(SrcTy->isFloatingPointTy() && "Invalid FPToUI instruction");

    if (SrcTy->getTypeID() == Type::FloatTyID)
      Dest.IntVal = APIntOps::RoundFloatToAPInt(Src.FloatVal, DBitWidth);
    else
      Dest.IntVal = APIntOps::RoundDoubleToAPInt(Src.DoubleVal, DBitWidth);
  }

  return Dest;
}

GenericValue Interpreter::executeFPToSIInst(Value *SrcVal, Type *DstTy,
                                            ExecutionContext &SF) {
  Type *SrcTy = SrcVal->getType();
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);

  if (isa<VectorType>(SrcTy)) {
    const Type *DstVecTy = DstTy->getScalarType();
    const Type *SrcVecTy = SrcTy->getScalarType();
    uint32_t DBitWidth = cast<IntegerType>(DstVecTy)->getBitWidth();
    unsigned size = Src.AggregateVal.size();
    // the sizes of src and dst vectors must be equal
    Dest.AggregateVal.resize(size);

    if (SrcVecTy->getTypeID() == Type::FloatTyID) {
      assert(SrcVecTy->isFloatingPointTy() && "Invalid FPToSI instruction");
      for (unsigned i = 0; i < size; i++)
        Dest.AggregateVal[i].IntVal = APIntOps::RoundFloatToAPInt(
            Src.AggregateVal[i].FloatVal, DBitWidth);
    } else {
      for (unsigned i = 0; i < size; i++)
        Dest.AggregateVal[i].IntVal = APIntOps::RoundDoubleToAPInt(
            Src.AggregateVal[i].DoubleVal, DBitWidth);
    }
  } else {
    // scalar
    unsigned DBitWidth = cast<IntegerType>(DstTy)->getBitWidth();
    assert(SrcTy->isFloatingPointTy() && "Invalid FPToSI instruction");

    if (SrcTy->getTypeID() == Type::FloatTyID)
      Dest.IntVal = APIntOps::RoundFloatToAPInt(Src.FloatVal, DBitWidth);
    else
      Dest.IntVal = APIntOps::RoundDoubleToAPInt(Src.DoubleVal, DBitWidth);
  }
  return Dest;
}

GenericValue Interpreter::executeUIToFPInst(Value *SrcVal, Type *DstTy,
                                            ExecutionContext &SF) {
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);

  if (isa<VectorType>(SrcVal->getType())) {
    const Type *DstVecTy = DstTy->getScalarType();
    unsigned size = Src.AggregateVal.size();
    // the sizes of src and dst vectors must be equal
    Dest.AggregateVal.resize(size);

    if (DstVecTy->getTypeID() == Type::FloatTyID) {
      assert(DstVecTy->isFloatingPointTy() && "Invalid UIToFP instruction");
      for (unsigned i = 0; i < size; i++)
        Dest.AggregateVal[i].FloatVal =
            APIntOps::RoundAPIntToFloat(Src.AggregateVal[i].IntVal);
    } else {
      for (unsigned i = 0; i < size; i++)
        Dest.AggregateVal[i].DoubleVal =
            APIntOps::RoundAPIntToDouble(Src.AggregateVal[i].IntVal);
    }
  } else {
    // scalar
    assert(DstTy->isFloatingPointTy() && "Invalid UIToFP instruction");
    if (DstTy->getTypeID() == Type::FloatTyID)
      Dest.FloatVal = APIntOps::RoundAPIntToFloat(Src.IntVal);
    else
      Dest.DoubleVal = APIntOps::RoundAPIntToDouble(Src.IntVal);
  }
  return Dest;
}

GenericValue Interpreter::executeSIToFPInst(Value *SrcVal, Type *DstTy,
                                            ExecutionContext &SF) {
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);

  if (isa<VectorType>(SrcVal->getType())) {
    const Type *DstVecTy = DstTy->getScalarType();
    unsigned size = Src.AggregateVal.size();
    // the sizes of src and dst vectors must be equal
    Dest.AggregateVal.resize(size);

    if (DstVecTy->getTypeID() == Type::FloatTyID) {
      assert(DstVecTy->isFloatingPointTy() && "Invalid SIToFP instruction");
      for (unsigned i = 0; i < size; i++)
        Dest.AggregateVal[i].FloatVal =
            APIntOps::RoundSignedAPIntToFloat(Src.AggregateVal[i].IntVal);
    } else {
      for (unsigned i = 0; i < size; i++)
        Dest.AggregateVal[i].DoubleVal =
            APIntOps::RoundSignedAPIntToDouble(Src.AggregateVal[i].IntVal);
    }
  } else {
    // scalar
    assert(DstTy->isFloatingPointTy() && "Invalid SIToFP instruction");

    if (DstTy->getTypeID() == Type::FloatTyID)
      Dest.FloatVal = APIntOps::RoundSignedAPIntToFloat(Src.IntVal);
    else
      Dest.DoubleVal = APIntOps::RoundSignedAPIntToDouble(Src.IntVal);
  }

  return Dest;
}

GenericValue Interpreter::executePtrToIntInst(Value *SrcVal, Type *DstTy,
                                              ExecutionContext &SF) {
  uint32_t DBitWidth = cast<IntegerType>(DstTy)->getBitWidth();
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);
  assert(SrcVal->getType()->isPointerTy() && "Invalid PtrToInt instruction");

  Dest.IntVal = APInt(DBitWidth, (intptr_t) Src.PointerVal);
  return Dest;
}

GenericValue Interpreter::executeIntToPtrInst(Value *SrcVal, Type *DstTy,
                                              ExecutionContext &SF) {
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);
  assert(DstTy->isPointerTy() && "Invalid PtrToInt instruction");

  uint32_t PtrSize = TD.getPointerSizeInBits();
  if (PtrSize != Src.IntVal.getBitWidth())
    Src.IntVal = Src.IntVal.zextOrTrunc(PtrSize);

  Dest.PointerVal = PointerTy(intptr_t(Src.IntVal.getZExtValue()));
  return Dest;
}

GenericValue Interpreter::executeBitCastInst(Value *SrcVal, Type *DstTy,
                                             ExecutionContext &SF) {
  // This instruction supports bitwise conversion of vectors to integers and
  // to vectors of other types (as long as they have the same size)
  Type *SrcTy = SrcVal->getType();
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);

  if (isa<VectorType>(SrcTy) || isa<VectorType>(DstTy)) {
    // vector src bitcast to vector dst or vector src bitcast to scalar dst or
    // scalar src bitcast to vector dst
    bool isLittleEndian = TD.isLittleEndian();
    GenericValue TempDst, TempSrc, SrcVec;
    const Type *SrcElemTy;
    const Type *DstElemTy;
    unsigned SrcBitSize;
    unsigned DstBitSize;
    unsigned SrcNum;
    unsigned DstNum;

    if (isa<VectorType>(SrcTy)) {
      SrcElemTy = SrcTy->getScalarType();
      SrcBitSize = SrcTy->getScalarSizeInBits();
      SrcNum = Src.AggregateVal.size();
      SrcVec = Src;
    } else {
      // if src is scalar value, make it vector <1 x type>
      SrcElemTy = SrcTy;
      SrcBitSize = SrcTy->getPrimitiveSizeInBits();
      SrcNum = 1;
      SrcVec.AggregateVal.push_back(Src);
    }

    if (isa<VectorType>(DstTy)) {
      DstElemTy = DstTy->getScalarType();
      DstBitSize = DstTy->getScalarSizeInBits();
      DstNum = (SrcNum * SrcBitSize) / DstBitSize;
    } else {
      DstElemTy = DstTy;
      DstBitSize = DstTy->getPrimitiveSizeInBits();
      DstNum = 1;
    }

    if (SrcNum * SrcBitSize != DstNum * DstBitSize)
      llvm_unreachable("Invalid BitCast");

    // If src is floating point, cast to integer first.
    TempSrc.AggregateVal.resize(SrcNum);
    if (SrcElemTy->isFloatTy()) {
      for (unsigned i = 0; i < SrcNum; i++)
        TempSrc.AggregateVal[i].IntVal =
            APInt::floatToBits(SrcVec.AggregateVal[i].FloatVal);

    } else if (SrcElemTy->isDoubleTy()) {
      for (unsigned i = 0; i < SrcNum; i++)
        TempSrc.AggregateVal[i].IntVal =
            APInt::doubleToBits(SrcVec.AggregateVal[i].DoubleVal);
    } else if (SrcElemTy->isIntegerTy()) {
      for (unsigned i = 0; i < SrcNum; i++)
        TempSrc.AggregateVal[i].IntVal = SrcVec.AggregateVal[i].IntVal;
    } else {
      // Pointers are not allowed as the element type of vector.
      llvm_unreachable("Invalid Bitcast");
    }

    // now TempSrc is integer type vector
    if (DstNum < SrcNum) {
      // Example: bitcast <4 x i32> <i32 0, i32 1, i32 2, i32 3> to <2 x i64>
      unsigned Ratio = SrcNum / DstNum;
      unsigned SrcElt = 0;
      for (unsigned i = 0; i < DstNum; i++) {
        GenericValue Elt;
        Elt.IntVal = 0;
        Elt.IntVal = Elt.IntVal.zext(DstBitSize);
        unsigned ShiftAmt = isLittleEndian ? 0 : SrcBitSize * (Ratio - 1);
        for (unsigned j = 0; j < Ratio; j++) {
          APInt Tmp;
          Tmp = Tmp.zext(SrcBitSize);
          Tmp = TempSrc.AggregateVal[SrcElt++].IntVal;
          Tmp = Tmp.zext(DstBitSize);
          Tmp = Tmp.shl(ShiftAmt);
          ShiftAmt += isLittleEndian ? SrcBitSize : -SrcBitSize;
          Elt.IntVal |= Tmp;
        }
        TempDst.AggregateVal.push_back(Elt);
      }
    } else {
      // Example: bitcast <2 x i64> <i64 0, i64 1> to <4 x i32>
      unsigned Ratio = DstNum / SrcNum;
      for (unsigned i = 0; i < SrcNum; i++) {
        unsigned ShiftAmt = isLittleEndian ? 0 : DstBitSize * (Ratio - 1);
        for (unsigned j = 0; j < Ratio; j++) {
          GenericValue Elt;
          Elt.IntVal = Elt.IntVal.zext(SrcBitSize);
          Elt.IntVal = TempSrc.AggregateVal[i].IntVal;
          Elt.IntVal = Elt.IntVal.lshr(ShiftAmt);
          // it could be DstBitSize == SrcBitSize, so check it
          if (DstBitSize < SrcBitSize)
            Elt.IntVal = Elt.IntVal.trunc(DstBitSize);
          ShiftAmt += isLittleEndian ? DstBitSize : -DstBitSize;
          TempDst.AggregateVal.push_back(Elt);
        }
      }
    }

    // convert result from integer to specified type
    if (isa<VectorType>(DstTy)) {
      if (DstElemTy->isDoubleTy()) {
        Dest.AggregateVal.resize(DstNum);
        for (unsigned i = 0; i < DstNum; i++)
          Dest.AggregateVal[i].DoubleVal =
              TempDst.AggregateVal[i].IntVal.bitsToDouble();
      } else if (DstElemTy->isFloatTy()) {
        Dest.AggregateVal.resize(DstNum);
        for (unsigned i = 0; i < DstNum; i++)
          Dest.AggregateVal[i].FloatVal =
              TempDst.AggregateVal[i].IntVal.bitsToFloat();
      } else {
        Dest = TempDst;
      }
    } else {
      if (DstElemTy->isDoubleTy())
        Dest.DoubleVal = TempDst.AggregateVal[0].IntVal.bitsToDouble();
      else if (DstElemTy->isFloatTy())
        Dest.FloatVal = TempDst.AggregateVal[0].IntVal.bitsToFloat();
      else
        Dest.IntVal = TempDst.AggregateVal[0].IntVal;
    }
  } else { //  if (isa<VectorType>(SrcTy)) || isa<VectorType>(DstTy))
    // scalar src bitcast to scalar dst
    if (DstTy->isPointerTy()) {
      assert(SrcTy->isPointerTy() && "Invalid BitCast");
      Dest.PointerVal = Src.PointerVal;
    } else if (DstTy->isIntegerTy()) {
      if (SrcTy->isFloatTy())
        Dest.IntVal = APInt::floatToBits(Src.FloatVal);
      else if (SrcTy->isDoubleTy())
        Dest.IntVal = APInt::doubleToBits(Src.DoubleVal);
      else if (SrcTy->isIntegerTy())
        Dest.IntVal = Src.IntVal;
      else
        llvm_unreachable("Invalid BitCast");
    } else if (DstTy->isFloatTy()) {
      if (SrcTy->isIntegerTy())
        Dest.FloatVal = Src.IntVal.bitsToFloat();
      else
        Dest.FloatVal = Src.FloatVal;
    } else if (DstTy->isDoubleTy()) {
      if (SrcTy->isIntegerTy())
        Dest.DoubleVal = Src.IntVal.bitsToDouble();
      else
        Dest.DoubleVal = Src.DoubleVal;
    } else {
      llvm_unreachable("Invalid Bitcast");
    }
  }

  return Dest;
}

GenericValue Interpreter::executeCastOperation(Instruction::CastOps opcode, Value *SrcVal,
                                               Type *Ty, ExecutionContext &SF){
  throw std::logic_error("Interpreter::executeCastOperation: Not implemented.");
}

void Interpreter::visitTruncInst(TruncInst &I) {
  ExecutionContext &SF = ECStack()->back();
  SetValue(&I, executeTruncInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitSExtInst(SExtInst &I) {
  ExecutionContext &SF = ECStack()->back();
  SetValue(&I, executeSExtInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitZExtInst(ZExtInst &I) {
  ExecutionContext &SF = ECStack()->back();
  SetValue(&I, executeZExtInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitFPTruncInst(FPTruncInst &I) {
  ExecutionContext &SF = ECStack()->back();
  SetValue(&I, executeFPTruncInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitFPExtInst(FPExtInst &I) {
  ExecutionContext &SF = ECStack()->back();
  SetValue(&I, executeFPExtInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitUIToFPInst(UIToFPInst &I) {
  ExecutionContext &SF = ECStack()->back();
  SetValue(&I, executeUIToFPInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitSIToFPInst(SIToFPInst &I) {
  ExecutionContext &SF = ECStack()->back();
  SetValue(&I, executeSIToFPInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitFPToUIInst(FPToUIInst &I) {
  ExecutionContext &SF = ECStack()->back();
  SetValue(&I, executeFPToUIInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitFPToSIInst(FPToSIInst &I) {
  ExecutionContext &SF = ECStack()->back();
  SetValue(&I, executeFPToSIInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitPtrToIntInst(PtrToIntInst &I) {
  ExecutionContext &SF = ECStack()->back();
  SetValue(&I, executePtrToIntInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitIntToPtrInst(IntToPtrInst &I) {
  ExecutionContext &SF = ECStack()->back();
  SetValue(&I, executeIntToPtrInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitBitCastInst(BitCastInst &I) {
  ExecutionContext &SF = ECStack()->back();
  SetValue(&I, executeBitCastInst(I.getOperand(0), I.getType(), SF), SF);
}

#define IMPLEMENT_VAARG(TY) \
   case Type::TY##TyID: Dest.TY##Val = Src.TY##Val; break

void Interpreter::visitVAArgInst(VAArgInst &I) {
  ExecutionContext &SF = ECStack()->back();

  // Get the incoming valist parameter.  LLI treats the valist as a
  // (ec-stack-depth var-arg-index) pair.
  GenericValue VAList = getOperandValue(I.getOperand(0), SF);
  GenericValue Dest;
  GenericValue Src = (*ECStack())[VAList.UIntPairVal.first]
                      .VarArgs[VAList.UIntPairVal.second];
  Type *Ty = I.getType();
  switch (Ty->getTypeID()) {
  case Type::IntegerTyID:
    Dest.IntVal = Src.IntVal;
    break;
  IMPLEMENT_VAARG(Pointer);
  IMPLEMENT_VAARG(Float);
  IMPLEMENT_VAARG(Double);
  default:
    dbgs() << "Unhandled dest type for vaarg instruction: " << *Ty << "\n";
    llvm_unreachable(0);
  }

  // Set the Value of this Instruction.
  SetValue(&I, Dest, SF);

  // Move the pointer to the next vararg.
  ++VAList.UIntPairVal.second;
}

void Interpreter::visitExtractElementInst(ExtractElementInst &I) {
  ExecutionContext &SF = ECStack()->back();
  GenericValue Src1 = getOperandValue(I.getOperand(0), SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue Dest;

  Type *Ty = I.getType();
  const unsigned indx = unsigned(Src2.IntVal.getZExtValue());

  if(Src1.AggregateVal.size() > indx) {
    switch (Ty->getTypeID()) {
    default:
      dbgs() << "Unhandled destination type for extractelement instruction: "
      << *Ty << "\n";
      llvm_unreachable(0);
      break;
    case Type::IntegerTyID:
      Dest.IntVal = Src1.AggregateVal[indx].IntVal;
      break;
    case Type::FloatTyID:
      Dest.FloatVal = Src1.AggregateVal[indx].FloatVal;
      break;
    case Type::DoubleTyID:
      Dest.DoubleVal = Src1.AggregateVal[indx].DoubleVal;
      break;
    }
  } else {
    dbgs() << "Invalid index in extractelement instruction\n";
  }

  SetValue(&I, Dest, SF);
}

void Interpreter::visitInsertElementInst(InsertElementInst &I) {
  ExecutionContext &SF = ECStack()->back();
  Type *Ty = I.getType();

  if(!(Ty->isVectorTy()) )
    llvm_unreachable("Unhandled dest type for insertelement instruction");

  GenericValue Src1 = getOperandValue(I.getOperand(0), SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue Src3 = getOperandValue(I.getOperand(2), SF);
  GenericValue Dest;

  Type *TyContained = Ty->getContainedType(0);

  const unsigned indx = unsigned(Src3.IntVal.getZExtValue());
  Dest.AggregateVal = Src1.AggregateVal;

  if(Src1.AggregateVal.size() <= indx)
      llvm_unreachable("Invalid index in insertelement instruction");
  switch (TyContained->getTypeID()) {
    default:
      llvm_unreachable("Unhandled dest type for insertelement instruction");
    case Type::IntegerTyID:
      Dest.AggregateVal[indx].IntVal = Src2.IntVal;
      break;
    case Type::FloatTyID:
      Dest.AggregateVal[indx].FloatVal = Src2.FloatVal;
      break;
    case Type::DoubleTyID:
      Dest.AggregateVal[indx].DoubleVal = Src2.DoubleVal;
      break;
  }
  SetValue(&I, Dest, SF);
}

void Interpreter::visitShuffleVectorInst(ShuffleVectorInst &I){
  ExecutionContext &SF = ECStack()->back();

  Type *Ty = I.getType();
  if(!(Ty->isVectorTy()))
    llvm_unreachable("Unhandled dest type for shufflevector instruction");

  GenericValue Src1 = getOperandValue(I.getOperand(0), SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue Src3 = getOperandValue(I.getOperand(2), SF);
  GenericValue Dest;

  // There is no need to check types of src1 and src2, because the compiled
  // bytecode can't contain different types for src1 and src2 for a
  // shufflevector instruction.

  Type *TyContained = Ty->getContainedType(0);
  unsigned src1Size = (unsigned)Src1.AggregateVal.size();
  unsigned src2Size = (unsigned)Src2.AggregateVal.size();
  unsigned src3Size = (unsigned)Src3.AggregateVal.size();

  Dest.AggregateVal.resize(src3Size);

  switch (TyContained->getTypeID()) {
    default:
      llvm_unreachable("Unhandled dest type for insertelement instruction");
      break;
    case Type::IntegerTyID:
      for (unsigned i = 0; i < src3Size; i++) {
        unsigned j = Src3.AggregateVal[i].IntVal.getZExtValue();
        if(j < src1Size)
          Dest.AggregateVal[i].IntVal = Src1.AggregateVal[j].IntVal;
        else if(j < src1Size + src2Size)
          Dest.AggregateVal[i].IntVal = Src2.AggregateVal[j-src1Size].IntVal;
        else
          // The selector may not be greater than sum of lengths of first and
          // second operands and llasm should not allow situation like
          // %tmp = shufflevector <2 x i32> <i32 3, i32 4>, <2 x i32> undef,
          //                      <2 x i32> < i32 0, i32 5 >,
          // where i32 5 is invalid, but let it be additional check here:
          llvm_unreachable("Invalid mask in shufflevector instruction");
      }
      break;
    case Type::FloatTyID:
      for (unsigned i = 0; i < src3Size; i++) {
        unsigned j = Src3.AggregateVal[i].IntVal.getZExtValue();
        if(j < src1Size)
          Dest.AggregateVal[i].FloatVal = Src1.AggregateVal[j].FloatVal;
        else if(j < src1Size + src2Size)
          Dest.AggregateVal[i].FloatVal = Src2.AggregateVal[j-src1Size].FloatVal;
        else
          llvm_unreachable("Invalid mask in shufflevector instruction");
        }
      break;
    case Type::DoubleTyID:
      for (unsigned i = 0; i < src3Size; i++) {
        unsigned j = Src3.AggregateVal[i].IntVal.getZExtValue();
        if(j < src1Size)
          Dest.AggregateVal[i].DoubleVal = Src1.AggregateVal[j].DoubleVal;
        else if(j < src1Size + src2Size)
          Dest.AggregateVal[i].DoubleVal =
            Src2.AggregateVal[j-src1Size].DoubleVal;
        else
          llvm_unreachable("Invalid mask in shufflevector instruction");
      }
      break;
  }
  SetValue(&I, Dest, SF);
}

void Interpreter::visitExtractValueInst(ExtractValueInst &I) {
  ExecutionContext &SF = ECStack()->back();
  Value *Agg = I.getAggregateOperand();
  GenericValue Dest;
  GenericValue Src = getOperandValue(Agg, SF);

  ExtractValueInst::idx_iterator IdxBegin = I.idx_begin();
  unsigned Num = I.getNumIndices();
  GenericValue *pSrc = &Src;

  for (unsigned i = 0 ; i < Num; ++i) {
    pSrc = &pSrc->AggregateVal[*IdxBegin];
    ++IdxBegin;
  }

  Type *IndexedType = ExtractValueInst::getIndexedType(Agg->getType(), I.getIndices());
  switch (IndexedType->getTypeID()) {
    default:
      llvm_unreachable("Unhandled dest type for extractelement instruction");
    break;
    case Type::IntegerTyID:
      Dest.IntVal = pSrc->IntVal;
    break;
    case Type::FloatTyID:
      Dest.FloatVal = pSrc->FloatVal;
    break;
    case Type::DoubleTyID:
      Dest.DoubleVal = pSrc->DoubleVal;
    break;
    case Type::ArrayTyID:
    case Type::StructTyID:
    LLVM_VECTOR_TYPEID_CASES
      Dest.AggregateVal = pSrc->AggregateVal;
    break;
    case Type::PointerTyID:
      Dest.PointerVal = pSrc->PointerVal;
    break;
  }

  SetValue(&I, Dest, SF);
}

void Interpreter::visitInsertValueInst(InsertValueInst &I) {
  ExecutionContext &SF = ECStack()->back();
  Value *Agg = I.getAggregateOperand();

  GenericValue Src1 = getOperandValue(Agg, SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue Dest = Src1; // Dest is a slightly changed Src1

  ExtractValueInst::idx_iterator IdxBegin = I.idx_begin();
  unsigned Num = I.getNumIndices();

  GenericValue *pDest = &Dest;
  for (unsigned i = 0 ; i < Num; ++i) {
    pDest = &pDest->AggregateVal[*IdxBegin];
    ++IdxBegin;
  }
  // pDest points to the target value in the Dest now

  Type *IndexedType = ExtractValueInst::getIndexedType(Agg->getType(), I.getIndices());

  switch (IndexedType->getTypeID()) {
    default:
      llvm_unreachable("Unhandled dest type for insertelement instruction");
    break;
    case Type::IntegerTyID:
      pDest->IntVal = Src2.IntVal;
    break;
    case Type::FloatTyID:
      pDest->FloatVal = Src2.FloatVal;
    break;
    case Type::DoubleTyID:
      pDest->DoubleVal = Src2.DoubleVal;
    break;
    case Type::ArrayTyID:
    case Type::StructTyID:
    LLVM_VECTOR_TYPEID_CASES
      pDest->AggregateVal = Src2.AggregateVal;
    break;
    case Type::PointerTyID:
      pDest->PointerVal = Src2.PointerVal;
    break;
  }

  SetValue(&I, Dest, SF);
}

GenericValue Interpreter::getConstantExprValue (ConstantExpr *CE,
                                                ExecutionContext &SF) {
  switch (CE->getOpcode()) {
  case Instruction::Trunc:
      return executeTruncInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::ZExt:
      return executeZExtInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::SExt:
      return executeSExtInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::FPTrunc:
      return executeFPTruncInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::FPExt:
      return executeFPExtInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::UIToFP:
      return executeUIToFPInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::SIToFP:
      return executeSIToFPInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::FPToUI:
      return executeFPToUIInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::FPToSI:
      return executeFPToSIInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::PtrToInt:
      return executePtrToIntInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::IntToPtr:
      return executeIntToPtrInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::BitCast:
      return executeBitCastInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::GetElementPtr:
    return executeGEPOperation(CE->getOperand(0), gep_type_begin(CE),
                               gep_type_end(CE), SF);
  case Instruction::FCmp:
  case Instruction::ICmp:
    return executeCmpInst(CE->getPredicate(),
                          getOperandValue(CE->getOperand(0), SF),
                          getOperandValue(CE->getOperand(1), SF),
                          CE->getOperand(0)->getType());
  case Instruction::Select:
    return executeSelectInst(getOperandValue(CE->getOperand(0), SF),
                             getOperandValue(CE->getOperand(1), SF),
                             getOperandValue(CE->getOperand(2), SF),
                             CE->getOperand(0)->getType());
  default :
    break;
  }

  // The cases below here require a GenericValue parameter for the result
  // so we initialize one, compute it and then return it.
  GenericValue Op0 = getOperandValue(CE->getOperand(0), SF);
  GenericValue Op1 = getOperandValue(CE->getOperand(1), SF);
  GenericValue Dest;
  Type * Ty = CE->getOperand(0)->getType();
  switch (CE->getOpcode()) {
  case Instruction::Add:  Dest.IntVal = Op0.IntVal + Op1.IntVal; break;
  case Instruction::Sub:  Dest.IntVal = Op0.IntVal - Op1.IntVal; break;
  case Instruction::Mul:  Dest.IntVal = Op0.IntVal * Op1.IntVal; break;
  case Instruction::FAdd: executeFAddInst(Dest, Op0, Op1, Ty); break;
  case Instruction::FSub: executeFSubInst(Dest, Op0, Op1, Ty); break;
  case Instruction::FMul: executeFMulInst(Dest, Op0, Op1, Ty); break;
  case Instruction::FDiv: executeFDivInst(Dest, Op0, Op1, Ty); break;
  case Instruction::FRem: executeFRemInst(Dest, Op0, Op1, Ty); break;
  case Instruction::SDiv: Dest.IntVal = Op0.IntVal.sdiv(Op1.IntVal); break;
  case Instruction::UDiv: Dest.IntVal = Op0.IntVal.udiv(Op1.IntVal); break;
  case Instruction::URem: Dest.IntVal = Op0.IntVal.urem(Op1.IntVal); break;
  case Instruction::SRem: Dest.IntVal = Op0.IntVal.srem(Op1.IntVal); break;
  case Instruction::And:  Dest.IntVal = Op0.IntVal & Op1.IntVal; break;
  case Instruction::Or:   Dest.IntVal = Op0.IntVal | Op1.IntVal; break;
  case Instruction::Xor:  Dest.IntVal = Op0.IntVal ^ Op1.IntVal; break;
  case Instruction::Shl:
    Dest.IntVal = Op0.IntVal.shl(Op1.IntVal.getZExtValue());
    break;
  case Instruction::LShr:
    Dest.IntVal = Op0.IntVal.lshr(Op1.IntVal.getZExtValue());
    break;
  case Instruction::AShr:
    Dest.IntVal = Op0.IntVal.ashr(Op1.IntVal.getZExtValue());
    break;
  default:
    dbgs() << "Unhandled ConstantExpr: " << *CE << "\n";
    llvm_unreachable("Unhandled ConstantExpr");
  }
  return Dest;
}

GenericValue Interpreter::getOperandValue(Value *V, ExecutionContext &SF) {
  if(Constant *CPV = dyn_cast<Constant>(V)){
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(V)) {
      return getConstantExprValue(CE, SF);
    } else if (llvm::GlobalVariable *GV = dyn_cast<llvm::GlobalVariable>(V)) {
      if(GV->isThreadLocal()){
        auto it = Threads[CurrentThread].ThreadLocalValues.find(GV);
        if(it == Threads[CurrentThread].ThreadLocalValues.end()){
          llvm::Type *ty = static_cast<llvm::PointerType*>(GV->getType())
            ->getPointerElementType();
          unsigned TypeSize = (size_t)TD.getTypeAllocSize(ty);
          void *Memory = malloc(TypeSize);
          GenericValue Result = PTOGV(Memory);
          assert(Result.PointerVal != 0 && "Null pointer returned by malloc!");
          AllocatedMemHeap.insert(Memory);
          SymMBlock mb = SymMBlock::Heap(CurrentThread, HeapAllocCount[CurrentThread]++);
          AllocatedMem.emplace(Memory, SymMBlockSize(std::move(mb), TypeSize));
          Threads[CurrentThread].ThreadLocalValues[GV] = Result;
          InitializeMemory(GV->getInitializer(),Memory);
          return Result;
        }else{
          return it->second;
        }
      }
    }
    return getConstantValue(CPV);
  } else {
    return SF.Values[V];
  }
}

//===----------------------------------------------------------------------===//
//                        Dispatch and Execution Code
//===----------------------------------------------------------------------===//

void Interpreter::callPthreadCreate(Function *F,
                                    const std::vector<GenericValue> &ArgVals) {
  // Return 0 (success)
  GenericValue Result;
  Result.IntVal = APInt(F->getReturnType()->getIntegerBitWidth(),0);
  returnValueToCaller(F->getReturnType(),Result);

  Function *F_inner = (Function*)GVTOP(ArgVals[2]);
  if (!ValidateFunctionPointer(F_inner)) return;

  // Save thread ID to the location pointed to by the first argument
  {
    int new_tid = Threads.size();
    GenericValue *Ptr = (GenericValue*)GVTOP(ArgVals[0]);
    if(Ptr){
      Type *ity = static_cast<PointerType*>(F->arg_begin()->getType())
        ->getPointerElementType();
      if (!GetSymAddrSize(Ptr,ity)) return;
      GenericValue TIDVal = tid_to_pthread_t(ity, new_tid);
      /* XXX: No race detection on this access! */
      StoreValueToMemory(TIDVal,Ptr,ity);
    }else{
      /* Allow null pointers in first argument. For convenience. */
    }
  }

  // Memory fence
  if (!TB.fence()
      || !TB.spawn()) {
    abort();
    return;
  }

  // Add a new stack for the new thread
  int caller_thread = CurrentThread;
  CurrentThread = newThread(CPS.spawn(Threads[CurrentThread].cpid));

  // Build stack frame for the call
  // XXX: No validation on argument value!
  std::vector<GenericValue> ArgVals_inner;
  if(F_inner->arg_size() == 1 &&
     F_inner->arg_begin()->getType() == Type::getInt8PtrTy(F->getContext())){
    ArgVals_inner.push_back(ArgVals[3]);
  }else if(F_inner->arg_size()){
    std::string _err;
    llvm::raw_string_ostream err(_err);
    err << "Unsupported: function passed as argument to pthread_create has type: "
        << *F_inner->getType();
    throw std::logic_error(err.str().c_str());
  }
  callFunction(F_inner,ArgVals_inner);

  // Return to caller
  CurrentThread = caller_thread;
}

void Interpreter::callPthreadJoin(Function *F,
                                  const std::vector<GenericValue> &ArgVals) {
  int tid = pthread_t_to_tid(F->arg_begin()->getType(), ArgVals[0]);

  if(tid < 0 || int(Threads.size()) <= tid || tid == CurrentThread){
    std::stringstream ss;
    ss << "Invalid thread ID in pthread_join: " << tid;
    if(tid == CurrentThread) ss << " (same as ID of calling thread)";
    TB.pthreads_error(ss.str());
    abort();
    return;
  }

  assert(Threads[tid].ECStack.empty());

  if (!TB.fence()
      || !TB.join(tid)) {
    abort();
    return;
  }

  // Forward return value
  GenericValue *rvPtr = (GenericValue*)GVTOP(ArgVals[1]);
  if(rvPtr){
    Type *ty = Type::getInt8PtrTy(F->getContext())->getPointerTo();
    if (!GetSymAddrSize(rvPtr,ty)) return;
    /* XXX: No race detection on this access*/
    StoreValueToMemory(Threads[tid].RetVal,rvPtr,ty);
  }

  // Return 0 (success)
  GenericValue Result;
  Result.IntVal = APInt(F->getReturnType()->getIntegerBitWidth(),0);
  returnValueToCaller(F->getReturnType(),Result);
}

void Interpreter::callPthreadSelf(Function *F,
                                  const std::vector<GenericValue> &ArgVals){
  Type *pthread_t = F->getReturnType();
  GenericValue Result = tid_to_pthread_t(pthread_t, CurrentThread);
  returnValueToCaller(pthread_t,Result);
}

void Interpreter::callPthreadExit(Function *F,
                                  const std::vector<GenericValue> &ArgVals){
  if (!TB.fence()) {
    abort();
    return;
  }
  while(ECStack()->size() > 1) ECStack()->pop_back();
  popStackAndReturnValueToCaller(Type::getInt8PtrTy(F->getContext()),ArgVals[0]);
}

void Interpreter::callPthreadMutexInit(Function *F,
                                       const std::vector<GenericValue> &ArgVals){
  GenericValue *lck = (GenericValue*)GVTOP(ArgVals[0]);
  GenericValue *attr = (GenericValue*)GVTOP(ArgVals[1]);

  if(attr){
    Debug::warn("pthreadmutexinitattr")
      << "WARNING: Unsupported: Non-null attributes given in pthread_mutex_init. (Ignoring argument.)\n";
  }

  if(!lck){
    TB.pthreads_error("pthread_mutex_init called with null pointer as first argument.");
    abort();
    return;
  }

  Option<SymAddr> addr = GetSymAddr(lck);
  if (!addr) return;
  if (!TB.mutex_init({*addr,1})) { // also acts as a fence
    abort();
    return;
  }

  if(PthreadMutexes.count(lck)){
    TB.pthreads_error("pthread_mutex_init called with already initialized mutex.");
    abort();
    return;
  }

  GenericValue Result;
  /* pthread_mutex_init always returns 0 */
  Result.IntVal = APInt(F->getReturnType()->getIntegerBitWidth(),0);
  returnValueToCaller(F->getReturnType(),Result);

  if(DryRun) return;
  PthreadMutexes[lck] = PthreadMutex();
}

void Interpreter::callPthreadMutexLock(Function *F,
                                       const std::vector<GenericValue> &ArgVals){
  GenericValue *lck = (GenericValue*)GVTOP(ArgVals[0]);
  callPthreadMutexLock(lck);

  if(ECStack()->size()){ // Abort did not occur in callPthreadMutexLock above
    GenericValue Result;
    Result.IntVal = APInt(F->getReturnType()->getIntegerBitWidth(),0); // Success
    returnValueToCaller(F->getReturnType(),Result);
  }
}

void Interpreter::callPthreadMutexLock(void *lck){
  if(!lck){
    TB.pthreads_error("pthread_mutex_lock called with null pointer as first argument.");
    abort();
    return;
  }

  Option<SymAddr> addr = GetSymAddr(lck);
  if (!addr) return;
  if(!TB.mutex_lock({*addr,1})){ // also acts as a fence
    abort();
    return;
  }

  if(PthreadMutexes.count(lck) == 0){
    if(conf.mutex_require_init){
      TB.pthreads_error("pthread_mutex_lock called with uninitialized mutex.");
      abort();
      return;
    }else if(!DryRun){
      PthreadMutexes[lck] = PthreadMutex();
    }
  }

  assert(PthreadMutexes.count(lck) == 0 || PthreadMutexes[lck].isUnlocked());

  if(DryRun) return;
  PthreadMutexes[lck].lock(CurrentThread);
}

void Interpreter::callPthreadMutexTryLock(Function *F,
                                       const std::vector<GenericValue> &ArgVals){
  GenericValue *lck = (GenericValue*)GVTOP(ArgVals[0]);

  if(!lck){
    TB.pthreads_error("pthread_mutex_trylock called with null pointer as first argument.");
    abort();
    return;
  }

  Option<SymAddr> addr = GetSymAddr(lck);
  if (!addr) return;
  if(!TB.mutex_trylock({*addr,1})){ // also acts as a fence
    abort();
    return;
  }

  if(PthreadMutexes.count(lck) == 0){
    if(conf.mutex_require_init){
      TB.pthreads_error("pthread_mutex_trylock called with uninitialized mutex.");
      abort();
      return;
    }else if(!DryRun){
      PthreadMutexes[lck] = PthreadMutex();
    }
  }

  GenericValue Result;

  if(PthreadMutexes.count(lck) == 0 || PthreadMutexes[lck].isUnlocked()){
    Result.IntVal = APInt(F->getReturnType()->getIntegerBitWidth(),0); // Success
    returnValueToCaller(F->getReturnType(),Result);

    if(DryRun) return;
    PthreadMutexes[lck].lock(CurrentThread);
  }else{
    Result.IntVal = APInt(F->getReturnType()->getIntegerBitWidth(),EBUSY); // Failure
    returnValueToCaller(F->getReturnType(),Result);
  }
}

void Interpreter::callPthreadMutexUnlock(Function *F,
                                         const std::vector<GenericValue> &ArgVals){
  GenericValue *lck = (GenericValue*)GVTOP(ArgVals[0]);

  if(!lck){
    TB.pthreads_error("pthread_mutex_unlock called with null pointer as first argument.");
    abort();
    return;
  }

  Option<SymAddr> addr = GetSymAddr(lck);
  if (!addr) return;
  if(!TB.mutex_unlock({*addr,1})){ // also acts as a fence
    abort();
    return;
  }

  if(PthreadMutexes.count(lck) == 0){
    if(conf.mutex_require_init){
      TB.pthreads_error("pthread_mutex_unlock called with uninitialized mutex.");
      abort();
      return;
    }else if(!DryRun){
      PthreadMutexes[lck] = PthreadMutex();
    }
  }

  if(PthreadMutexes.count(lck) && PthreadMutexes[lck].owner != CurrentThread){
    TB.pthreads_error("pthread_mutex_unlock called with mutex not locked by the same process.");
    abort();
    return;
  }

  GenericValue Result;
  Result.IntVal = APInt(F->getReturnType()->getIntegerBitWidth(),0); // Success
  returnValueToCaller(F->getReturnType(),Result);

  if(DryRun) return;

  PthreadMutexes[lck].unlock();
  for(int p : PthreadMutexes[lck].waiting){
    TB.mark_available(p);
  }
  PthreadMutexes[lck].waiting.clear();
}

void Interpreter::callPthreadMutexDestroy(Function *F,
                                          const std::vector<GenericValue> &ArgVals){
  GenericValue *lck = (GenericValue*)GVTOP(ArgVals[0]);

  if(!lck){
    TB.pthreads_error("pthread_mutex_destroy called with null pointer as first argument.");
    abort();
    return;
  }

  Option<SymAddr> addr = GetSymAddr(lck);
  if (!addr) return;
  if(!TB.mutex_destroy({*addr,1})){ // also acts as a fence
    abort();
    return;
  }

  if(PthreadMutexes.count(lck) == 0){
    if(conf.mutex_require_init){
      TB.pthreads_error("pthread_mutex_destroy called with uninitialized mutex.");
      abort();
      return;
    }else if(!DryRun){
      PthreadMutexes[lck] = PthreadMutex();
    }
  }

  if(PthreadMutexes.count(lck) && PthreadMutexes[lck].isLocked()){
    TB.pthreads_error("pthread_mutex_destroy called with locked mutex.");
    abort();
    return;
  }

  GenericValue Result;
  Result.IntVal = APInt(F->getReturnType()->getIntegerBitWidth(),0); // Success
  returnValueToCaller(F->getReturnType(),Result);

  if(DryRun) return;
  PthreadMutexes.erase(lck);
}

void Interpreter::callPthreadCondInit(Function *F,
                                      const std::vector<GenericValue> &ArgVals){
  GenericValue *cnd = (GenericValue*)GVTOP(ArgVals[0]);
  GenericValue *attr = (GenericValue*)GVTOP(ArgVals[1]);

  if(attr){
    Debug::warn("pthreadcondinitattr")
      << "WARNING: Unsupported: Non-null attributes given in pthread_cond_init. (Ignoring argument.)\n";
  }

  if(!cnd){
    TB.pthreads_error("pthread_cond_init called with null pointer as first argument.");
    abort();
    return;
  }

  Option<SymAddr> addr = GetSymAddr(cnd);
  if (!addr) return;
  if(!TB.cond_init({*addr,1})){ // also acts as a fence
    abort();
    return;
  }

  GenericValue Result;
  /* pthread_cond_init always returns 0 */
  Result.IntVal = APInt(F->getReturnType()->getIntegerBitWidth(),0);
  returnValueToCaller(F->getReturnType(),Result);
}

void Interpreter::callPthreadCondSignal(Function *F,
                                        const std::vector<GenericValue> &ArgVals){
  GenericValue *cnd = (GenericValue*)GVTOP(ArgVals[0]);

  if(!cnd){
    TB.pthreads_error("pthread_cond_signal called with null pointer as first argument.");
    abort();
    return;
  }

  Option<SymAddr> addr = GetSymAddr(cnd);
  if (!addr) return;
  if(!TB.cond_signal({*addr,1})){ // also acts as a fence
    abort();
    return;
  }

  GenericValue Result;
  /* pthread_cond_wait always returns 0 */
  Result.IntVal = APInt(F->getReturnType()->getIntegerBitWidth(),0);
  returnValueToCaller(F->getReturnType(),Result);
}

void Interpreter::callPthreadCondBroadcast(Function *F,
                                           const std::vector<GenericValue> &ArgVals){
  GenericValue *cnd = (GenericValue*)GVTOP(ArgVals[0]);

  if(!cnd){
    TB.pthreads_error("pthread_cond_broadcast called with null pointer as first argument.");
    abort();
    return;
  }

  Option<SymAddr> addr = GetSymAddr(cnd);
  if (!addr) return;
  if(!TB.cond_broadcast({*addr,1})){ // also acts as a fence
    abort();
    return;
  }

  GenericValue Result;
  /* pthread_cond_wait always returns 0 */
  Result.IntVal = APInt(F->getReturnType()->getIntegerBitWidth(),0);
  returnValueToCaller(F->getReturnType(),Result);
}

void Interpreter::callPthreadCondWait(Function *F,
                                      const std::vector<GenericValue> &ArgVals){
  GenericValue *cnd = (GenericValue*)GVTOP(ArgVals[0]);
  GenericValue *lck = (GenericValue*)GVTOP(ArgVals[1]);

  if(!cnd){
    TB.pthreads_error("pthread_cond_wait called with null pointer as first argument.");
    abort();
    return;
  }

  if(!lck){
    TB.pthreads_error("pthread_cond_wait called with null pointer as second argument.");
    abort();
    return;
  }

  Option<SymAddr> cnd_sa = GetSymAddr(cnd);
  Option<SymAddr> lck_sa = GetSymAddr(lck);
  if (!cnd_sa || !lck_sa) return;
  if(!TB.cond_wait({*cnd_sa,1},{*lck_sa,1})){ // also acts as a fence
    abort();
    return;
  }

  GenericValue Result;
  /* pthread_cond_wait always returns 0 */
  Result.IntVal = APInt(F->getReturnType()->getIntegerBitWidth(),0);
  returnValueToCaller(F->getReturnType(),Result);

  if(DryRun) return;

  assert(PthreadMutexes.count(lck));
  assert(PthreadMutexes[lck].owner == CurrentThread);
  PthreadMutexes[lck].unlock();
  for(int p : PthreadMutexes[lck].waiting){
    TB.mark_available(p);
  }
  PthreadMutexes[lck].waiting.clear();

  Threads[CurrentThread].pending_mutex_lock = lck;
  Threads[CurrentThread].pending_condvar_awake = cnd;
}

void Interpreter::doPthreadCondAwake(void *cnd, void *lck){
  assert(lck);

  Option<SymAddr> cnd_sa = GetSymAddr(cnd);
  Option<SymAddr> lck_sa = GetSymAddr(lck);
  if (!cnd_sa || !lck_sa) return;
  if(!TB.cond_awake({*cnd_sa,1},{*lck_sa,1})){ // also acts as a fence
    abort();
    return;
  }

  if(PthreadMutexes.count(lck) == 0){
    /* We don't need to check conf.mutex_require_init as the mutex is always
     * initialised during callPthreadCondWait().
     */
    TB.pthreads_error("mutex destroyed during pthread_cond_wait.");
    abort();
    return;
  }

  assert(PthreadMutexes.count(lck) == 0 || PthreadMutexes[lck].isUnlocked());

  if(DryRun) return;
  PthreadMutexes[lck].lock(CurrentThread);
}

void Interpreter::callPthreadCondDestroy(Function *F,
                                         const std::vector<GenericValue> &ArgVals){
  GenericValue *cnd = (GenericValue*)GVTOP(ArgVals[0]);

  if(!cnd){
    TB.pthreads_error("pthread_cond_destroy called with null pointer as first argument.");
    abort();
    return;
  }

  Option<SymAddr> addr = GetSymAddr(cnd);
  if (!addr) return;
  int rv = TB.cond_destroy({*addr,1}); // also acts as a fence

  if(rv == 0 || rv == EBUSY){
    GenericValue Result;
    Result.IntVal = APInt(F->getReturnType()->getIntegerBitWidth(),rv); // Success or EBUSY
    returnValueToCaller(F->getReturnType(),Result);
  }else{
    // Pthreads error
    abort();
  }
}

void Interpreter::callNondetInt(Function *F, const std::vector<GenericValue> &ArgVals){
  int int_result;
  if (conf.svcomp_nondet_int) {
    int_result = *conf.svcomp_nondet_int;
  } else {
    std::uniform_int_distribution<int> distr(std::numeric_limits<int>::min(),
                                             std::numeric_limits<int>::max());
    int_result = distr(Threads[CurrentThread].RandEng);
  }
  GenericValue Result;
  Result.IntVal = APInt(F->getReturnType()->getIntegerBitWidth(),int_result,true);
  returnValueToCaller(F->getReturnType(),Result);
}

void Interpreter::callAssume(Function *F, const std::vector<GenericValue> &ArgVals){
  bool cond = ArgVals[0].IntVal.getBoolValue();
  if(!cond){
    if(DryRun){
      /* Do not clear the stack. */
      while(0 <= AtomicFunctionCall && AtomicFunctionCall < int(ECStack()->size())){
        /* We are inside an atomic function call. Remove the top part
         * of the stack corresponding to that call.
         */
        popStackAndReturnValueToCaller(Type::getVoidTy(F->getContext()), GenericValue());
      }
      return;
    }
    setAssumeBlocked(true);
    ECStack()->clear();
    AtExitHandlers.clear();
    Threads[CurrentThread].AssumeBlocked = true;
    /* Do not call terminate. We don't want to explicitly terminate
     * since that would allow other processes to join with this
     * process.
     */
  }
}

void Interpreter::callMCalloc(Function *F,
                              const std::vector<GenericValue> &ArgVals,
                              bool isCalloc){
  if(conf.malloc_may_fail) {
    if(!TB.register_alternatives(2)){
      abort();
      return;
    }
  }
  if(conf.malloc_may_fail && CurrentAlt == 0){
    GenericValue Result;
    Result.PointerVal = 0; // Return null
    returnValueToCaller(F->getReturnType(),Result);
  }else{// else call as usual
    GenericValue Result;
    void *Memory;
    uint64_t Size;
    assert(ArgVals[0].IntVal.getBitWidth() <= 64);
    if(isCalloc){
      uint64_t nm = ArgVals[0].IntVal.getLimitedValue();
      uint64_t sz = ArgVals[1].IntVal.getLimitedValue();
      Memory = calloc(nm,sz);
      Size = nm * sz;
    }else{ // malloc
      Size = ArgVals[0].IntVal.getLimitedValue();
      Memory = malloc(Size);
    }
    Result.PointerVal = Memory;
    AllocatedMemHeap.insert(Result.PointerVal);
    SymMBlock mb = SymMBlock::Heap(CurrentThread, HeapAllocCount[CurrentThread]++);
    AllocatedMem.emplace(Memory, SymMBlockSize(std::move(mb), Size));
    returnValueToCaller(F->getReturnType(),Result);
  }
}

void Interpreter::callFree(Function *F,
                           const std::vector<GenericValue> &ArgVals){
  void *ptr = (void*)GVTOP(ArgVals[0]);

  if(!ptr){
    /* When the argument is NULL, free does nothing. */
    return;
  }

  if(AllocatedMemStack.count(ptr)){
    TB.memory_error("Attempt to free block which is on the stack.");
    abort();
    return;
  }
  if(!AllocatedMemHeap.count(ptr)){
    TB.memory_error("Attempt to free address not returned by malloc.");
    abort();
    return;
  }

  auto pr = FreedMem.insert({ptr,TB.get_iid()});

  if(!pr.second){ // Double free
    TB.segmentation_fault_error();
    abort();
    return;
  }
}

void Interpreter::callAtexit(Function *F,
                             const std::vector<GenericValue> &ArgVals){
  Function *Handler = (Function*)GVTOP(ArgVals[0]);
  if (!ValidateFunctionPointer(Handler)) return;
  addAtExitHandler(Handler);
  GenericValue Result;
  Result.IntVal = APInt(F->getReturnType()->getIntegerBitWidth(),0);
  returnValueToCaller(F->getReturnType(),Result);
}

void Interpreter::callAssertFail(Function *F,
                                 const std::vector<GenericValue> &ArgVals){
  std::string err;
  if(ArgVals.size()){
    err = (char*)GVTOP(ArgVals[0]);
  }else{
    err = "unknown";
  }
  TB.assertion_error(err);
  abort();
}

void Interpreter::callLoadAwait(Function *F,
                                const std::vector<GenericValue> &ArgVals){
  GenericValue *Ptr = (GenericValue*)GVTOP(ArgVals[0]);
  uint64_t op_int = ArgVals[1].IntVal.getZExtValue();
  AwaitCond::Op op(static_cast<AwaitCond::Op>(op_int));
   /* Should be arg2 type, but if well-formed is same as return type */
  Type *Ty = F->getReturnType();
  SymData::block_type operand = SymData::alloc_block
#ifdef LLVM_EXECUTIONENGINE_DATALAYOUT_PTR
    (getDataLayout()->getTypeStoreSize(Ty));
#else
    (getDataLayout().getTypeStoreSize(Ty));
#endif
  StoreValueToMemory(ArgVals[2],
                     static_cast<GenericValue*>((void*)operand.get()),Ty);

  GenericValue Result;
  Option<SymAddrSize> Ptr_sas = GetSymAddrSize(Ptr,Ty);
  if(!Ptr_sas) return;

  if(!TB.load_await(*Ptr_sas, AwaitCond{op, std::move(operand)})){
    abort();
    return;
  }

  assert(!(DryRun && DryRunMem.size()));
  LoadValueFromMemory(Result, Ptr, Ty);
  returnValueToCaller(F->getReturnType(),Result);
}

 void Interpreter::callXchgAwait(Function *F,
                                const std::vector<GenericValue> &ArgVals){
  GenericValue *Ptr = (GenericValue*)GVTOP(ArgVals[0]);
  const GenericValue NewVal = ArgVals[1];
  uint64_t op_int = ArgVals[2].IntVal.getZExtValue();
  AwaitCond::Op op(static_cast<AwaitCond::Op>(op_int));
   /* Should be arg2 type, but if well-formed is same as return type */
  Type *Ty = F->getReturnType();
  std::size_t Ty_size =
#ifdef LLVM_EXECUTIONENGINE_DATALAYOUT_PTR
    getDataLayout()->getTypeStoreSize(Ty);
#else
    getDataLayout().getTypeStoreSize(Ty);
#endif
  SymData::block_type operand = SymData::alloc_block(Ty_size);
  StoreValueToMemory(ArgVals[3],
                     static_cast<GenericValue*>((void*)operand.get()),Ty);
  AwaitCond cond{op, std::move(operand)};

  Option<SymAddrSize> Ptr_sas = GetSymAddrSize(Ptr,Ty);
  if(!Ptr_sas) return;

  GenericValue OldVal;
  assert(!(DryRun && DryRunMem.size()));
  LoadValueFromMemory(OldVal, Ptr, Ty);
#ifndef NDEBUG
  SymData::block_type OldValBlock = SymData::alloc_block(Ptr_sas->size);
  memcpy(OldValBlock.get(), (const void*)Ptr, Ptr_sas->size);
  assert(cond.satisfied_by(OldValBlock.get(), Ptr_sas->size));
#endif

  SymData sd = GetSymData(*Ptr_sas,Ty,NewVal);

  if(!TB.xchg_await(sd, std::move(cond))){
    abort();
    return;
  }

  /* Store NewVal */
  if(DryRun){
    DryRunMem.emplace_back(std::move(sd));
    return;
  }
  StoreValueToMemory(NewVal,Ptr,Ty);
  CheckAwaitWakeup(NewVal, Ptr, *Ptr_sas);

  returnValueToCaller(F->getReturnType(),OldVal);
}

void Interpreter::CheckAwaitWakeup(const GenericValue &Val, const void *ptr,
                                   const SymAddrSize &sas) {
  assert(!DryRun);

  auto sas_awaits_iter = blocking_awaits.find(sas);
  if (sas_awaits_iter == blocking_awaits.end()) return;
  auto &sas_awaits = sas_awaits_iter->second;

  /* Complexity could be improved, but it's unlikely to matter. */
  for (auto it = sas_awaits.begin(); it != sas_awaits.end();) {
    if (it->second.satisfied_by(ptr, sas.size)) {
      TB.mark_available(it->first);
      it = sas_awaits.erase(it);
    } else {
      ++it;
    }
  }
  if (sas_awaits.empty()) blocking_awaits.erase(sas_awaits_iter);
}

//===----------------------------------------------------------------------===//
// callFunction - Execute the specified function...
//
void Interpreter::callFunction(Function *F,
                               const std::vector<GenericValue> &ArgVals) {
  assert(F && "Caller should validate F");
  if(F->getName().str() == "pthread_create"){
    callPthreadCreate(F,ArgVals);
    return;
  }else if(F->getName().str() == "pthread_join"){
    callPthreadJoin(F,ArgVals);
    return;
  }else if(F->getName().str() == "pthread_self"){
    callPthreadSelf(F,ArgVals);
    return;
  }else if(F->getName().str() == "pthread_exit"){
    callPthreadExit(F,ArgVals);
    return;
  }else if(F->getName().str() == "pthread_mutex_init"){
    callPthreadMutexInit(F,ArgVals);
    return;
  }else if(F->getName().str() == "pthread_mutex_lock"){
    callPthreadMutexLock(F,ArgVals);
    return;
  }else if(F->getName().str() == "pthread_mutex_trylock"){
    callPthreadMutexTryLock(F,ArgVals);
    return;
  }else if(F->getName().str() == "pthread_mutex_unlock"){
    callPthreadMutexUnlock(F,ArgVals);
    return;
  }else if(F->getName().str() == "pthread_mutex_destroy"){
    callPthreadMutexDestroy(F,ArgVals);
    return;
  }else if(F->getName().str() == "pthread_cond_init"){
    callPthreadCondInit(F,ArgVals);
    return;
  }else if(F->getName().str() == "pthread_cond_signal"){
    callPthreadCondSignal(F,ArgVals);
    return;
  }else if(F->getName().str() == "pthread_cond_broadcast"){
    callPthreadCondBroadcast(F,ArgVals);
    return;
  }else if(F->getName().str() == "pthread_cond_wait"){
    callPthreadCondWait(F,ArgVals);
    return;
  }else if(F->getName().str() == "pthread_cond_destroy"){
    callPthreadCondDestroy(F,ArgVals);
    return;
  }else if(F->getName().str() == "malloc"){
    callMCalloc(F,ArgVals,false);
    return;
  }else if(F->getName().str() == "calloc"){
    callMCalloc(F,ArgVals,true);
    return;
  }else if(F->getName().str() == "free"){
    callFree(F,ArgVals);
    return;
  }else if(F->getName().str() == "atexit"){
    callAtexit(F,ArgVals);
    return;
  }else if(F->getName().str() == "__VERIFIER_nondet_int" ||
           F->getName().str() == "__VERIFIER_nondet_uint"){
    callNondetInt(F,ArgVals);
    return;
  }else if(F->getName().str() == "__VERIFIER_assume"){
    callAssume(F,ArgVals);
    return;
  }else if(F->getName().str() == "__assert_fail"){
    callAssertFail(F,ArgVals);
    return;
  }else if(F->getName().str().rfind("__VERIFIER_load_await", 0) == 0){
    callLoadAwait(F,ArgVals);
    return;
  }else if(F->getName().str().rfind("__VERIFIER_xchg_await", 0) == 0){
    callXchgAwait(F,ArgVals);
    return;
  }

  assert((ECStack()->empty() || ECStack()->back().Caller == nullptr ||
          ECStack()->back().Caller.arg_size() == ArgVals.size()) &&
         "Incorrect number of arguments passed into function call!");

  if(F->getName().str().find("__VERIFIER_atomic_") == 0){
    if (conf.dpor_algorithm == Configuration::OBSERVERS)
      Debug::warn("optimal+atomic")
        << "WARNING: Support for atomic blocks is limited with --optimal.\n"
           "         Nidhugg might crash or miss bugs, see the manual.\n";
    if(!TB.fence()){
      abort();
      return;
    }
    if(AtomicFunctionCall < 0){
      AtomicFunctionCall = ECStack()->size();
    } // else we are already inside an atomic function call
  }

  // Make a new stack frame... and fill it in.
  ECStack()->push_back(ExecutionContext());
  ExecutionContext &StackFrame = ECStack()->back();
  StackFrame.CurFunction = F;

  // Special handling for external functions.
  if (F->isDeclaration()) {
    // Memory fence
    if(!conf.extfun_no_fence.count(F->getName().str())){
      if(!TB.fence()){
        abort();
        return;
      }
    }
    if(!conf.extfun_no_full_memory_conflict.count(F->getName().str())){
      Debug::warn("unknown external:"+F->getName().str())
        << "WARNING: Calling unknown external function "
        << F->getName().str()
        << " as blackbox.\n";

      if(!TB.full_memory_conflict()){
        abort();
        return;
      }
    }

    if(DryRun){
      ECStack()->pop_back();
      return;
    }

    GenericValue Result = callExternalFunction (F, ArgVals);
    // Simulate a 'ret' instruction of the appropriate type.
    popStackAndReturnValueToCaller (F->getReturnType (), Result);
    return;
  }

  // Get pointers to first LLVM BB & Instruction in function.
  StackFrame.CurBB     = &F->front();
  StackFrame.CurInst   = StackFrame.CurBB->begin();

  // Run through the function arguments and initialize their values...
  assert((ArgVals.size() == F->arg_size() ||
         (ArgVals.size() > F->arg_size() && F->getFunctionType()->isVarArg()))&&
         "Invalid number of values passed to function invocation!");

  // Handle non-varargs arguments...
  unsigned i = 0;
  for (Function::arg_iterator AI = F->arg_begin(), E = F->arg_end();
       AI != E; ++AI, ++i){
    SetValue(&*AI, ArgVals[i], StackFrame);
  }

  // Handle varargs arguments...
  StackFrame.VarArgs.assign(ArgVals.begin()+i, ArgVals.end());
}

/* Strip away whitespace from the beginning and end of s. */
static void stripws(std::string &s){
  int first = 0, len;
  while(first < int(s.size()) && std::isspace(s[first])) ++first;
  len = s.size() - first;
  while(0 < len && std::isspace(s[first+len-1])) --len;
  s = s.substr(first,len);
}

bool Interpreter::isInlineAsm(AnyCallInst ACI, std::string *asmstr){
  if(llvm::CallInst *CI = dyn_cast<llvm::CallInst>(&ACI)){
    if(CI){
      if(CI->isInlineAsm()){
        llvm::InlineAsm *IA = llvm::dyn_cast<llvm::InlineAsm>(ACI.getCalledOperand());
        assert(IA);
        *asmstr = IA->getAsmString();
        stripws(*asmstr);
        return true;
      }
    }
  }
  return false;
}

bool Interpreter::isUnknownIntrinsic(Instruction &I){
  if(CallInst *CI = dyn_cast<CallInst>(&I)){
    Function *F = CI->getCalledFunction();
    if(F && F->isDeclaration() &&
       F->getIntrinsicID() != Intrinsic::not_intrinsic &&
       F->getIntrinsicID() != Intrinsic::vastart &&
       F->getIntrinsicID() != Intrinsic::vaend &&
       F->getIntrinsicID() != Intrinsic::vacopy){
      return true;
    }
  }
  return false;
}

static AnyCallInst isCallOrInvoke(Instruction &I) {
  if (CallInst *CI = dyn_cast<CallInst>(&I)) {
    return {CI};
  } else if(InvokeInst *II = dyn_cast<InvokeInst>(&I)) {
    return {II};
  } else {
    return AnyCallInst();
  }
}

bool Interpreter::isPthreadJoin(Instruction &I, int *tid){
  AnyCallInst CI = isCallOrInvoke(I);
  if(!CI) return false;
  Function *F = CI.getCalledFunction();
  if(!F || F->getName() != "pthread_join") return false;
  llvm::GenericValue gv_tid =
    getOperandValue(*CI.arg_begin(), ECStack()->back());
  *tid = pthread_t_to_tid(F->arg_begin()->getType(), gv_tid);
  return true;
}

bool Interpreter::isPthreadMutexLock(Instruction &I, GenericValue **ptr){
  if(Threads[CurrentThread].pending_mutex_lock){
    *ptr = (llvm::GenericValue*)Threads[CurrentThread].pending_mutex_lock;
    return true;
  }
  AnyCallInst CI = isCallOrInvoke(I);
  if(!CI) return false;
  Function *F = CI.getCalledFunction();
  if(!F || F->getName() != "pthread_mutex_lock") return false;
  *ptr = (GenericValue*)GVTOP(getOperandValue(*CI.arg_begin(),ECStack()->back()));
  return true;
}

bool Interpreter::isLoadAwait(Instruction &I, GenericValue **ptr, AwaitCond *cond){
  return isAnyAwait(I, ptr, cond, "__VERIFIER_load_await", 0);
}

bool Interpreter::isXchgAwait(Instruction &I, GenericValue **ptr, AwaitCond *cond){
  return isAnyAwait(I, ptr, cond, "__VERIFIER_xchg_await", 1);
}

bool Interpreter::isAnyAwait(Instruction &I, GenericValue **ptr, AwaitCond *cond,
                             const char *name_prefix, unsigned nargs){
  if(!isa<CallInst>(I)) return false;
  AnyCallInst CI(static_cast<CallInst*>(&I));
  Function *F = CI.getCalledFunction();
  if(!F || F->getName().str().rfind(name_prefix, 0) != 0) return false;
  auto args = CI.arg_begin();
  *ptr = (GenericValue*)GVTOP(getOperandValue(args[0],ECStack()->back()));
  uint64_t op_int = getOperandValue(args[nargs+1],ECStack()->back()).IntVal.getZExtValue();
  AwaitCond::Op op(static_cast<AwaitCond::Op>(op_int));

  Type *Ty = args[nargs+2]->getType();
  SymData::block_type operand = SymData::alloc_block
#ifdef LLVM_EXECUTIONENGINE_DATALAYOUT_PTR
    (getDataLayout()->getTypeStoreSize(Ty));
#else
    (getDataLayout().getTypeStoreSize(Ty));
#endif
  StoreValueToMemory(getOperandValue(args[nargs+2],ECStack()->back()),
                     static_cast<GenericValue*>((void*)operand.get()),Ty);

  *cond = AwaitCond{op, std::move(operand)};
  return true;
}

bool Interpreter::checkRefuse(Instruction &I){
  {
    int tid;
    if(isPthreadJoin(I,&tid)){
      if(0 <= tid && tid < int(Threads.size()) && tid != CurrentThread){
        if(Threads[tid].ECStack.size() || Threads[tid].AssumeBlocked) {
          /* The awaited thread is still executing. */
          TB.refuse_schedule();
          Threads[tid].AwaitingJoin.push_back(CurrentThread);
          return true;
        }
      }else{
        // Erroneous thread id
        // Allow execution (will produce an error trace)
      }
    }
  }
  {
    GenericValue *ptr;
    Option<SymAddr> addr;
    if(isPthreadMutexLock(I,&ptr)){
      if((addr = TryGetSymAddr(ptr)) &&
         PthreadMutexes.count(ptr) &&
         PthreadMutexes[ptr].isLocked()){
        if(!TB.mutex_lock_fail({*addr,1})){
          abort();
          return true;
        }
        TB.refuse_schedule();
        PthreadMutexes[ptr].waiting.insert(CurrentThread);
        return true;
      }else{
        // Either unlocked mutex, or uninitialized mutex.
        // In both cases let callPthreadMutex handle it.
      }
    }
  }
  {
    GenericValue *ptr;
    AwaitCond cond;
    enum kind { NONE = 0, LOAD = 1, XCHG = 2, } kind = NONE;
    int nargs = 0;
    if(isLoadAwait(I, &ptr, &cond)) {
      kind = LOAD;
    } else if(isXchgAwait(I, &ptr, &cond)) {
      kind = XCHG;
      nargs = 1;
    }
    if(kind != NONE) {
      Option<SymAddrSize> ptr_sas = TryGetSymAddrSize(ptr,I.getOperand(nargs+2)->getType());
      if (ptr_sas) {
        assert(!(DryRun && DryRunMem.size()));
        SymData::block_type actual = SymData::alloc_block(ptr_sas->size);
        memcpy(actual.get(), (const void*)ptr, ptr_sas->size);
        if (!cond.satisfied_by((const void*)ptr, ptr_sas->size)) {
          assert(!cond.satisfied_by(actual.get(), ptr_sas->size));
          bool callback_ret;
          if (kind == LOAD) {
            callback_ret = TB.load_await_fail(*ptr_sas, cond);
          } else {
            assert(kind == XCHG);
            ExecutionContext &SF = ECStack()->back();
            GenericValue Val = getOperandValue(I.getOperand(1), SF);
            SymData sd = GetSymData(*ptr_sas,I.getOperand(1)->getType(),Val);
            callback_ret = TB.xchg_await_fail(sd, cond);
          }
          if(!callback_ret) {
            abort();
            return true;
          }
          TB.refuse_schedule();
          /* TODO: Overlapping accesses */
          blocking_awaits[*ptr_sas].emplace(CurrentThread, std::move(cond));
          return true;
        }
      } else {
        Debug::warn("await-bad-addr")
          << "Await will segfault";
        /* Bad address, let execute and crash */
      }
    }
  }
  return false;
}

void Interpreter::terminate(Type *RetTy, GenericValue Result){
  if(CurrentThread != 0){
    assert(RetTy == Type::getInt8PtrTy(RetTy->getContext()));
    Threads[CurrentThread].RetVal = Result;
  }
  for(int p : Threads[CurrentThread].AwaitingJoin){
    TB.mark_available(p);
  }
}

void Interpreter::clearAllStacks(){
  if(assumeBlocked()) Blocked = true;
  for(unsigned i = 0; i < Threads.size(); ++i){
    if(!Threads[i].ECStack.empty()) Blocked = true;
    Threads[i].ECStack.clear();
  }
}

void Interpreter::abort(){
  TB.cancel_replay();
  for(unsigned i = 0; i < Threads.size(); ++i){
    TB.mark_unavailable(i);
  }
  clearAllStacks();
}

Option<SymAddr> Interpreter::TryGetSymAddr(void *Ptr) {
  auto ub = AllocatedMem.upper_bound(Ptr);
  if (ub == AllocatedMem.begin()) return nullptr;
  --ub;
  if ((char*)ub->first + ub->second.size <= Ptr) return nullptr;
  return SymAddr(ub->second.block, (char*)Ptr - (char*)ub->first);
}

Option<SymAddr> Interpreter::GetSymAddr(void *Ptr) {
  Option<SymAddr> ret = TryGetSymAddr(Ptr);
  if (!ret) {
    if (DryRun) {
      TB.nondeterminism_error("Address of memory access became undefined "
                              "in replay");
    } else {
      TB.segmentation_fault_error();
    }
    abort();
  }
  return ret;
}

bool Interpreter::ValidateFunctionPointer(Function *F) {
  if (!F) {
    TB.segmentation_fault_error();
    abort();
    return false;
  }
  return true;
}

void Interpreter::run() {
  int aux;
  bool rerun = false;
  while(rerun || TB.schedule(&CurrentThread,&aux,&CurrentAlt,&DryRun)){
    assert(0 <= CurrentThread && CurrentThread < int_fast64_t(Threads.size()));
    rerun = false;
    if(0 <= aux){ // Run some auxiliary thread
      runAux(CurrentThread,aux);
      continue;
    }

    // Interpret a single instruction & increment the "PC".
    ExecutionContext &SF = ECStack()->back();  // Current stack frame
    Instruction &I = *SF.CurInst++;            // Increment before execute

    if(isUnknownIntrinsic(I)){
      /* This instruction is intrinsic. It will be removed from the IR
       * and replaced by some new sequence of instructions. Executing
       * the intrinsic itself does not count as executing an
       * instruction. The flag rerun indicates that the same process
       * should be scheduled once more, so that a real instruction may
       * be executed.
       */
      rerun = true;
    }else if(checkRefuse(I)){
      if (!ECStack()->empty()) {
        /* Revert without executing the next instruction. */
        --SF.CurInst;
      }
      continue;
    }

    if(Threads[CurrentThread].pending_mutex_lock){
      doPthreadCondAwake(Threads[CurrentThread].pending_condvar_awake,
                         Threads[CurrentThread].pending_mutex_lock);
      if(!DryRun) {
        Threads[CurrentThread].pending_mutex_lock = 0;
        Threads[CurrentThread].pending_condvar_awake = 0;
      }
    }

    TB.metadata(I.getMetadata("dbg"));

    assert(DryRunMem.empty());

    if(DryRun && I.isTerminator()){
      /* Revert without executing the next instruction. */
      --SF.CurInst;
      continue;
    }

    /* Execute */
    visit(I);

    /* Atomic function? */
    if(0 <= AtomicFunctionCall){
      /* We have entered an atomic function.
       * Keep executing until we exit it.
       */
      while(AtomicFunctionCall < int(ECStack()->size())){
        ExecutionContext &SF = ECStack()->back();  // Current stack frame
        Instruction &I = *SF.CurInst++;         // Increment before execute
        visit(I);
      }
      AtomicFunctionCall = -1;
    }

    if(ECStack()->empty()){ // The thread has terminated
      if(CurrentThread == 0 && AtExitHandlers.size()){
        callFunction(AtExitHandlers.back(),{});
        AtExitHandlers.pop_back();
      }else{
        TB.mark_unavailable(CurrentThread);
      }
    }

    if(DryRun && !rerun){ // Did dry run. Now back up.
      --ECStack()->back().CurInst;
      DryRunMem.clear();
    }
  }
  CurrentThread = 0;
  clearAllStacks();
}
