// For the parts of the code originating in LLVM:
//===-- Execution.cpp - Implement code to simulate the program ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file contains the actual instruction interpreter.
//
//===----------------------------------------------------------------------===//

/* Copyright (C) 2015-2017 Carl Leonardsson
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

#include "Debug.h"
#include "POWERInterpreter.h"

#include <llvm/ADT/APInt.h>
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
#if defined(HAVE_LLVM_SUPPORT_GETELEMENTPTRTYPEITERATOR_H)
#include <llvm/Support/GetElementPtrTypeIterator.h>
#elif defined(HAVE_LLVM_IR_GETELEMENTPTRTYPEITERATOR_H)
#include <llvm/IR/GetElementPtrTypeIterator.h>
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
#include <llvm/Support/MathExtras.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

//===----------------------------------------------------------------------===//
//                     Various Helper Functions
//===----------------------------------------------------------------------===//

void POWERInterpreter::setCurInstrValue(const llvm::GenericValue &Val) {
  assert(!CurInstr->Committed);
  CurInstr->Value = Val;
}

//===----------------------------------------------------------------------===//
//                    Binary Instruction Implementations
//===----------------------------------------------------------------------===//

#define IMPLEMENT_BINARY_OPERATOR(OP, TY)       \
  case llvm::Type::TY##TyID:                    \
  Dest.TY##Val = Src1.TY##Val OP Src2.TY##Val;  \
  break

static void executeFAddInst(llvm::GenericValue &Dest, llvm::GenericValue Src1,
                            llvm::GenericValue Src2, llvm::Type *Ty) {
  switch (Ty->getTypeID()) {
    IMPLEMENT_BINARY_OPERATOR(+, Float);
    IMPLEMENT_BINARY_OPERATOR(+, Double);
  default:
    llvm::dbgs() << "Unhandled type for FAdd instruction: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
}

static void executeFSubInst(llvm::GenericValue &Dest, llvm::GenericValue Src1,
                            llvm::GenericValue Src2, llvm::Type *Ty) {
  switch (Ty->getTypeID()) {
    IMPLEMENT_BINARY_OPERATOR(-, Float);
    IMPLEMENT_BINARY_OPERATOR(-, Double);
  default:
    llvm::dbgs() << "Unhandled type for FSub instruction: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
}

static void executeFMulInst(llvm::GenericValue &Dest, llvm::GenericValue Src1,
                            llvm::GenericValue Src2, llvm::Type *Ty) {
  switch (Ty->getTypeID()) {
    IMPLEMENT_BINARY_OPERATOR(*, Float);
    IMPLEMENT_BINARY_OPERATOR(*, Double);
  default:
    llvm::dbgs() << "Unhandled type for FMul instruction: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
}

static void executeFDivInst(llvm::GenericValue &Dest, llvm::GenericValue Src1,
                            llvm::GenericValue Src2, llvm::Type *Ty) {
  switch (Ty->getTypeID()) {
    IMPLEMENT_BINARY_OPERATOR(/, Float);
    IMPLEMENT_BINARY_OPERATOR(/, Double);
  default:
    llvm::dbgs() << "Unhandled type for FDiv instruction: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
}

static void executeFRemInst(llvm::GenericValue &Dest, llvm::GenericValue Src1,
                            llvm::GenericValue Src2, llvm::Type *Ty) {
  switch (Ty->getTypeID()) {
  case llvm::Type::FloatTyID:
    Dest.FloatVal = fmod(Src1.FloatVal, Src2.FloatVal);
    break;
  case llvm::Type::DoubleTyID:
    Dest.DoubleVal = fmod(Src1.DoubleVal, Src2.DoubleVal);
    break;
  default:
    llvm::dbgs() << "Unhandled type for Rem instruction: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
}

#define IMPLEMENT_INTEGER_ICMP(OP, TY)                      \
  case llvm::Type::IntegerTyID:                             \
  Dest.IntVal = llvm::APInt(1,Src1.IntVal.OP(Src2.IntVal)); \
  break;

#define IMPLEMENT_VECTOR_INTEGER_ICMP(OP, TY)                           \
  case llvm::Type::VectorTyID: {                                        \
    assert(Src1.AggregateVal.size() == Src2.AggregateVal.size());       \
    Dest.AggregateVal.resize( Src1.AggregateVal.size() );               \
    for( uint32_t _i=0;_i<Src1.AggregateVal.size();_i++)                \
      Dest.AggregateVal[_i].IntVal = llvm::APInt(1,                     \
                                                 Src1.AggregateVal[_i].IntVal.OP(Src2.AggregateVal[_i].IntVal)); \
  } break;

// Handle pointers specially because they must be compared with only as much
// width as the host has.  We _do not_ want to be comparing 64 bit values when
// running on a 32-bit target, otherwise the upper 32 bits might mess up
// comparisons if they contain garbage.
#define IMPLEMENT_POINTER_ICMP(OP)                                \
  case llvm::Type::PointerTyID:                                   \
  Dest.IntVal = llvm::APInt(1,(void*)(intptr_t)Src1.PointerVal OP \
                            (void*)(intptr_t)Src2.PointerVal);    \
  break;

static llvm::GenericValue executeICMP_EQ(llvm::GenericValue Src1, llvm::GenericValue Src2,
                                         llvm::Type *Ty) {
  llvm::GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(eq,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(eq,Ty);
    IMPLEMENT_POINTER_ICMP(==);
  default:
    llvm::dbgs() << "Unhandled type for ICMP_EQ predicate: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

static llvm::GenericValue executeICMP_NE(llvm::GenericValue Src1, llvm::GenericValue Src2,
                                         llvm::Type *Ty) {
  llvm::GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(ne,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(ne,Ty);
    IMPLEMENT_POINTER_ICMP(!=);
  default:
    llvm::dbgs() << "Unhandled type for ICMP_NE predicate: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

static llvm::GenericValue executeICMP_ULT(llvm::GenericValue Src1, llvm::GenericValue Src2,
                                          llvm::Type *Ty) {
  llvm::GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(ult,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(ult,Ty);
    IMPLEMENT_POINTER_ICMP(<);
  default:
    llvm::dbgs() << "Unhandled type for ICMP_ULT predicate: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

static llvm::GenericValue executeICMP_SLT(llvm::GenericValue Src1, llvm::GenericValue Src2,
                                          llvm::Type *Ty) {
  llvm::GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(slt,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(slt,Ty);
    IMPLEMENT_POINTER_ICMP(<);
  default:
    llvm::dbgs() << "Unhandled type for ICMP_SLT predicate: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

static llvm::GenericValue executeICMP_UGT(llvm::GenericValue Src1, llvm::GenericValue Src2,
                                          llvm::Type *Ty) {
  llvm::GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(ugt,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(ugt,Ty);
    IMPLEMENT_POINTER_ICMP(>);
  default:
    llvm::dbgs() << "Unhandled type for ICMP_UGT predicate: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

static llvm::GenericValue executeICMP_SGT(llvm::GenericValue Src1, llvm::GenericValue Src2,
                                          llvm::Type *Ty) {
  llvm::GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(sgt,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(sgt,Ty);
    IMPLEMENT_POINTER_ICMP(>);
  default:
    llvm::dbgs() << "Unhandled type for ICMP_SGT predicate: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

static llvm::GenericValue executeICMP_ULE(llvm::GenericValue Src1, llvm::GenericValue Src2,
                                          llvm::Type *Ty) {
  llvm::GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(ule,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(ule,Ty);
    IMPLEMENT_POINTER_ICMP(<=);
  default:
    llvm::dbgs() << "Unhandled type for ICMP_ULE predicate: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

static llvm::GenericValue executeICMP_SLE(llvm::GenericValue Src1, llvm::GenericValue Src2,
                                          llvm::Type *Ty) {
  llvm::GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(sle,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(sle,Ty);
    IMPLEMENT_POINTER_ICMP(<=);
  default:
    llvm::dbgs() << "Unhandled type for ICMP_SLE predicate: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

static llvm::GenericValue executeICMP_UGE(llvm::GenericValue Src1, llvm::GenericValue Src2,
                                          llvm::Type *Ty) {
  llvm::GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(uge,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(uge,Ty);
    IMPLEMENT_POINTER_ICMP(>=);
  default:
    llvm::dbgs() << "Unhandled type for ICMP_UGE predicate: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

static llvm::GenericValue executeICMP_SGE(llvm::GenericValue Src1, llvm::GenericValue Src2,
                                          llvm::Type *Ty) {
  llvm::GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(sge,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(sge,Ty);
    IMPLEMENT_POINTER_ICMP(>=);
  default:
    llvm::dbgs() << "Unhandled type for ICMP_SGE predicate: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

void POWERInterpreter::visitICmpInst(llvm::ICmpInst &I) {
  llvm::Type *Ty    = I.getOperand(0)->getType();
  llvm::GenericValue Src1 = getOperandValue(0);
  llvm::GenericValue Src2 = getOperandValue(1);
  llvm::GenericValue R;   // Result

  switch (I.getPredicate()) {
  case llvm::ICmpInst::ICMP_EQ:  R = executeICMP_EQ(Src1,  Src2, Ty); break;
  case llvm::ICmpInst::ICMP_NE:  R = executeICMP_NE(Src1,  Src2, Ty); break;
  case llvm::ICmpInst::ICMP_ULT: R = executeICMP_ULT(Src1, Src2, Ty); break;
  case llvm::ICmpInst::ICMP_SLT: R = executeICMP_SLT(Src1, Src2, Ty); break;
  case llvm::ICmpInst::ICMP_UGT: R = executeICMP_UGT(Src1, Src2, Ty); break;
  case llvm::ICmpInst::ICMP_SGT: R = executeICMP_SGT(Src1, Src2, Ty); break;
  case llvm::ICmpInst::ICMP_ULE: R = executeICMP_ULE(Src1, Src2, Ty); break;
  case llvm::ICmpInst::ICMP_SLE: R = executeICMP_SLE(Src1, Src2, Ty); break;
  case llvm::ICmpInst::ICMP_UGE: R = executeICMP_UGE(Src1, Src2, Ty); break;
  case llvm::ICmpInst::ICMP_SGE: R = executeICMP_SGE(Src1, Src2, Ty); break;
  default:
    llvm::dbgs() << "Don't know how to handle this ICmp predicate!\n-->" << I;
    llvm_unreachable(nullptr);
  }

  setCurInstrValue(R);
}

#define IMPLEMENT_FCMP(OP, TY)                                \
  case llvm::Type::TY##TyID:                                  \
  Dest.IntVal = llvm::APInt(1,Src1.TY##Val OP Src2.TY##Val);  \
  break

#define IMPLEMENT_VECTOR_FCMP_T(OP, TY)                                 \
  assert(Src1.AggregateVal.size() == Src2.AggregateVal.size());         \
  Dest.AggregateVal.resize( Src1.AggregateVal.size() );                 \
  for( uint32_t _i=0;_i<Src1.AggregateVal.size();_i++)                  \
    Dest.AggregateVal[_i].IntVal = llvm::APInt(1,                       \
                                               Src1.AggregateVal[_i].TY##Val OP Src2.AggregateVal[_i].TY##Val); \
  break;

#define IMPLEMENT_VECTOR_FCMP(OP)                                       \
  case llvm::Type::VectorTyID:                                          \
  if(llvm::dyn_cast<llvm::VectorType>(Ty)->getElementType()->isFloatTy()) { \
    IMPLEMENT_VECTOR_FCMP_T(OP, Float);                                 \
  } else {                                                              \
    IMPLEMENT_VECTOR_FCMP_T(OP, Double);                                \
  }

static llvm::GenericValue executeFCMP_OEQ(llvm::GenericValue Src1, llvm::GenericValue Src2,
                                          llvm::Type *Ty) {
  llvm::GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(==, Float);
    IMPLEMENT_FCMP(==, Double);
    IMPLEMENT_VECTOR_FCMP(==);
  default:
    llvm::dbgs() << "Unhandled type for FCmp EQ instruction: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

#define IMPLEMENT_SCALAR_NANS(TY, X,Y)                              \
  if (TY->isFloatTy()) {                                            \
    if (X.FloatVal != X.FloatVal || Y.FloatVal != Y.FloatVal) {     \
      Dest.IntVal = llvm::APInt(1,false);                           \
      return Dest;                                                  \
    }                                                               \
  } else {                                                          \
    if (X.DoubleVal != X.DoubleVal || Y.DoubleVal != Y.DoubleVal) { \
      Dest.IntVal = llvm::APInt(1,false);                           \
      return Dest;                                                  \
    }                                                               \
  }

#define MASK_VECTOR_NANS_T(X,Y, TZ, FLAG)                           \
  assert(X.AggregateVal.size() == Y.AggregateVal.size());           \
  Dest.AggregateVal.resize( X.AggregateVal.size() );                \
  for( uint32_t _i=0;_i<X.AggregateVal.size();_i++) {               \
    if (X.AggregateVal[_i].TZ##Val != X.AggregateVal[_i].TZ##Val || \
        Y.AggregateVal[_i].TZ##Val != Y.AggregateVal[_i].TZ##Val)   \
      Dest.AggregateVal[_i].IntVal = llvm::APInt(1,FLAG);           \
    else  {                                                         \
      Dest.AggregateVal[_i].IntVal = llvm::APInt(1,!FLAG);          \
    }                                                               \
  }

#define MASK_VECTOR_NANS(TY, X,Y, FLAG)                                 \
  if (TY->isVectorTy()) {                                               \
                         if (llvm::dyn_cast<llvm::VectorType>(TY)->getElementType()->isFloatTy()) { \
                                                                                                   MASK_VECTOR_NANS_T(X, Y, Float, FLAG) \
                                                                                                   } else { \
                                                                                                           MASK_VECTOR_NANS_T(X, Y, Double, FLAG) \
                                                                                                           } \
                         }                                              \



static llvm::GenericValue executeFCMP_ONE(llvm::GenericValue Src1, llvm::GenericValue Src2,
                                          llvm::Type *Ty)
{
  llvm::GenericValue Dest;
  // if input is scalar value and Src1 or Src2 is NaN return false
  IMPLEMENT_SCALAR_NANS(Ty, Src1, Src2)
    // if vector input detect NaNs and fill mask
    MASK_VECTOR_NANS(Ty, Src1, Src2, false)
    llvm::GenericValue DestMask = Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(!=, Float);
    IMPLEMENT_FCMP(!=, Double);
    IMPLEMENT_VECTOR_FCMP(!=);
  default:
    llvm::dbgs() << "Unhandled type for FCmp NE instruction: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  // in vector case mask out NaN elements
  if (Ty->isVectorTy())
    for( size_t _i=0; _i<Src1.AggregateVal.size(); _i++)
      if (DestMask.AggregateVal[_i].IntVal == false)
        Dest.AggregateVal[_i].IntVal = llvm::APInt(1,false);

  return Dest;
}

static llvm::GenericValue executeFCMP_OLE(llvm::GenericValue Src1, llvm::GenericValue Src2,
                                          llvm::Type *Ty) {
  llvm::GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(<=, Float);
    IMPLEMENT_FCMP(<=, Double);
    IMPLEMENT_VECTOR_FCMP(<=);
  default:
    llvm::dbgs() << "Unhandled type for FCmp LE instruction: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

static llvm::GenericValue executeFCMP_OGE(llvm::GenericValue Src1, llvm::GenericValue Src2,
                                          llvm::Type *Ty) {
  llvm::GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(>=, Float);
    IMPLEMENT_FCMP(>=, Double);
    IMPLEMENT_VECTOR_FCMP(>=);
  default:
    llvm::dbgs() << "Unhandled type for FCmp GE instruction: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

static llvm::GenericValue executeFCMP_OLT(llvm::GenericValue Src1, llvm::GenericValue Src2,
                                          llvm::Type *Ty) {
  llvm::GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(<, Float);
    IMPLEMENT_FCMP(<, Double);
    IMPLEMENT_VECTOR_FCMP(<);
  default:
    llvm::dbgs() << "Unhandled type for FCmp LT instruction: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

static llvm::GenericValue executeFCMP_OGT(llvm::GenericValue Src1, llvm::GenericValue Src2,
                                          llvm::Type *Ty) {
  llvm::GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(>, Float);
    IMPLEMENT_FCMP(>, Double);
    IMPLEMENT_VECTOR_FCMP(>);
  default:
    llvm::dbgs() << "Unhandled type for FCmp GT instruction: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

#define IMPLEMENT_UNORDERED(TY, X,Y)                                    \
  if (TY->isFloatTy()) {                                                \
    if (X.FloatVal != X.FloatVal || Y.FloatVal != Y.FloatVal) {         \
      Dest.IntVal = llvm::APInt(1,true);                                \
      return Dest;                                                      \
    }                                                                   \
  } else if (X.DoubleVal != X.DoubleVal || Y.DoubleVal != Y.DoubleVal) { \
    Dest.IntVal = llvm::APInt(1,true);                                  \
    return Dest;                                                        \
  }

#define IMPLEMENT_VECTOR_UNORDERED(TY, X,Y, _FUNC)          \
  if (TY->isVectorTy()) {                                   \
    llvm::GenericValue DestMask = Dest;                     \
    Dest = _FUNC(Src1, Src2, Ty);                           \
    for( size_t _i=0; _i<Src1.AggregateVal.size(); _i++)    \
      if (DestMask.AggregateVal[_i].IntVal == true)         \
        Dest.AggregateVal[_i].IntVal = llvm::APInt(1,true); \
    return Dest;                                            \
  }

static llvm::GenericValue executeFCMP_UEQ(llvm::GenericValue Src1, llvm::GenericValue Src2,
                                          llvm::Type *Ty) {
  llvm::GenericValue Dest;
  IMPLEMENT_UNORDERED(Ty, Src1, Src2)
    MASK_VECTOR_NANS(Ty, Src1, Src2, true)
    IMPLEMENT_VECTOR_UNORDERED(Ty, Src1, Src2, executeFCMP_OEQ)
    return executeFCMP_OEQ(Src1, Src2, Ty);

}

static llvm::GenericValue executeFCMP_UNE(llvm::GenericValue Src1, llvm::GenericValue Src2,
                                          llvm::Type *Ty) {
  llvm::GenericValue Dest;
  IMPLEMENT_UNORDERED(Ty, Src1, Src2)
    MASK_VECTOR_NANS(Ty, Src1, Src2, true)
    IMPLEMENT_VECTOR_UNORDERED(Ty, Src1, Src2, executeFCMP_ONE)
    return executeFCMP_ONE(Src1, Src2, Ty);
}

static llvm::GenericValue executeFCMP_ULE(llvm::GenericValue Src1, llvm::GenericValue Src2,
                                          llvm::Type *Ty) {
  llvm::GenericValue Dest;
  IMPLEMENT_UNORDERED(Ty, Src1, Src2)
    MASK_VECTOR_NANS(Ty, Src1, Src2, true)
    IMPLEMENT_VECTOR_UNORDERED(Ty, Src1, Src2, executeFCMP_OLE)
    return executeFCMP_OLE(Src1, Src2, Ty);
}

static llvm::GenericValue executeFCMP_UGE(llvm::GenericValue Src1, llvm::GenericValue Src2,
                                          llvm::Type *Ty) {
  llvm::GenericValue Dest;
  IMPLEMENT_UNORDERED(Ty, Src1, Src2)
    MASK_VECTOR_NANS(Ty, Src1, Src2, true)
    IMPLEMENT_VECTOR_UNORDERED(Ty, Src1, Src2, executeFCMP_OGE)
    return executeFCMP_OGE(Src1, Src2, Ty);
}

static llvm::GenericValue executeFCMP_ULT(llvm::GenericValue Src1, llvm::GenericValue Src2,
                                          llvm::Type *Ty) {
  llvm::GenericValue Dest;
  IMPLEMENT_UNORDERED(Ty, Src1, Src2)
    MASK_VECTOR_NANS(Ty, Src1, Src2, true)
    IMPLEMENT_VECTOR_UNORDERED(Ty, Src1, Src2, executeFCMP_OLT)
    return executeFCMP_OLT(Src1, Src2, Ty);
}

static llvm::GenericValue executeFCMP_UGT(llvm::GenericValue Src1, llvm::GenericValue Src2,
                                          llvm::Type *Ty) {
  llvm::GenericValue Dest;
  IMPLEMENT_UNORDERED(Ty, Src1, Src2)
    MASK_VECTOR_NANS(Ty, Src1, Src2, true)
    IMPLEMENT_VECTOR_UNORDERED(Ty, Src1, Src2, executeFCMP_OGT)
    return executeFCMP_OGT(Src1, Src2, Ty);
}

static llvm::GenericValue executeFCMP_ORD(llvm::GenericValue Src1, llvm::GenericValue Src2,
                                          llvm::Type *Ty) {
  llvm::GenericValue Dest;
  if(Ty->isVectorTy()) {
    assert(Src1.AggregateVal.size() == Src2.AggregateVal.size());
    Dest.AggregateVal.resize( Src1.AggregateVal.size() );
    if(llvm::dyn_cast<llvm::VectorType>(Ty)->getElementType()->isFloatTy()) {
      for( size_t _i=0;_i<Src1.AggregateVal.size();_i++)
        Dest.AggregateVal[_i].IntVal = llvm::APInt(1,
                                                   ( (Src1.AggregateVal[_i].FloatVal ==
                                                      Src1.AggregateVal[_i].FloatVal) &&
                                                     (Src2.AggregateVal[_i].FloatVal ==
                                                      Src2.AggregateVal[_i].FloatVal)));
    } else {
      for( size_t _i=0;_i<Src1.AggregateVal.size();_i++)
        Dest.AggregateVal[_i].IntVal = llvm::APInt(1,
                                                   ( (Src1.AggregateVal[_i].DoubleVal ==
                                                      Src1.AggregateVal[_i].DoubleVal) &&
                                                     (Src2.AggregateVal[_i].DoubleVal ==
                                                      Src2.AggregateVal[_i].DoubleVal)));
    }
  } else if (Ty->isFloatTy())
    Dest.IntVal = llvm::APInt(1,(Src1.FloatVal == Src1.FloatVal &&
                                 Src2.FloatVal == Src2.FloatVal));
  else {
    Dest.IntVal = llvm::APInt(1,(Src1.DoubleVal == Src1.DoubleVal &&
                                 Src2.DoubleVal == Src2.DoubleVal));
  }
  return Dest;
}

static llvm::GenericValue executeFCMP_UNO(llvm::GenericValue Src1, llvm::GenericValue Src2,
                                          llvm::Type *Ty) {
  llvm::GenericValue Dest;
  if(Ty->isVectorTy()) {
    assert(Src1.AggregateVal.size() == Src2.AggregateVal.size());
    Dest.AggregateVal.resize( Src1.AggregateVal.size() );
    if(llvm::dyn_cast<llvm::VectorType>(Ty)->getElementType()->isFloatTy()) {
      for( size_t _i=0;_i<Src1.AggregateVal.size();_i++)
        Dest.AggregateVal[_i].IntVal = llvm::APInt(1,
                                                   ( (Src1.AggregateVal[_i].FloatVal !=
                                                      Src1.AggregateVal[_i].FloatVal) ||
                                                     (Src2.AggregateVal[_i].FloatVal !=
                                                      Src2.AggregateVal[_i].FloatVal)));
    } else {
      for( size_t _i=0;_i<Src1.AggregateVal.size();_i++)
        Dest.AggregateVal[_i].IntVal = llvm::APInt(1,
                                                   ( (Src1.AggregateVal[_i].DoubleVal !=
                                                      Src1.AggregateVal[_i].DoubleVal) ||
                                                     (Src2.AggregateVal[_i].DoubleVal !=
                                                      Src2.AggregateVal[_i].DoubleVal)));
    }
  } else if (Ty->isFloatTy())
    Dest.IntVal = llvm::APInt(1,(Src1.FloatVal != Src1.FloatVal ||
                                 Src2.FloatVal != Src2.FloatVal));
  else {
    Dest.IntVal = llvm::APInt(1,(Src1.DoubleVal != Src1.DoubleVal ||
                                 Src2.DoubleVal != Src2.DoubleVal));
  }
  return Dest;
}

static llvm::GenericValue executeFCMP_BOOL(llvm::GenericValue Src1, llvm::GenericValue Src2,
                                           const llvm::Type *Ty, const bool val) {
  llvm::GenericValue Dest;
  if(Ty->isVectorTy()) {
    assert(Src1.AggregateVal.size() == Src2.AggregateVal.size());
    Dest.AggregateVal.resize( Src1.AggregateVal.size() );
    for( size_t _i=0; _i<Src1.AggregateVal.size(); _i++)
      Dest.AggregateVal[_i].IntVal = llvm::APInt(1,val);
  } else {
    Dest.IntVal = llvm::APInt(1, val);
  }

  return Dest;
}

void POWERInterpreter::visitFCmpInst(llvm::FCmpInst &I) {
  llvm::Type *Ty    = I.getOperand(0)->getType();
  llvm::GenericValue Src1 = getOperandValue(0);
  llvm::GenericValue Src2 = getOperandValue(1);
  llvm::GenericValue R;   // Result

  switch (I.getPredicate()) {
  default:
    llvm::dbgs() << "Don't know how to handle this FCmp predicate!\n-->" << I;
    llvm_unreachable(nullptr);
    break;
  case llvm::FCmpInst::FCMP_FALSE: R = executeFCMP_BOOL(Src1, Src2, Ty, false);
    break;
  case llvm::FCmpInst::FCMP_TRUE:  R = executeFCMP_BOOL(Src1, Src2, Ty, true);
    break;
  case llvm::FCmpInst::FCMP_ORD:   R = executeFCMP_ORD(Src1, Src2, Ty); break;
  case llvm::FCmpInst::FCMP_UNO:   R = executeFCMP_UNO(Src1, Src2, Ty); break;
  case llvm::FCmpInst::FCMP_UEQ:   R = executeFCMP_UEQ(Src1, Src2, Ty); break;
  case llvm::FCmpInst::FCMP_OEQ:   R = executeFCMP_OEQ(Src1, Src2, Ty); break;
  case llvm::FCmpInst::FCMP_UNE:   R = executeFCMP_UNE(Src1, Src2, Ty); break;
  case llvm::FCmpInst::FCMP_ONE:   R = executeFCMP_ONE(Src1, Src2, Ty); break;
  case llvm::FCmpInst::FCMP_ULT:   R = executeFCMP_ULT(Src1, Src2, Ty); break;
  case llvm::FCmpInst::FCMP_OLT:   R = executeFCMP_OLT(Src1, Src2, Ty); break;
  case llvm::FCmpInst::FCMP_UGT:   R = executeFCMP_UGT(Src1, Src2, Ty); break;
  case llvm::FCmpInst::FCMP_OGT:   R = executeFCMP_OGT(Src1, Src2, Ty); break;
  case llvm::FCmpInst::FCMP_ULE:   R = executeFCMP_ULE(Src1, Src2, Ty); break;
  case llvm::FCmpInst::FCMP_OLE:   R = executeFCMP_OLE(Src1, Src2, Ty); break;
  case llvm::FCmpInst::FCMP_UGE:   R = executeFCMP_UGE(Src1, Src2, Ty); break;
  case llvm::FCmpInst::FCMP_OGE:   R = executeFCMP_OGE(Src1, Src2, Ty); break;
  }

  setCurInstrValue(R);
}

static llvm::GenericValue executeCmpInst(unsigned predicate, llvm::GenericValue Src1,
                                         llvm::GenericValue Src2, llvm::Type *Ty) {
  llvm::GenericValue Result;
  switch (predicate) {
  case llvm::ICmpInst::ICMP_EQ:    return executeICMP_EQ(Src1, Src2, Ty);
  case llvm::ICmpInst::ICMP_NE:    return executeICMP_NE(Src1, Src2, Ty);
  case llvm::ICmpInst::ICMP_UGT:   return executeICMP_UGT(Src1, Src2, Ty);
  case llvm::ICmpInst::ICMP_SGT:   return executeICMP_SGT(Src1, Src2, Ty);
  case llvm::ICmpInst::ICMP_ULT:   return executeICMP_ULT(Src1, Src2, Ty);
  case llvm::ICmpInst::ICMP_SLT:   return executeICMP_SLT(Src1, Src2, Ty);
  case llvm::ICmpInst::ICMP_UGE:   return executeICMP_UGE(Src1, Src2, Ty);
  case llvm::ICmpInst::ICMP_SGE:   return executeICMP_SGE(Src1, Src2, Ty);
  case llvm::ICmpInst::ICMP_ULE:   return executeICMP_ULE(Src1, Src2, Ty);
  case llvm::ICmpInst::ICMP_SLE:   return executeICMP_SLE(Src1, Src2, Ty);
  case llvm::FCmpInst::FCMP_ORD:   return executeFCMP_ORD(Src1, Src2, Ty);
  case llvm::FCmpInst::FCMP_UNO:   return executeFCMP_UNO(Src1, Src2, Ty);
  case llvm::FCmpInst::FCMP_OEQ:   return executeFCMP_OEQ(Src1, Src2, Ty);
  case llvm::FCmpInst::FCMP_UEQ:   return executeFCMP_UEQ(Src1, Src2, Ty);
  case llvm::FCmpInst::FCMP_ONE:   return executeFCMP_ONE(Src1, Src2, Ty);
  case llvm::FCmpInst::FCMP_UNE:   return executeFCMP_UNE(Src1, Src2, Ty);
  case llvm::FCmpInst::FCMP_OLT:   return executeFCMP_OLT(Src1, Src2, Ty);
  case llvm::FCmpInst::FCMP_ULT:   return executeFCMP_ULT(Src1, Src2, Ty);
  case llvm::FCmpInst::FCMP_OGT:   return executeFCMP_OGT(Src1, Src2, Ty);
  case llvm::FCmpInst::FCMP_UGT:   return executeFCMP_UGT(Src1, Src2, Ty);
  case llvm::FCmpInst::FCMP_OLE:   return executeFCMP_OLE(Src1, Src2, Ty);
  case llvm::FCmpInst::FCMP_ULE:   return executeFCMP_ULE(Src1, Src2, Ty);
  case llvm::FCmpInst::FCMP_OGE:   return executeFCMP_OGE(Src1, Src2, Ty);
  case llvm::FCmpInst::FCMP_UGE:   return executeFCMP_UGE(Src1, Src2, Ty);
  case llvm::FCmpInst::FCMP_FALSE: return executeFCMP_BOOL(Src1, Src2, Ty, false);
  case llvm::FCmpInst::FCMP_TRUE:  return executeFCMP_BOOL(Src1, Src2, Ty, true);
  default:
    llvm::dbgs() << "Unhandled Cmp predicate\n";
    llvm_unreachable(nullptr);
  }
}

void POWERInterpreter::visitBinaryOperator(llvm::BinaryOperator &I) {
  llvm::Type *Ty    = I.getOperand(0)->getType();
  llvm::GenericValue Src1 = getOperandValue(0);
  llvm::GenericValue Src2 = getOperandValue(1);
  llvm::GenericValue R;   // Result

  // First process vector operation
  if (Ty->isVectorTy()) {
    assert(Src1.AggregateVal.size() == Src2.AggregateVal.size());
    R.AggregateVal.resize(Src1.AggregateVal.size());

    // Macros to execute binary operation 'OP' over integer vectors
#define INTEGER_VECTOR_OPERATION(OP)                                \
    for (unsigned i = 0; i < R.AggregateVal.size(); ++i)            \
      R.AggregateVal[i].IntVal =                                    \
        Src1.AggregateVal[i].IntVal OP Src2.AggregateVal[i].IntVal;

    // Additional macros to execute binary operations udiv/sdiv/urem/srem since
    // they have different notation.
#define INTEGER_VECTOR_FUNCTION(OP)                                   \
    for (unsigned i = 0; i < R.AggregateVal.size(); ++i)              \
      R.AggregateVal[i].IntVal =                                      \
        Src1.AggregateVal[i].IntVal.OP(Src2.AggregateVal[i].IntVal);

    // Macros to execute binary operation 'OP' over floating point type TY
    // (float or double) vectors
#define FLOAT_VECTOR_FUNCTION(OP, TY)                       \
    for (unsigned i = 0; i < R.AggregateVal.size(); ++i)    \
      R.AggregateVal[i].TY =                                \
        Src1.AggregateVal[i].TY OP Src2.AggregateVal[i].TY;

    // Macros to choose appropriate TY: float or double and run operation
    // execution
#define FLOAT_VECTOR_OP(OP) {                                           \
      if (llvm::dyn_cast<llvm::VectorType>(Ty)->getElementType()->isFloatTy()) \
        FLOAT_VECTOR_FUNCTION(OP, FloatVal)                             \
        else {                                                          \
          if (llvm::dyn_cast<llvm::VectorType>(Ty)->getElementType()->isDoubleTy()) \
            FLOAT_VECTOR_FUNCTION(OP, DoubleVal)                        \
            else {                                                      \
              llvm::dbgs() << "Unhandled type for OP instruction: " << *Ty << "\n"; \
              llvm_unreachable(0);                                      \
            }                                                           \
        }                                                               \
    }

    switch(I.getOpcode()){
    default:
      llvm::dbgs() << "Don't know how to handle this binary operator!\n-->" << I;
      llvm_unreachable(nullptr);
      break;
    case llvm::Instruction::Add:   INTEGER_VECTOR_OPERATION(+) break;
    case llvm::Instruction::Sub:   INTEGER_VECTOR_OPERATION(-) break;
    case llvm::Instruction::Mul:   INTEGER_VECTOR_OPERATION(*) break;
    case llvm::Instruction::UDiv:  INTEGER_VECTOR_FUNCTION(udiv) break;
    case llvm::Instruction::SDiv:  INTEGER_VECTOR_FUNCTION(sdiv) break;
    case llvm::Instruction::URem:  INTEGER_VECTOR_FUNCTION(urem) break;
    case llvm::Instruction::SRem:  INTEGER_VECTOR_FUNCTION(srem) break;
    case llvm::Instruction::And:   INTEGER_VECTOR_OPERATION(&) break;
    case llvm::Instruction::Or:    INTEGER_VECTOR_OPERATION(|) break;
    case llvm::Instruction::Xor:   INTEGER_VECTOR_OPERATION(^) break;
    case llvm::Instruction::FAdd:  FLOAT_VECTOR_OP(+) break;
    case llvm::Instruction::FSub:  FLOAT_VECTOR_OP(-) break;
    case llvm::Instruction::FMul:  FLOAT_VECTOR_OP(*) break;
    case llvm::Instruction::FDiv:  FLOAT_VECTOR_OP(/) break;
    case llvm::Instruction::FRem:
      if (llvm::dyn_cast<llvm::VectorType>(Ty)->getElementType()->isFloatTy())
        for (unsigned i = 0; i < R.AggregateVal.size(); ++i)
          R.AggregateVal[i].FloatVal =
            fmod(Src1.AggregateVal[i].FloatVal, Src2.AggregateVal[i].FloatVal);
      else {
        if (llvm::dyn_cast<llvm::VectorType>(Ty)->getElementType()->isDoubleTy())
          for (unsigned i = 0; i < R.AggregateVal.size(); ++i)
            R.AggregateVal[i].DoubleVal =
              fmod(Src1.AggregateVal[i].DoubleVal, Src2.AggregateVal[i].DoubleVal);
        else {
          llvm::dbgs() << "Unhandled type for Rem instruction: " << *Ty << "\n";
          llvm_unreachable(nullptr);
        }
      }
      break;
    }
  } else {
    switch (I.getOpcode()) {
    default:
      llvm::dbgs() << "Don't know how to handle this binary operator!\n-->" << I;
      llvm_unreachable(nullptr);
      break;
    case llvm::Instruction::Add:   R.IntVal = Src1.IntVal + Src2.IntVal; break;
    case llvm::Instruction::Sub:   R.IntVal = Src1.IntVal - Src2.IntVal; break;
    case llvm::Instruction::Mul:   R.IntVal = Src1.IntVal * Src2.IntVal; break;
    case llvm::Instruction::FAdd:  executeFAddInst(R, Src1, Src2, Ty); break;
    case llvm::Instruction::FSub:  executeFSubInst(R, Src1, Src2, Ty); break;
    case llvm::Instruction::FMul:  executeFMulInst(R, Src1, Src2, Ty); break;
    case llvm::Instruction::FDiv:  executeFDivInst(R, Src1, Src2, Ty); break;
    case llvm::Instruction::FRem:  executeFRemInst(R, Src1, Src2, Ty); break;
    case llvm::Instruction::UDiv:  R.IntVal = Src1.IntVal.udiv(Src2.IntVal); break;
    case llvm::Instruction::SDiv:  R.IntVal = Src1.IntVal.sdiv(Src2.IntVal); break;
    case llvm::Instruction::URem:  R.IntVal = Src1.IntVal.urem(Src2.IntVal); break;
    case llvm::Instruction::SRem:  R.IntVal = Src1.IntVal.srem(Src2.IntVal); break;
    case llvm::Instruction::And:   R.IntVal = Src1.IntVal & Src2.IntVal; break;
    case llvm::Instruction::Or:    R.IntVal = Src1.IntVal | Src2.IntVal; break;
    case llvm::Instruction::Xor:   R.IntVal = Src1.IntVal ^ Src2.IntVal; break;
    }
  }
  setCurInstrValue(R);
}

static llvm::GenericValue executeSelectInst(llvm::GenericValue Src1, llvm::GenericValue Src2,
                                            llvm::GenericValue Src3, const llvm::Type *Ty) {
  llvm::GenericValue Dest;
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

void POWERInterpreter::visitSelectInst(llvm::SelectInst &I) {
  const llvm::Type * Ty = I.getOperand(0)->getType();
  llvm::GenericValue Src1 = getOperandValue(0);
  llvm::GenericValue Src2 = getOperandValue(1);
  llvm::GenericValue Src3 = getOperandValue(2);
  llvm::GenericValue R = executeSelectInst(Src1, Src2, Src3, Ty);
  setCurInstrValue(R);
}

//===----------------------------------------------------------------------===//
//                     Terminator llvm::Instruction Implementations
//===----------------------------------------------------------------------===//

void POWERInterpreter::exitCalled(llvm::GenericValue GV) {
  // runAtExitHandlers() assumes there are no stack frames, but
  // if exit() was called, then it had a stack frame. Blow away
  // the stack before interpreting atexit handlers.
  Threads[CurrentThread].ECStack.clear();
  runAtExitHandlers();
  exit(GV.IntVal.zextOrTrunc(32).getZExtValue());
}

/// Pop the last stack frame off of ECStack and then copy the result
/// back into the result variable if we are not returning void. The
/// result variable may be the ExitValue, or the llvm::Value of the calling
/// CallInst if there was a previous stack frame. This method may
/// invalidate any ECStack iterators you have. This method also takes
/// care of switching to the normal destination BB, if we are returning
/// from an invoke.
///
void POWERInterpreter::popStackAndReturnValueToCaller(llvm::Type *RetTy,
                                                      llvm::GenericValue Result) {
  // Pop the current stack frame.
  Threads[CurrentThread].ECStack.pop_back();

  if (Threads[CurrentThread].ECStack.empty()) {  // Finished main.  Put result into exit code...
    fetchThreadExit();
    if (RetTy && !RetTy->isVoidTy()) {          // Nonvoid return type?
      ExitValue = Result;   // Capture the exit value of the program
    } else {
      memset(&ExitValue.Untyped, 0, sizeof(ExitValue.Untyped));
    }
  } else {
    // If we have a previous stack frame, and we have a previous call,
    // fill in the return value...
    ExecutionContext &CallingSF = Threads[CurrentThread].ECStack.back();
    if(CallingSF.Caller){
      // Save result...
      if (!CallingSF.Caller->I.getType()->isVoidTy()){
        setCurInstrValue(Result);
      }
      if (llvm::InvokeInst *II = llvm::dyn_cast<llvm::InvokeInst> (&CallingSF.Caller->I)){
        SwitchToNewBasicBlock (II->getNormalDest (), CallingSF);
      }
      CallingSF.Values[&CallingSF.Caller->I] = CurInstr; // Redirect dependents of the call to the return instruction
      CallingSF.Caller = nullptr;          // We returned from the call...
    }
  }
}

void POWERInterpreter::fetchThreadExit(){
  TB.fetch_fence(CurrentThread,POWERARMTraceBuilder::LWSYNC);
  IID<int> iid = TB.fetch(CurrentThread,0,1,{},{});
  TB.register_addr(iid,0,MRef(Threads[CurrentThread].status,1));
  MBlock data2(MRef(0,1),1);
  *((uint8_t*)data2.get_block()) = 2;
  TB.register_data(iid,0,data2);
  std::shared_ptr<FetchedInstruction> st(new FetchedInstruction(*dummy_store));
  st->EventIndex = iid.get_index();
  Threads[CurrentThread].CommittableEvents.insert(st);
}

void POWERInterpreter::visitReturnInst(llvm::ReturnInst &I) {
  llvm::Type *RetTy = llvm::Type::getVoidTy(I.getContext());
  llvm::GenericValue Result;

  if(conf.ee_store_trace){
    TB.trace_register_function_exit(CurrentThread);
  }

  // Save away the return value... (if we are not 'ret void')
  if (I.getNumOperands()) {
    RetTy  = I.getReturnValue()->getType();
    assert(I.getReturnValue() == I.getOperand(0));
    Result = getOperandValue(0);
  }

  popStackAndReturnValueToCaller(RetTy, Result);
}

void POWERInterpreter::visitUnreachableInst(llvm::UnreachableInst &I) {
  llvm::report_fatal_error("Program executed an 'unreachable' instruction!");
}

void POWERInterpreter::visitBranchInst(llvm::BranchInst &I) {
  ExecutionContext &SF = Threads[CurrentThread].ECStack.back();
  llvm::BasicBlock *Dest;

  Dest = I.getSuccessor(0);          // Uncond branches have a fixed dest...
  if (!I.isUnconditional()) {
    assert(I.getCondition() == I.getOperand(0));
    if (getOperandValue(0).IntVal == 0){ // If false cond...
      Dest = I.getSuccessor(1);
    }
  }
  SwitchToNewBasicBlock(Dest, SF);
}

void POWERInterpreter::visitSwitchInst(llvm::SwitchInst &I) {
  ExecutionContext &SF = Threads[CurrentThread].ECStack.back();
  llvm::Value* Cond = I.getCondition();
  llvm::Type *ElTy = Cond->getType();
  assert(I.getCondition() == I.getOperand(0));
  llvm::GenericValue CondVal = getOperandValue(0);

  // Check to see if any of the cases match...
  llvm::BasicBlock *Dest = nullptr;
  const unsigned nc = I.getNumCases();
  llvm::SwitchInst::CaseIt cit = I.case_begin();
  for(unsigned i = 0; i < nc; ++i, ++cit){
    llvm::GenericValue CaseVal = getOperandValue(2*i+2);
    if(CaseVal.AggregateVal.size()){
      assert(CaseVal.AggregateVal.size() == 1);
      assert(CaseVal.AggregateVal[0].AggregateVal.size() == 2);
      CaseVal = CaseVal.AggregateVal[0].AggregateVal[0];
    }else{
      assert(I.getOperand(2*i+2) == cit.getCaseValue());
    }
    if(executeICMP_EQ(CondVal, CaseVal, ElTy).IntVal != 0){
      Dest = llvm::cast<llvm::BasicBlock>(cit.getCaseSuccessor());
      break;
    }
  }

  if (!Dest) Dest = I.getDefaultDest();   // No cases matched: use default
  SwitchToNewBasicBlock(Dest, SF);
}

void POWERInterpreter::visitIndirectBrInst(llvm::IndirectBrInst &I) {
  ExecutionContext &SF = Threads[CurrentThread].ECStack.back();
  assert(I.getAddress() == I.getOperand(0));
  void *Dest = GVTOP(getOperandValue(0));
  SwitchToNewBasicBlock((llvm::BasicBlock*)Dest, SF);
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
void POWERInterpreter::SwitchToNewBasicBlock(llvm::BasicBlock *Dest, ExecutionContext &SF){
  SF.PrevBB  = SF.CurBB;              // Remember where we came from...
  SF.CurBB   = Dest;                  // Update CurBB to branch destination
  SF.CurInst = SF.CurBB->begin();     // Update new instruction ptr...
}

//===----------------------------------------------------------------------===//
//                     Memory llvm::Instruction Implementations
//===----------------------------------------------------------------------===//

void POWERInterpreter::visitAllocaInst(llvm::AllocaInst &I) {
  llvm::Type *Ty = I.getType()->getElementType();  // Type to be allocated

  // Get the number of elements being allocated by the array...
  unsigned NumElements =
    getOperandValue(0).IntVal.getZExtValue();

  unsigned TypeSize = (size_t)TD.getTypeAllocSize(Ty);

  // Avoid malloc-ing zero bytes, use max()...
  unsigned MemToAlloc = std::max(1U, NumElements * TypeSize);

  // Allocate enough memory to hold the type...
  void *Memory = malloc(MemToAlloc);

  llvm::GenericValue Result = llvm::PTOGV(Memory);
  assert(Result.PointerVal && "Null pointer returned by malloc!");
  setCurInstrValue(Result);

  if (I.getOpcode() == llvm::Instruction::Alloca){
    Threads[CurrentThread].Allocas.add(Memory);
  }
}

// getElementOffset - The workhorse for getelementptr.
//
llvm::GenericValue POWERInterpreter::executeGEPOperation(llvm::Value *PtrVal,
                                                         const std::vector<FetchedInstruction::Operand> &Operands,
                                                         gep_type_iterator I,
                                                         gep_type_iterator E) {
  assert(Operands.size());
  assert(Operands[0].Available);
  const llvm::GenericValue &Ptr = Operands[0].Value;
  assert(PtrVal->getType()->isPointerTy() &&
         "Cannot getElementOffset of a nonpointer type!");

  uint64_t Total = 0;

  for (unsigned i = 1; I != E; ++I, ++i) {
    if (llvm::StructType *STy = llvm::dyn_cast<llvm::StructType>(*I)) {
      const llvm::StructLayout *SLO = TD.getStructLayout(STy);

      const llvm::ConstantInt *CPU = llvm::cast<llvm::ConstantInt>(I.getOperand());
      unsigned Index = unsigned(CPU->getZExtValue());

      Total += SLO->getElementOffset(Index);
    } else {
      llvm::SequentialType *ST = llvm::cast<llvm::SequentialType>(*I);
      // Get the index number for the array... which must be long type...
      assert(i < Operands.size());
      assert(Operands[i].Available);
      llvm::GenericValue IdxGV = Operands[i].Value;

      int64_t Idx;
      unsigned BitWidth =
        llvm::cast<llvm::IntegerType>(I.getOperand()->getType())->getBitWidth();
      if (BitWidth == 32)
        Idx = (int64_t)(int32_t)IdxGV.IntVal.getZExtValue();
      else {
        assert(BitWidth == 64 && "Invalid index type for getelementptr");
        Idx = (int64_t)IdxGV.IntVal.getZExtValue();
      }
      Total += TD.getTypeAllocSize(ST->getElementType())*Idx;
    }
  }

  llvm::GenericValue Result;
  Result.PointerVal = ((char*)Ptr.PointerVal) + Total;
  return Result;
}

void POWERInterpreter::visitGetElementPtrInst(llvm::GetElementPtrInst &I) {
  assert(I.getPointerOperand() == I.getOperand(0));
  setCurInstrValue(executeGEPOperation(I.getPointerOperand(),
                                       CurInstr->Operands,
                                       gep_type_begin(I), gep_type_end(I)));
}

void POWERInterpreter::visitLoadInst(llvm::LoadInst &I) {
  // Value is assigned at the time of scheduling
}

void POWERInterpreter::visitStoreInst(llvm::StoreInst &I) {
  // Stored value is registered at the time of scheduling
}

//===----------------------------------------------------------------------===//
//                 Miscellaneous llvm::Instruction Implementations
//===----------------------------------------------------------------------===//

/* Strip away whitespace from the beginning and end of s. */
static void stripws(std::string &s){
  int first = 0, len;
  while(first < int(s.size()) && std::isspace(s[first])) ++first;
  len = s.size() - first;
  while(0 < len && std::isspace(s[first+len-1])) --len;
  s = s.substr(first,len);
}

bool POWERInterpreter::isInlineAsm(llvm::CallSite &CS, std::string *asmstr){
  if(CS.isCall()){
    llvm::CallInst *CI = llvm::cast<llvm::CallInst>(CS.getInstruction());
    if(CI){
      if(CI->isInlineAsm()){
        llvm::InlineAsm *IA = llvm::dyn_cast<llvm::InlineAsm>(CI->getCalledValue());
        assert(IA);
        *asmstr = IA->getAsmString();
        stripws(*asmstr);
        return true;
      }
    }
  }
  return false;
}

bool POWERInterpreter::isInlineAsm(llvm::Instruction &I, std::string *asmstr){
  llvm::CallInst *C = llvm::dyn_cast<llvm::CallInst>(&I);
  if(!C) return false;
  llvm::CallSite CS(C);
  return isInlineAsm(CS,asmstr);
}

void POWERInterpreter::visitInlineAsm(llvm::CallSite &CS, const std::string &asmstr){
  if(asmstr == "DMB" || asmstr == "dmb" ||
     asmstr == "DSB" || asmstr == "dsb" ||
     asmstr == "ISB" || asmstr == "isb" ||
     asmstr == "isync" ||
     asmstr == "eieio" ||
     asmstr == "lwsync" ||
     asmstr == "sync" ||
     asmstr == ""){ // Do nothing
  }else{
    throw std::logic_error("Unsupported inline assembly: " + asmstr);
  }
}

void POWERInterpreter::visitCallSite(llvm::CallSite CS) {
  {
    std::string asmstr;
    if(isInlineAsm(CS,&asmstr)){
      visitInlineAsm(CS,asmstr);
      return;
    }
  }

  ExecutionContext &SF = Threads[CurrentThread].ECStack.back();

  // Check to see if this is an intrinsic function call...
  llvm::Function *F = CS.getCalledFunction();
  if (F && F->isDeclaration()){
    switch (F->getIntrinsicID()) {
    case llvm::Intrinsic::not_intrinsic:
      break;
    case llvm::Intrinsic::vastart: { // va_start
      throw std::logic_error("POWERInterpreter: Varargs are not supported.");
      /*
        llvm::GenericValue ArgIndex;
        ArgIndex.UIntPairVal.first = ECStack.size() - 1;
        ArgIndex.UIntPairVal.second = 0;
        SetValue(CS.getInstruction(), ArgIndex, SF);
      */
      return;
    }
    case llvm::Intrinsic::vaend:    // va_end is a noop for the interpreter
      throw std::logic_error("POWERInterpreter: Varargs are not supported.");
      /*
        return;
      */
    case llvm::Intrinsic::vacopy:   // va_copy: dest = src
      throw std::logic_error("POWERInterpreter: Varargs are not supported.");
      /*
        SetValue(CS.getInstruction(), getOperandValue(*CS.arg_begin(), SF), SF);
      */
      return;
    default:
      {
        if(F->getName().str() == "llvm.dbg.value"){
          /* Ignore this intrinsic function */
          return;
        }
        // If it is an unknown intrinsic function, use the intrinsic lowering
        // class to transform it into hopefully tasty LLVM code.
        //
        llvm::BasicBlock::iterator me(CS.getInstruction());
        llvm::BasicBlock *Parent = CS.getInstruction()->getParent();
        bool atBegin(Parent->begin() == me);
        if (!atBegin)
          --me;
        IL->LowerIntrinsicCall(llvm::cast<llvm::CallInst>(CS.getInstruction()));

        // Restore the CurInst pointer to the first instruction newly inserted, if
        // any.
        if (atBegin) {
          SF.CurInst = Parent->begin();
        } else {
          SF.CurInst = me;
          ++SF.CurInst;
        }
        return;
      }
    }
  }

  assert(CS.getInstruction());
  assert(CS.getInstruction() == &CurInstr->I);

  std::vector<llvm::Value*> ArgVals;
  const unsigned NumArgs = CS.arg_size();
  ArgVals.reserve(NumArgs);
  for(unsigned i = 0; i < NumArgs; ++i){
    assert(CS.getArgument(i) == CS.getInstruction()->getOperand(i));
    ArgVals.push_back(CS.getInstruction()->getOperand(i));
  }

  // To handle indirect calls, we must get the pointer value from the argument
  // and treat it as a function pointer.
  llvm::Function *CF = getCallee();

  if(!CF->isDeclaration()){
    assert(Threads[CurrentThread].ECStack.size());
    SF.Caller = CurInstr;
  }

  callFunction(CF,ArgVals);
}

int POWERInterpreter::getCalleeOpIdx(const llvm::Instruction &I){
  if(I.getOpcode() == llvm::Instruction::Call){
    return I.getNumOperands()-1;
  }else{
    assert(I.getOpcode() == llvm::Instruction::Invoke);
    return I.getNumOperands()-3;
  }
}

llvm::Function *POWERInterpreter::getCallee(){
#ifndef NDEBUG
  if(llvm::CallInst *CI = llvm::dyn_cast<llvm::CallInst>(&CurInstr->I)){
    assert(CI->getCalledValue() == CI->getOperand(CI->getNumOperands()-1));
  }else{
    assert(llvm::dyn_cast<llvm::InvokeInst>(&CurInstr->I));
    llvm::InvokeInst &II = llvm::cast<llvm::InvokeInst>(CurInstr->I);
    assert(II.getCalledValue() == II.getOperand(II.getNumOperands()-3));
  }
#endif
  return (llvm::Function*)GVTOP(getOperandValue(getCalleeOpIdx(CurInstr->I)));
}

// auxiliary function for shift operations
static unsigned getShiftAmount(uint64_t orgShiftAmount,
                               llvm::APInt valueToShift) {
  unsigned valueWidth = valueToShift.getBitWidth();
  if (orgShiftAmount < (uint64_t)valueWidth)
    return orgShiftAmount;
  // according to the llvm documentation, if orgShiftAmount > valueWidth,
  // the result is undfeined. but we do shift by this rule:
  return (llvm::NextPowerOf2(valueWidth-1) - 1) & orgShiftAmount;
}


void POWERInterpreter::visitShl(llvm::BinaryOperator &I) {
  llvm::GenericValue Src1 = getOperandValue(0);
  llvm::GenericValue Src2 = getOperandValue(1);
  llvm::GenericValue Dest;
  const llvm::Type *Ty = I.getType();

  if (Ty->isVectorTy()) {
    uint32_t src1Size = uint32_t(Src1.AggregateVal.size());
    assert(src1Size == Src2.AggregateVal.size());
    for (unsigned i = 0; i < src1Size; i++) {
      llvm::GenericValue Result;
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

  setCurInstrValue(Dest);
}

void POWERInterpreter::visitLShr(llvm::BinaryOperator &I) {
  llvm::GenericValue Src1 = getOperandValue(0);
  llvm::GenericValue Src2 = getOperandValue(1);
  llvm::GenericValue Dest;
  const llvm::Type *Ty = I.getType();

  if (Ty->isVectorTy()) {
    uint32_t src1Size = uint32_t(Src1.AggregateVal.size());
    assert(src1Size == Src2.AggregateVal.size());
    for (unsigned i = 0; i < src1Size; i++) {
      llvm::GenericValue Result;
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

  setCurInstrValue(Dest);
}

void POWERInterpreter::visitAShr(llvm::BinaryOperator &I) {
  llvm::GenericValue Src1 = getOperandValue(0);
  llvm::GenericValue Src2 = getOperandValue(1);
  llvm::GenericValue Dest;
  const llvm::Type *Ty = I.getType();

  if (Ty->isVectorTy()) {
    size_t src1Size = Src1.AggregateVal.size();
    assert(src1Size == Src2.AggregateVal.size());
    for (unsigned i = 0; i < src1Size; i++) {
      llvm::GenericValue Result;
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

  setCurInstrValue(Dest);
}

llvm::GenericValue POWERInterpreter::executeTruncInst(llvm::Value *SrcVal, const llvm::GenericValue &Src, llvm::Type *DstTy) {
  llvm::GenericValue Dest;
  llvm::Type *SrcTy = SrcVal->getType();
  if (SrcTy->isVectorTy()) {
    llvm::Type *DstVecTy = DstTy->getScalarType();
    unsigned DBitWidth = llvm::cast<llvm::IntegerType>(DstVecTy)->getBitWidth();
    unsigned NumElts = Src.AggregateVal.size();
    // the sizes of src and dst vectors must be equal
    Dest.AggregateVal.resize(NumElts);
    for (unsigned i = 0; i < NumElts; i++)
      Dest.AggregateVal[i].IntVal = Src.AggregateVal[i].IntVal.trunc(DBitWidth);
  } else {
    llvm::IntegerType *DITy = llvm::cast<llvm::IntegerType>(DstTy);
    unsigned DBitWidth = DITy->getBitWidth();
    Dest.IntVal = Src.IntVal.trunc(DBitWidth);
  }
  return Dest;
}

llvm::GenericValue POWERInterpreter::executeSExtInst(llvm::Value *SrcVal,
                                                     const llvm::GenericValue &Src,
                                                     llvm::Type *DstTy) {
  const llvm::Type *SrcTy = SrcVal->getType();
  llvm::GenericValue Dest;
  if (SrcTy->isVectorTy()) {
    const llvm::Type *DstVecTy = DstTy->getScalarType();
    unsigned DBitWidth = llvm::cast<llvm::IntegerType>(DstVecTy)->getBitWidth();
    unsigned size = Src.AggregateVal.size();
    // the sizes of src and dst vectors must be equal.
    Dest.AggregateVal.resize(size);
    for (unsigned i = 0; i < size; i++)
      Dest.AggregateVal[i].IntVal = Src.AggregateVal[i].IntVal.sext(DBitWidth);
  } else {
    const llvm::IntegerType *DITy = llvm::cast<llvm::IntegerType>(DstTy);
    unsigned DBitWidth = DITy->getBitWidth();
    Dest.IntVal = Src.IntVal.sext(DBitWidth);
  }
  return Dest;
}

llvm::GenericValue POWERInterpreter::executeZExtInst(llvm::Value *SrcVal,
                                                     const llvm::GenericValue &Src,
                                                     llvm::Type *DstTy) {
  const llvm::Type *SrcTy = SrcVal->getType();
  llvm::GenericValue Dest;
  if (SrcTy->isVectorTy()) {
    const llvm::Type *DstVecTy = DstTy->getScalarType();
    unsigned DBitWidth = llvm::cast<llvm::IntegerType>(DstVecTy)->getBitWidth();

    unsigned size = Src.AggregateVal.size();
    // the sizes of src and dst vectors must be equal.
    Dest.AggregateVal.resize(size);
    for (unsigned i = 0; i < size; i++)
      Dest.AggregateVal[i].IntVal = Src.AggregateVal[i].IntVal.zext(DBitWidth);
  } else {
    const llvm::IntegerType *DITy = llvm::cast<llvm::IntegerType>(DstTy);
    unsigned DBitWidth = DITy->getBitWidth();
    Dest.IntVal = Src.IntVal.zext(DBitWidth);
  }
  return Dest;
}

llvm::GenericValue POWERInterpreter::executeFPTruncInst(llvm::Value *SrcVal,
                                                        const llvm::GenericValue &Src,
                                                        llvm::Type *DstTy) {
  llvm::GenericValue Dest;

  if (SrcVal->getType()->getTypeID() == llvm::Type::VectorTyID) {
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

llvm::GenericValue POWERInterpreter::executeFPExtInst(llvm::Value *SrcVal,
                                                      const llvm::GenericValue &Src,
                                                      llvm::Type *DstTy) {
  llvm::GenericValue Dest;

  if (SrcVal->getType()->getTypeID() == llvm::Type::VectorTyID) {
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

llvm::GenericValue POWERInterpreter::executeFPToUIInst(llvm::Value *SrcVal,
                                                       const llvm::GenericValue &Src,
                                                       llvm::Type *DstTy) {
  llvm::Type *SrcTy = SrcVal->getType();
  llvm::GenericValue Dest;

  if (SrcTy->getTypeID() == llvm::Type::VectorTyID) {
    const llvm::Type *DstVecTy = DstTy->getScalarType();
    const llvm::Type *SrcVecTy = SrcTy->getScalarType();
    uint32_t DBitWidth = llvm::cast<llvm::IntegerType>(DstVecTy)->getBitWidth();
    unsigned size = Src.AggregateVal.size();
    // the sizes of src and dst vectors must be equal.
    Dest.AggregateVal.resize(size);

    if (SrcVecTy->getTypeID() == llvm::Type::FloatTyID) {
      assert(SrcVecTy->isFloatingPointTy() && "Invalid FPToUI instruction");
      for (unsigned i = 0; i < size; i++)
        Dest.AggregateVal[i].IntVal = llvm::APIntOps::RoundFloatToAPInt(
                                                                        Src.AggregateVal[i].FloatVal, DBitWidth);
    } else {
      for (unsigned i = 0; i < size; i++)
        Dest.AggregateVal[i].IntVal = llvm::APIntOps::RoundDoubleToAPInt(
                                                                         Src.AggregateVal[i].DoubleVal, DBitWidth);
    }
  } else {
    // scalar
    uint32_t DBitWidth = llvm::cast<llvm::IntegerType>(DstTy)->getBitWidth();
    assert(SrcTy->isFloatingPointTy() && "Invalid FPToUI instruction");

    if (SrcTy->getTypeID() == llvm::Type::FloatTyID)
      Dest.IntVal = llvm::APIntOps::RoundFloatToAPInt(Src.FloatVal, DBitWidth);
    else {
      Dest.IntVal = llvm::APIntOps::RoundDoubleToAPInt(Src.DoubleVal, DBitWidth);
    }
  }

  return Dest;
}

llvm::GenericValue POWERInterpreter::executeFPToSIInst(llvm::Value *SrcVal,
                                                       const llvm::GenericValue &Src,
                                                       llvm::Type *DstTy) {
  llvm::Type *SrcTy = SrcVal->getType();
  llvm::GenericValue Dest;

  if (SrcTy->getTypeID() == llvm::Type::VectorTyID) {
    const llvm::Type *DstVecTy = DstTy->getScalarType();
    const llvm::Type *SrcVecTy = SrcTy->getScalarType();
    uint32_t DBitWidth = llvm::cast<llvm::IntegerType>(DstVecTy)->getBitWidth();
    unsigned size = Src.AggregateVal.size();
    // the sizes of src and dst vectors must be equal
    Dest.AggregateVal.resize(size);

    if (SrcVecTy->getTypeID() == llvm::Type::FloatTyID) {
      assert(SrcVecTy->isFloatingPointTy() && "Invalid FPToSI instruction");
      for (unsigned i = 0; i < size; i++)
        Dest.AggregateVal[i].IntVal = llvm::APIntOps::RoundFloatToAPInt(
                                                                        Src.AggregateVal[i].FloatVal, DBitWidth);
    } else {
      for (unsigned i = 0; i < size; i++)
        Dest.AggregateVal[i].IntVal = llvm::APIntOps::RoundDoubleToAPInt(
                                                                         Src.AggregateVal[i].DoubleVal, DBitWidth);
    }
  } else {
    // scalar
    unsigned DBitWidth = llvm::cast<llvm::IntegerType>(DstTy)->getBitWidth();
    assert(SrcTy->isFloatingPointTy() && "Invalid FPToSI instruction");

    if (SrcTy->getTypeID() == llvm::Type::FloatTyID)
      Dest.IntVal = llvm::APIntOps::RoundFloatToAPInt(Src.FloatVal, DBitWidth);
    else {
      Dest.IntVal = llvm::APIntOps::RoundDoubleToAPInt(Src.DoubleVal, DBitWidth);
    }
  }
  return Dest;
}

llvm::GenericValue POWERInterpreter::executeUIToFPInst(llvm::Value *SrcVal,
                                                       const llvm::GenericValue &Src,
                                                       llvm::Type *DstTy) {
  llvm::GenericValue Dest;

  if (SrcVal->getType()->getTypeID() == llvm::Type::VectorTyID) {
    const llvm::Type *DstVecTy = DstTy->getScalarType();
    unsigned size = Src.AggregateVal.size();
    // the sizes of src and dst vectors must be equal
    Dest.AggregateVal.resize(size);

    if (DstVecTy->getTypeID() == llvm::Type::FloatTyID) {
      assert(DstVecTy->isFloatingPointTy() && "Invalid UIToFP instruction");
      for (unsigned i = 0; i < size; i++)
        Dest.AggregateVal[i].FloatVal =
          llvm::APIntOps::RoundAPIntToFloat(Src.AggregateVal[i].IntVal);
    } else {
      for (unsigned i = 0; i < size; i++)
        Dest.AggregateVal[i].DoubleVal =
          llvm::APIntOps::RoundAPIntToDouble(Src.AggregateVal[i].IntVal);
    }
  } else {
    // scalar
    assert(DstTy->isFloatingPointTy() && "Invalid UIToFP instruction");
    if (DstTy->getTypeID() == llvm::Type::FloatTyID)
      Dest.FloatVal = llvm::APIntOps::RoundAPIntToFloat(Src.IntVal);
    else {
      Dest.DoubleVal = llvm::APIntOps::RoundAPIntToDouble(Src.IntVal);
    }
  }
  return Dest;
}

llvm::GenericValue POWERInterpreter::executeSIToFPInst(llvm::Value *SrcVal,
                                                       const llvm::GenericValue &Src,
                                                       llvm::Type *DstTy) {
  llvm::GenericValue Dest;

  if (SrcVal->getType()->getTypeID() == llvm::Type::VectorTyID) {
    const llvm::Type *DstVecTy = DstTy->getScalarType();
    unsigned size = Src.AggregateVal.size();
    // the sizes of src and dst vectors must be equal
    Dest.AggregateVal.resize(size);

    if (DstVecTy->getTypeID() == llvm::Type::FloatTyID) {
      assert(DstVecTy->isFloatingPointTy() && "Invalid SIToFP instruction");
      for (unsigned i = 0; i < size; i++)
        Dest.AggregateVal[i].FloatVal =
          llvm::APIntOps::RoundSignedAPIntToFloat(Src.AggregateVal[i].IntVal);
    } else {
      for (unsigned i = 0; i < size; i++)
        Dest.AggregateVal[i].DoubleVal =
          llvm::APIntOps::RoundSignedAPIntToDouble(Src.AggregateVal[i].IntVal);
    }
  } else {
    // scalar
    assert(DstTy->isFloatingPointTy() && "Invalid SIToFP instruction");

    if (DstTy->getTypeID() == llvm::Type::FloatTyID)
      Dest.FloatVal = llvm::APIntOps::RoundSignedAPIntToFloat(Src.IntVal);
    else {
      Dest.DoubleVal = llvm::APIntOps::RoundSignedAPIntToDouble(Src.IntVal);
    }
  }

  return Dest;
}

llvm::GenericValue POWERInterpreter::executePtrToIntInst(llvm::Value *SrcVal,
                                                         const llvm::GenericValue &Src,
                                                         llvm::Type *DstTy) {
  uint32_t DBitWidth = llvm::cast<llvm::IntegerType>(DstTy)->getBitWidth();
  llvm::GenericValue Dest;
  assert(SrcVal->getType()->isPointerTy() && "Invalid PtrToInt instruction");

  Dest.IntVal = llvm::APInt(DBitWidth, (intptr_t) Src.PointerVal);
  return Dest;
}

llvm::GenericValue POWERInterpreter::executeIntToPtrInst(llvm::Value *SrcVal,
                                                         const llvm::GenericValue &_Src,
                                                         llvm::Type *DstTy) {
  llvm::GenericValue Dest, Src = _Src;
  assert(DstTy->isPointerTy() && "Invalid PtrToInt instruction");

  uint32_t PtrSize = TD.getPointerSizeInBits();
  if (PtrSize != Src.IntVal.getBitWidth())
    Src.IntVal = Src.IntVal.zextOrTrunc(PtrSize);

  Dest.PointerVal = llvm::PointerTy(intptr_t(Src.IntVal.getZExtValue()));
  return Dest;
}

llvm::GenericValue POWERInterpreter::executeBitCastInst(llvm::Value *SrcVal,
                                                        const llvm::GenericValue &Src,
                                                        llvm::Type *DstTy) {

  // This instruction supports bitwise conversion of vectors to integers and
  // to vectors of other types (as long as they have the same size)
  llvm::Type *SrcTy = SrcVal->getType();
  llvm::GenericValue Dest;

  if ((SrcTy->getTypeID() == llvm::Type::VectorTyID) ||
      (DstTy->getTypeID() == llvm::Type::VectorTyID)) {
    // vector src bitcast to vector dst or vector src bitcast to scalar dst or
    // scalar src bitcast to vector dst
    bool isLittleEndian = TD.isLittleEndian();
    llvm::GenericValue TempDst, TempSrc, SrcVec;
    const llvm::Type *SrcElemTy;
    const llvm::Type *DstElemTy;
    unsigned SrcBitSize;
    unsigned DstBitSize;
    unsigned SrcNum;
    unsigned DstNum;

    if (SrcTy->getTypeID() == llvm::Type::VectorTyID) {
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

    if (DstTy->getTypeID() == llvm::Type::VectorTyID) {
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
          llvm::APInt::floatToBits(SrcVec.AggregateVal[i].FloatVal);

    } else if (SrcElemTy->isDoubleTy()) {
      for (unsigned i = 0; i < SrcNum; i++)
        TempSrc.AggregateVal[i].IntVal =
          llvm::APInt::doubleToBits(SrcVec.AggregateVal[i].DoubleVal);
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
        llvm::GenericValue Elt;
        Elt.IntVal = 0;
        Elt.IntVal = Elt.IntVal.zext(DstBitSize);
        unsigned ShiftAmt = isLittleEndian ? 0 : SrcBitSize * (Ratio - 1);
        for (unsigned j = 0; j < Ratio; j++) {
          llvm::APInt Tmp;
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
          llvm::GenericValue Elt;
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
    if (DstTy->getTypeID() == llvm::Type::VectorTyID) {
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
      else if (DstElemTy->isFloatTy()) {
        Dest.FloatVal = TempDst.AggregateVal[0].IntVal.bitsToFloat();
      } else {
        Dest.IntVal = TempDst.AggregateVal[0].IntVal;
      }
    }
  } else { //  if ((SrcTy->getTypeID() == llvm::Type::VectorTyID) ||
           //     (DstTy->getTypeID() == llvm::Type::VectorTyID))

    // scalar src bitcast to scalar dst
    if (DstTy->isPointerTy()) {
      assert(SrcTy->isPointerTy() && "Invalid BitCast");
      Dest.PointerVal = Src.PointerVal;
    } else if (DstTy->isIntegerTy()) {
      if (SrcTy->isFloatTy())
        Dest.IntVal = llvm::APInt::floatToBits(Src.FloatVal);
      else if (SrcTy->isDoubleTy()) {
        Dest.IntVal = llvm::APInt::doubleToBits(Src.DoubleVal);
      } else if (SrcTy->isIntegerTy()) {
        Dest.IntVal = Src.IntVal;
      } else {
        llvm_unreachable("Invalid BitCast");
      }
    } else if (DstTy->isFloatTy()) {
      if (SrcTy->isIntegerTy())
        Dest.FloatVal = Src.IntVal.bitsToFloat();
      else {
        Dest.FloatVal = Src.FloatVal;
      }
    } else if (DstTy->isDoubleTy()) {
      if (SrcTy->isIntegerTy())
        Dest.DoubleVal = Src.IntVal.bitsToDouble();
      else {
        Dest.DoubleVal = Src.DoubleVal;
      }
    } else {
      llvm_unreachable("Invalid Bitcast");
    }
  }

  return Dest;
}

void POWERInterpreter::visitTruncInst(llvm::TruncInst &I) {
  setCurInstrValue(executeTruncInst(I.getOperand(0), getOperandValue(0), I.getType()));
}

void POWERInterpreter::visitSExtInst(llvm::SExtInst &I) {
  setCurInstrValue(executeSExtInst(I.getOperand(0), getOperandValue(0), I.getType()));
}

void POWERInterpreter::visitZExtInst(llvm::ZExtInst &I) {
  setCurInstrValue(executeZExtInst(I.getOperand(0), getOperandValue(0), I.getType()));
}

void POWERInterpreter::visitFPTruncInst(llvm::FPTruncInst &I) {
  setCurInstrValue(executeFPTruncInst(I.getOperand(0), getOperandValue(0), I.getType()));
}

void POWERInterpreter::visitFPExtInst(llvm::FPExtInst &I) {
  setCurInstrValue(executeFPExtInst(I.getOperand(0), getOperandValue(0), I.getType()));
}

void POWERInterpreter::visitUIToFPInst(llvm::UIToFPInst &I) {
  setCurInstrValue(executeUIToFPInst(I.getOperand(0), getOperandValue(0), I.getType()));
}

void POWERInterpreter::visitSIToFPInst(llvm::SIToFPInst &I) {
  setCurInstrValue(executeSIToFPInst(I.getOperand(0), getOperandValue(0), I.getType()));
}

void POWERInterpreter::visitFPToUIInst(llvm::FPToUIInst &I) {
  setCurInstrValue(executeFPToUIInst(I.getOperand(0), getOperandValue(0), I.getType()));
}

void POWERInterpreter::visitFPToSIInst(llvm::FPToSIInst &I) {
  setCurInstrValue(executeFPToSIInst(I.getOperand(0), getOperandValue(0), I.getType()));
}

void POWERInterpreter::visitPtrToIntInst(llvm::PtrToIntInst &I) {
  setCurInstrValue(executePtrToIntInst(I.getOperand(0), getOperandValue(0), I.getType()));
}

void POWERInterpreter::visitIntToPtrInst(llvm::IntToPtrInst &I) {
  setCurInstrValue(executeIntToPtrInst(I.getOperand(0), getOperandValue(0), I.getType()));
}

void POWERInterpreter::visitBitCastInst(llvm::BitCastInst &I) {
  setCurInstrValue(executeBitCastInst(I.getOperand(0), getOperandValue(0), I.getType()));
}

#define IMPLEMENT_VAARG(TY)                                     \
  case llvm::Type::TY##TyID: Dest.TY##Val = Src.TY##Val; break

void POWERInterpreter::visitVAArgInst(llvm::VAArgInst &I) {
  // Get the incoming valist parameter.  LLI treats the valist as a
  // (ec-stack-depth var-arg-index) pair.
  llvm::GenericValue VAList = getOperandValue(0);
  llvm::GenericValue Dest;
  llvm::GenericValue Src = Threads[CurrentThread].ECStack[VAList.UIntPairVal.first]
    .VarArgs[VAList.UIntPairVal.second];
  llvm::Type *Ty = I.getType();
  switch (Ty->getTypeID()) {
  case llvm::Type::IntegerTyID:
    Dest.IntVal = Src.IntVal;
    break;
    IMPLEMENT_VAARG(Pointer);
    IMPLEMENT_VAARG(Float);
    IMPLEMENT_VAARG(Double);
  default:
    llvm::dbgs() << "Unhandled dest type for vaarg instruction: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }

  setCurInstrValue(Dest);

  // Move the pointer to the next vararg.
  ++VAList.UIntPairVal.second;
}

void POWERInterpreter::visitExtractElementInst(llvm::ExtractElementInst &I) {
  llvm::GenericValue Src1 = getOperandValue(0);
  llvm::GenericValue Src2 = getOperandValue(1);
  llvm::GenericValue Dest;

  llvm::Type *Ty = I.getType();
  const unsigned indx = unsigned(Src2.IntVal.getZExtValue());

  if(Src1.AggregateVal.size() > indx) {
    switch (Ty->getTypeID()) {
    default:
      llvm::dbgs() << "Unhandled destination type for extractelement instruction: "
                   << *Ty << "\n";
      llvm_unreachable(nullptr);
      break;
    case llvm::Type::IntegerTyID:
      Dest.IntVal = Src1.AggregateVal[indx].IntVal;
      break;
    case llvm::Type::FloatTyID:
      Dest.FloatVal = Src1.AggregateVal[indx].FloatVal;
      break;
    case llvm::Type::DoubleTyID:
      Dest.DoubleVal = Src1.AggregateVal[indx].DoubleVal;
      break;
    }
  } else {
    llvm::dbgs() << "Invalid index in extractelement instruction\n";
  }

  setCurInstrValue(Dest);
}

void POWERInterpreter::visitInsertElementInst(llvm::InsertElementInst &I) {
  llvm::Type *Ty = I.getType();

  if(!(Ty->isVectorTy()) )
    llvm_unreachable("Unhandled dest type for insertelement instruction");

  llvm::GenericValue Src1 = getOperandValue(0);
  llvm::GenericValue Src2 = getOperandValue(1);
  llvm::GenericValue Src3 = getOperandValue(2);
  llvm::GenericValue Dest;

  llvm::Type *TyContained = Ty->getContainedType(0);

  const unsigned indx = unsigned(Src3.IntVal.getZExtValue());
  Dest.AggregateVal = Src1.AggregateVal;

  if(Src1.AggregateVal.size() <= indx)
    llvm_unreachable("Invalid index in insertelement instruction");
  switch (TyContained->getTypeID()) {
  default:
    llvm_unreachable("Unhandled dest type for insertelement instruction");
  case llvm::Type::IntegerTyID:
    Dest.AggregateVal[indx].IntVal = Src2.IntVal;
    break;
  case llvm::Type::FloatTyID:
    Dest.AggregateVal[indx].FloatVal = Src2.FloatVal;
    break;
  case llvm::Type::DoubleTyID:
    Dest.AggregateVal[indx].DoubleVal = Src2.DoubleVal;
    break;
  }
  setCurInstrValue(Dest);
}

void POWERInterpreter::visitShuffleVectorInst(llvm::ShuffleVectorInst &I){
  llvm::Type *Ty = I.getType();
  if(!(Ty->isVectorTy()))
    llvm_unreachable("Unhandled dest type for shufflevector instruction");

  llvm::GenericValue Src1 = getOperandValue(0);
  llvm::GenericValue Src2 = getOperandValue(1);
  llvm::GenericValue Src3 = getOperandValue(2);
  llvm::GenericValue Dest;

  // There is no need to check types of src1 and src2, because the compiled
  // bytecode can't contain different types for src1 and src2 for a
  // shufflevector instruction.

  llvm::Type *TyContained = Ty->getContainedType(0);
  unsigned src1Size = (unsigned)Src1.AggregateVal.size();
  unsigned src2Size = (unsigned)Src2.AggregateVal.size();
  unsigned src3Size = (unsigned)Src3.AggregateVal.size();

  Dest.AggregateVal.resize(src3Size);

  switch (TyContained->getTypeID()) {
  default:
    llvm_unreachable("Unhandled dest type for insertelement instruction");
    break;
  case llvm::Type::IntegerTyID:
    for( unsigned i=0; i<src3Size; i++) {
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
  case llvm::Type::FloatTyID:
    for( unsigned i=0; i<src3Size; i++) {
      unsigned j = Src3.AggregateVal[i].IntVal.getZExtValue();
      if(j < src1Size)
        Dest.AggregateVal[i].FloatVal = Src1.AggregateVal[j].FloatVal;
      else if(j < src1Size + src2Size)
        Dest.AggregateVal[i].FloatVal = Src2.AggregateVal[j-src1Size].FloatVal;
      else
        llvm_unreachable("Invalid mask in shufflevector instruction");
    }
    break;
  case llvm::Type::DoubleTyID:
    for( unsigned i=0; i<src3Size; i++) {
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
  setCurInstrValue(Dest);
}

void POWERInterpreter::visitExtractValueInst(llvm::ExtractValueInst &I) {
  llvm::Value *Agg = I.getAggregateOperand();
  llvm::GenericValue Dest;
  assert(I.getAggregateOperand() == I.getOperand(0));
  llvm::GenericValue Src = getOperandValue(0);

  llvm::ExtractValueInst::idx_iterator IdxBegin = I.idx_begin();
  unsigned Num = I.getNumIndices();
  llvm::GenericValue *pSrc = &Src;

  for (unsigned i = 0 ; i < Num; ++i) {
    pSrc = &pSrc->AggregateVal[*IdxBegin];
    ++IdxBegin;
  }

  llvm::Type *IndexedType = llvm::ExtractValueInst::getIndexedType(Agg->getType(), I.getIndices());
  switch (IndexedType->getTypeID()) {
  default:
    llvm_unreachable("Unhandled dest type for extractelement instruction");
    break;
  case llvm::Type::IntegerTyID:
    Dest.IntVal = pSrc->IntVal;
    break;
  case llvm::Type::FloatTyID:
    Dest.FloatVal = pSrc->FloatVal;
    break;
  case llvm::Type::DoubleTyID:
    Dest.DoubleVal = pSrc->DoubleVal;
    break;
  case llvm::Type::ArrayTyID:
  case llvm::Type::StructTyID:
  case llvm::Type::VectorTyID:
    Dest.AggregateVal = pSrc->AggregateVal;
    break;
  case llvm::Type::PointerTyID:
    Dest.PointerVal = pSrc->PointerVal;
    break;
  }

  setCurInstrValue(Dest);
}

void POWERInterpreter::visitInsertValueInst(llvm::InsertValueInst &I) {
  llvm::Value *Agg = I.getAggregateOperand();

  assert(I.getAggregateOperand() == I.getOperand(0));
  llvm::GenericValue Src1 = getOperandValue(0);
  llvm::GenericValue Src2 = getOperandValue(1);
  llvm::GenericValue Dest = Src1; // Dest is a slightly changed Src1

  llvm::ExtractValueInst::idx_iterator IdxBegin = I.idx_begin();
  unsigned Num = I.getNumIndices();

  llvm::GenericValue *pDest = &Dest;
  for (unsigned i = 0 ; i < Num; ++i) {
    pDest = &pDest->AggregateVal[*IdxBegin];
    ++IdxBegin;
  }
  // pDest points to the target value in the Dest now

  llvm::Type *IndexedType = llvm::ExtractValueInst::getIndexedType(Agg->getType(), I.getIndices());

  switch (IndexedType->getTypeID()) {
  default:
    llvm_unreachable("Unhandled dest type for insertelement instruction");
    break;
  case llvm::Type::IntegerTyID:
    pDest->IntVal = Src2.IntVal;
    break;
  case llvm::Type::FloatTyID:
    pDest->FloatVal = Src2.FloatVal;
    break;
  case llvm::Type::DoubleTyID:
    pDest->DoubleVal = Src2.DoubleVal;
    break;
  case llvm::Type::ArrayTyID:
  case llvm::Type::StructTyID:
  case llvm::Type::VectorTyID:
    pDest->AggregateVal = Src2.AggregateVal;
    break;
  case llvm::Type::PointerTyID:
    pDest->PointerVal = Src2.PointerVal;
    break;
  }

  setCurInstrValue(Dest);
}

llvm::GenericValue POWERInterpreter::getConstantExprValue (llvm::ConstantExpr *CE) {
  switch (CE->getOpcode()) {
  case llvm::Instruction::Trunc:
    return executeTruncInst(CE->getOperand(0),
                            getConstantOperandValue(CE->getOperand(0)),
                            CE->getType());
  case llvm::Instruction::ZExt:
    return executeZExtInst(CE->getOperand(0),
                           getConstantOperandValue(CE->getOperand(0)),
                           CE->getType());
  case llvm::Instruction::SExt:
    return executeSExtInst(CE->getOperand(0),
                           getConstantOperandValue(CE->getOperand(0)),
                           CE->getType());
  case llvm::Instruction::FPTrunc:
    return executeFPTruncInst(CE->getOperand(0),
                              getConstantOperandValue(CE->getOperand(0)),
                              CE->getType());
  case llvm::Instruction::FPExt:
    return executeFPExtInst(CE->getOperand(0),
                            getConstantOperandValue(CE->getOperand(0)),
                            CE->getType());
  case llvm::Instruction::UIToFP:
    return executeUIToFPInst(CE->getOperand(0),
                             getConstantOperandValue(CE->getOperand(0)),
                             CE->getType());
  case llvm::Instruction::SIToFP:
    return executeSIToFPInst(CE->getOperand(0),
                             getConstantOperandValue(CE->getOperand(0)),
                             CE->getType());
  case llvm::Instruction::FPToUI:
    return executeFPToUIInst(CE->getOperand(0),
                             getConstantOperandValue(CE->getOperand(0)),
                             CE->getType());
  case llvm::Instruction::FPToSI:
    return executeFPToSIInst(CE->getOperand(0),
                             getConstantOperandValue(CE->getOperand(0)),
                             CE->getType());
  case llvm::Instruction::PtrToInt:
    return executePtrToIntInst(CE->getOperand(0),
                               getConstantOperandValue(CE->getOperand(0)),
                               CE->getType());
  case llvm::Instruction::IntToPtr:
    return executeIntToPtrInst(CE->getOperand(0),
                               getConstantOperandValue(CE->getOperand(0)),
                               CE->getType());
  case llvm::Instruction::BitCast:
    return executeBitCastInst(CE->getOperand(0),
                              getConstantOperandValue(CE->getOperand(0)),
                              CE->getType());
  case llvm::Instruction::GetElementPtr:
    {
      std::vector<FetchedInstruction::Operand> ops;
      const unsigned NO = CE->getNumOperands();
      ops.reserve(NO);
      for(unsigned i = 0; i < NO; ++i){
        ops.push_back({getConstantOperandValue(CE->getOperand(i))});
      }
      return executeGEPOperation(CE->getOperand(0),
                                 ops,
                                 gep_type_begin(CE),
                                 gep_type_end(CE));
    }
  case llvm::Instruction::FCmp:
  case llvm::Instruction::ICmp:
    return executeCmpInst(CE->getPredicate(),
                          getConstantOperandValue(CE->getOperand(0)),
                          getConstantOperandValue(CE->getOperand(1)),
                          CE->getOperand(0)->getType());
  case llvm::Instruction::Select:
    return executeSelectInst(getConstantOperandValue(CE->getOperand(0)),
                             getConstantOperandValue(CE->getOperand(1)),
                             getConstantOperandValue(CE->getOperand(2)),
                             CE->getOperand(0)->getType());
  default :
    break;
  }

  // The cases below here require a llvm::GenericValue parameter for the result
  // so we initialize one, compute it and then return it.
  llvm::GenericValue Op0 = getConstantOperandValue(CE->getOperand(0));
  llvm::GenericValue Op1 = getConstantOperandValue(CE->getOperand(1));
  llvm::GenericValue Dest;
  llvm::Type * Ty = CE->getOperand(0)->getType();
  switch (CE->getOpcode()) {
  case llvm::Instruction::Add:  Dest.IntVal = Op0.IntVal + Op1.IntVal; break;
  case llvm::Instruction::Sub:  Dest.IntVal = Op0.IntVal - Op1.IntVal; break;
  case llvm::Instruction::Mul:  Dest.IntVal = Op0.IntVal * Op1.IntVal; break;
  case llvm::Instruction::FAdd: executeFAddInst(Dest, Op0, Op1, Ty); break;
  case llvm::Instruction::FSub: executeFSubInst(Dest, Op0, Op1, Ty); break;
  case llvm::Instruction::FMul: executeFMulInst(Dest, Op0, Op1, Ty); break;
  case llvm::Instruction::FDiv: executeFDivInst(Dest, Op0, Op1, Ty); break;
  case llvm::Instruction::FRem: executeFRemInst(Dest, Op0, Op1, Ty); break;
  case llvm::Instruction::SDiv: Dest.IntVal = Op0.IntVal.sdiv(Op1.IntVal); break;
  case llvm::Instruction::UDiv: Dest.IntVal = Op0.IntVal.udiv(Op1.IntVal); break;
  case llvm::Instruction::URem: Dest.IntVal = Op0.IntVal.urem(Op1.IntVal); break;
  case llvm::Instruction::SRem: Dest.IntVal = Op0.IntVal.srem(Op1.IntVal); break;
  case llvm::Instruction::And:  Dest.IntVal = Op0.IntVal & Op1.IntVal; break;
  case llvm::Instruction::Or:   Dest.IntVal = Op0.IntVal | Op1.IntVal; break;
  case llvm::Instruction::Xor:  Dest.IntVal = Op0.IntVal ^ Op1.IntVal; break;
  case llvm::Instruction::Shl:
    Dest.IntVal = Op0.IntVal.shl(Op1.IntVal.getZExtValue());
    break;
  case llvm::Instruction::LShr:
    Dest.IntVal = Op0.IntVal.lshr(Op1.IntVal.getZExtValue());
    break;
  case llvm::Instruction::AShr:
    Dest.IntVal = Op0.IntVal.ashr(Op1.IntVal.getZExtValue());
    break;
  default:
    llvm::dbgs() << "Unhandled ConstantExpr: " << *CE << "\n";
    llvm_unreachable("Unhandled ConstantExpr");
  }
  return Dest;
}

llvm::GenericValue POWERInterpreter::getConstantOperandValue(llvm::Value *V) {
  if (llvm::ConstantExpr *CE = llvm::dyn_cast<llvm::ConstantExpr>(V)) {
    return getConstantExprValue(CE);
  } else{
    llvm::Constant *CPV = llvm::dyn_cast<llvm::Constant>(V);
    assert(CPV);
    return getConstantValue(CPV);
  }
}

llvm::GenericValue POWERInterpreter::getConstantValue(llvm::Constant *CPV){
  if(llvm::isa<llvm::ConstantArray>(CPV)){
    llvm::ConstantArray *CA = llvm::cast<llvm::ConstantArray>(CPV);
    llvm::GenericValue GV;
    GV.AggregateVal.resize(CA->getNumOperands());
    for(unsigned i = 0; i < CA->getNumOperands(); ++i){
      GV.AggregateVal[i] = getConstantValue(llvm::cast<llvm::Constant>(CA->getOperand(i)));
    }
    return GV;
  }
  return llvm::ExecutionEngine::getConstantValue(CPV);
}

//===----------------------------------------------------------------------===//
//                        Dispatch and Execution Code
//===----------------------------------------------------------------------===//

void POWERInterpreter::callAssume(llvm::Function *F){
  bool cond = getOperandValue(0).IntVal.getBoolValue();
  if(!cond){
    Threads[CurrentThread].ECStack.clear();
    AtExitHandlers.clear();
    /* Do not call terminate. We don't want to explicitly terminate
     * since that would allow other processes to join with this
     * process.
     */
  }
}

void POWERInterpreter::callAssertFail(llvm::Function *F){
  TB.assertion_error("(unspecified)");
  Threads[CurrentThread].ECStack.clear();
  AtExitHandlers.clear();

  /* Record error in error trace */
  if(conf.ee_store_trace){
    const char *err = 0;
    const char *file = 0;
    int lnno = 0;
    if(3 <= CurInstr->Operands.size()){
      // Arguments have been sent to assert_fail.
      // Assume that they describe the assertion failure.
      err = static_cast<char*>(getOperandValue(0).PointerVal); // Error message
      file = static_cast<char*>(getOperandValue(1).PointerVal); // Source file where the assert statement occurs
      lnno = getOperandValue(2).IntVal.getZExtValue();         // Line number where the assert statement occurs
    }
    std::stringstream ss;
    ss << "Assertion failure";
    if(file) ss << " at " << file << ":" << lnno;
    if(err) ss << ": " << err;
    TB.trace_register_error(CurrentThread,ss.str());
  }

  /* Do not call terminate. We don't want to explicitly terminate
   * since that would allow other processes to join with this
   * process.
   */
}

void POWERInterpreter::callMalloc(llvm::Function *F){
  unsigned n = getOperandValue(0).IntVal.getZExtValue();

  void *Memory = malloc(n);

  llvm::GenericValue Result = llvm::PTOGV(Memory);
  assert(Result.PointerVal && "Null pointer returned by malloc!");
  setCurInstrValue(Result);

  Threads[CurrentThread].Allocas.add(Memory);
}

void POWERInterpreter::callPthreadCreate(llvm::Function *F){
  // Return 0 (success)
  llvm::GenericValue Result;
  Result.IntVal = llvm::APInt(F->getReturnType()->getIntegerBitWidth(),0);
  setCurInstrValue(Result);

  // Add a new stack for the new thread
  int caller_thread = CurrentThread;
  IID<int> caller_iid(caller_thread,CurInstr->EventIndex);
  std::shared_ptr<FetchedInstruction> CallFI = CurInstr;

  /* Find the thread that is created by this call. (It should already
   * have been added to Threads when the current instruction was
   * fetched.)
   */
  {
    for(unsigned i = 0; i < Threads.size(); ++i){
      if(Threads[i].CreateEvent == caller_iid){
        CurrentThread = i;
        break;
      }
      if(i+1 >= Threads.size()){
        llvm::dbgs() << "Failed to find thread created by " << caller_iid << "\n";
      }
      assert(i+1 < Threads.size()); // Should find the right thread before reaching the end of Threads.
    }
  }

  // Build stack frame for the call
  llvm::Function *F_inner = (llvm::Function*)GVTOP(getOperandValue(2));
  std::vector<llvm::Value*> ArgVals_inner;
  llvm::Type *i8ptr = llvm::Type::getInt8PtrTy(F->getContext());
  if(F_inner->getArgumentList().size() == 1 &&
     F_inner->arg_begin()->getType() == i8ptr){
    void *opval = llvm::GVTOP(getOperandValue(3));
    llvm::Type *i64 = llvm::Type::getInt64Ty(F->getContext());
    llvm::Constant *opval_int = llvm::ConstantInt::get(i64,uint64_t(opval));
    llvm::Value *opval_ptr = llvm::ConstantExpr::getIntToPtr(opval_int,i8ptr);
    ArgVals_inner.push_back(opval_ptr);
  }else if(F_inner->getArgumentList().size()){
    std::string _err;
    llvm::raw_string_ostream err(_err);
    err << "Unsupported: function passed as argument to pthread_create has type: "
        << *F_inner->getType();
    throw std::logic_error(err.str().c_str());
  }
  callFunction(F_inner,ArgVals_inner);

  static POWERARMTraceBuilder::ldreqfun_t isone =
    [](const MBlock &B){
    return *((uint8_t*)B.get_block()) == 1;
  };
  IID<int> ld0_iid = TB.fetch(CurrentThread,1,0,{},{});
  TB.register_addr(ld0_iid,0,MRef(Threads[CurrentThread].status,1));
  TB.register_load_requirement(ld0_iid,0,&isone);
  TB.fetch_fence(CurrentThread,POWERARMTraceBuilder::LWSYNC);
  std::shared_ptr<FetchedInstruction> FI_ld0(new FetchedInstruction(*dummy_load8));
  FI_ld0->EventIndex = ld0_iid.get_index();
  Threads[CurrentThread].CommittableEvents.insert(FI_ld0);
  fetchAll();
  commitLocalAndFetchAll();

  // Return to caller
  CurrentThread = caller_thread;
  CurInstr = CallFI;
}

void POWERInterpreter::callPthreadExit(llvm::Function *F){
  fetchThreadExit();
  Threads[CurrentThread].ECStack.clear();
}

void POWERInterpreter::callPthreadJoin(llvm::Function *F){
}

void POWERInterpreter::callPutchar(llvm::Function *F){
  llvm::GenericValue C = getOperandValue(0);
  setCurInstrValue(C); // Return the argument value
  unsigned char c = (unsigned char)C.IntVal.getLimitedValue();
  std::cout << c;;
}

//===----------------------------------------------------------------------===//
// callFunction - Execute the specified function...
//
void POWERInterpreter::callFunction(llvm::Function *F,
                                    const std::vector<llvm::GenericValue> &ArgVals) {
  std::vector<std::shared_ptr<FetchedInstruction> > FIArgVals;
  unsigned i = 0;
  for (llvm::Function::arg_iterator AI = F->arg_begin(), E = F->arg_end();
       AI != E; ++AI, ++i){
    /* Set up a fake FetchedInstruction for each constant argument.
     * The Instruction part will be a dummy, but the value will be
     * the real value of the argument.
     */
    std::shared_ptr<FetchedInstruction> FI(new FetchedInstruction(*dummy_store));
    FI->Value = ArgVals[i];
    FI->Committed = true;
    FIArgVals.push_back(FI);
  }

  callFunction(F,FIArgVals);
}

void POWERInterpreter::callFunction(llvm::Function *F,
                                    const std::vector<llvm::Value*> &ArgVals) {
  ExecutionContext &SF = Threads[CurrentThread].ECStack.back();
  std::vector<std::shared_ptr<FetchedInstruction> > FIArgVals;
  unsigned i = 0;
  for (llvm::Function::arg_iterator AI = F->arg_begin(), E = F->arg_end();
       AI != E; ++AI, ++i){
    if(llvm::isa<llvm::Constant>(ArgVals[i])){
      /* Set up a fake FetchedInstruction for each constant argument.
       * The Instruction part will be a dummy, but the value will be
       * the real value of the argument.
       */
      std::shared_ptr<FetchedInstruction> FI(new FetchedInstruction(*dummy_store));
      FI->Value = getConstantOperandValue(ArgVals[i]);
      FI->Committed = true;
      FIArgVals.push_back(FI);
    }else if(!F->isDeclaration()){
      assert(SF.Values.count(ArgVals[i]));
      FIArgVals.push_back(SF.Values[ArgVals[i]]);
    }
  }

  callFunction(F,FIArgVals);
}

void POWERInterpreter::callFunction(llvm::Function *F,
                                    const std::vector<std::shared_ptr<FetchedInstruction> > &ArgVals) {
  if(F->getName().str() == "__assert_fail"){
    callAssertFail(F);
    return;
  }else if(F->getName().str() == "free"){
    return; // Do nothing
  }else if(F->getName().str() == "malloc"){
    callMalloc(F);
    return;
  }else if(F->getName().str() == "pthread_create"){
    callPthreadCreate(F);
    return;
  }else if(F->getName().str() == "pthread_exit"){
    callPthreadExit(F);
    return;
  }else if(F->getName().str() == "pthread_join"){
    callPthreadJoin(F);
    return;
  }else if(F->getName().str() == "pthread_mutex_destroy" ||
           F->getName().str() == "pthread_mutex_init" ||
           F->getName().str() == "pthread_mutex_lock" ||
           F->getName().str() == "pthread_mutex_unlock"){
    /* Return 0 */
    llvm::GenericValue Result;
    Result.IntVal = llvm::APInt(F->getReturnType()->getIntegerBitWidth(),0);
    setCurInstrValue(Result);
    return;
  }else if(F->getName().str() == "putchar"){
    callPutchar(F);
    return;
  }else if(F->getName().str() == "__VERIFIER_assume"){
    callAssume(F);
    return;
  }
  // Make a new stack frame... and fill it in.
  Threads[CurrentThread].ECStack.push_back(ExecutionContext());
  ExecutionContext &StackFrame = Threads[CurrentThread].ECStack.back();
  StackFrame.CurFunction = F;

  // Special handling for external functions.
  if (F->isDeclaration()) {
    throw std::logic_error("ERROR: Call to external function "+
                           F->getName().str()+
                           " is not supported under POWER.");
  }

  if(conf.ee_store_trace){
    TB.trace_register_function_entry(CurrentThread,F->getName(),0);
  }

  // Get pointers to first LLVM BB & llvm::Instruction in function.
  StackFrame.CurBB     = &F->front();
  StackFrame.CurInst   = StackFrame.CurBB->begin();

  // Run through the function arguments and initialize their values...
  assert((ArgVals.size() == F->arg_size() ||
          (ArgVals.size() > F->arg_size() && F->getFunctionType()->isVarArg()))&&
         "Invalid number of values passed to function invocation!");

  // Handle non-varargs arguments...
  unsigned i = 0;
  for (llvm::Function::arg_iterator AI = F->arg_begin(), E = F->arg_end();
       AI != E; ++AI, ++i){
    StackFrame.Values[&*AI] = ArgVals[i];
  }

  // Handle varargs arguments...
  if(ArgVals.begin()+i != ArgVals.end()){
    throw std::logic_error("POWERInterpreter: Varargs are not supported.");
  }
}

void POWERInterpreter::abort(){
  for(unsigned p = 0; p < Threads.size(); ++p){
    Threads[p].ECStack.clear();
    Threads[p].CommittableEvents.clear();
  }
  TB.abort();
}

int POWERInterpreter::getAddrOpIdx(const llvm::Instruction &I){
  switch(I.getOpcode()){
  case llvm::Instruction::Load: // 0
  case llvm::Instruction::AtomicCmpXchg: // 0
  case llvm::Instruction::AtomicRMW: // 0
    assert(!llvm::isa<llvm::LoadInst>(I) ||
           llvm::cast<llvm::LoadInst>(I).getPointerOperandIndex() == 0);
    assert(!llvm::isa<llvm::AtomicCmpXchgInst>(I) ||
           llvm::cast<llvm::AtomicCmpXchgInst>(I).getPointerOperandIndex() == 0);
    assert(!llvm::isa<llvm::AtomicRMWInst>(I) ||
           llvm::cast<llvm::AtomicRMWInst>(I).getPointerOperandIndex() == 0);
    return 0;
  case llvm::Instruction::Store: // 1
    assert(!llvm::isa<llvm::StoreInst>(I) ||
           llvm::cast<llvm::StoreInst>(I).getPointerOperandIndex() == 1);
    return 1;
  default:
    return -1;
  }
}

void POWERInterpreter::registerOperand(int proc, FetchedInstruction &FI, int idx){
  if(FI.Operands[idx].isAddr()){
    llvm::Function *F = getCallee(FI.I);
    if(F && F->getName() == "pthread_join"){
      assert(idx == 0);
      int tid = FI.Operands[idx].Value.IntVal.getLimitedValue();
      TB.register_addr({proc,FI.EventIndex},0,MRef(Threads[tid].status,1));
      static POWERARMTraceBuilder::ldreqfun_t istwo =
        [](const MBlock &B){
        return *((uint8_t*)B.get_block()) == 2;
      };
      TB.register_load_requirement({proc,FI.EventIndex},0,&istwo);
    }else if(F && F->getName().str().size() > 14 &&
             F->getName().str().substr(0,14) == "pthread_mutex_"){
      /* Access only the first byte of the pthread_t */
      void *addr = (void*)llvm::GVTOP(FI.Operands[idx].Value);
      TB.register_addr({proc,FI.EventIndex},FI.Operands[idx].IsAddrOf,
                       MRef(addr,1));
      if(F->getName().str() == "pthread_mutex_lock"){
        TB.register_addr({proc,FI.EventIndex},1,MRef(addr,1));
        static POWERARMTraceBuilder::ldreqfun_t iszero =
          [](const MBlock &B){
          return *((uint8_t*)B.get_block()) == 0;
        };
        TB.register_load_requirement({proc,FI.EventIndex},0,&iszero);
      }
    }else{
      void *addr = (void*)llvm::GVTOP(FI.Operands[idx].Value);
      llvm::Type *ty =
        FI.I.getOperand(idx)->getType();
      assert(ty->isPointerTy());
      ty = llvm::cast<llvm::PointerType>(ty)->getElementType();
      TB.register_addr({proc,FI.EventIndex},FI.Operands[idx].IsAddrOf,
                       GetMRef(addr,ty));
    }
  }else if(FI.Operands[idx].isData()){
    TB.register_data({proc,FI.EventIndex},FI.Operands[idx].IsDataOf,
                     GetMBlock(0,FI.I.getOperand(idx)->getType(),FI.Operands[idx].Value));
  }
}

llvm::Function *POWERInterpreter::getCallee(llvm::Instruction &I){
  if(I.getOpcode() != llvm::Instruction::Call &&
     I.getOpcode() != llvm::Instruction::Invoke){
    return 0;
  }
  int callee_idx = getCalleeOpIdx(I);
  llvm::Value *fop = I.getOperand(callee_idx);
  if(!llvm::isa<llvm::Constant>(fop)){
    return 0;
  }
  return (llvm::Function*)GVTOP(getConstantOperandValue(fop));
}

std::shared_ptr<POWERInterpreter::FetchedInstruction> POWERInterpreter::fetch(llvm::Instruction &I){
  ExecutionContext &SF = Threads[CurrentThread].ECStack.back();  // Current stack frame
  std::shared_ptr<FetchedInstruction> FI(new FetchedInstruction(I));
  CurInstr = FI;

  int load_count = 0, store_count = 0;
  bool is_event = false;
  bool create_thread = false;
  bool has_fence = false;
  bool is_internal_fun_call = false;
  // Consider fence undefined unless has_fence is true
  // (Initialize to quiet spurious compiler warning.)
  POWERARMTraceBuilder::FenceType fence = POWERARMTraceBuilder::SYNC;
  FI->Operands.resize(I.getNumOperands(),FetchedInstruction::Operand());
  std::vector<llvm::Value*> op_values;
  op_values.reserve(I.getNumOperands());
  for(unsigned i = 0; i < I.getNumOperands(); ++i){
    op_values.push_back(I.getOperand(i));
  }

  struct ExtraAccess{
    ExtraAccess(int aid, MRef addr) : is_addr(true), aid(aid), addr(addr), data(addr,0) {};
    ExtraAccess(int staid, MBlock data) : is_addr(false), aid(staid), addr(data.get_ref()), data(data) {};
    bool is_addr;
    int aid;
    MRef addr;
    MBlock data;
  };
  std::vector<ExtraAccess> extra_accesses;

  switch(I.getOpcode()){
  case llvm::Instruction::Load:
    load_count = 1;
    is_event = true;
    FI->Operands[0].IsAddrOf = 0;
    break;
  case llvm::Instruction::Store:
    store_count = 1;
    is_event = true;
    FI->Operands[0].IsDataOf = 0;
    FI->Operands[1].IsAddrOf = 0;
    break;
  case llvm::Instruction::PHI:
    {
      /* Remove all operands except the one corresponding to the
       * previous BasicBlock.
       */
      assert(SF.PrevBB);
      llvm::PHINode *P = llvm::cast<llvm::PHINode>(&I);
      int i = P->getBasicBlockIndex(SF.PrevBB);
      assert(0 <= i);
      op_values[0] = P->getIncomingValue(i);
      op_values.resize(1);
      FI->Operands.resize(1);
      break;
    }
  case llvm::Instruction::Br:
    FI->IsBranch = true;
    break;
  case llvm::Instruction::Call: case llvm::Instruction::Invoke:
    {
      std::string asmstr;
      if(isInlineAsm(CurInstr->I,&asmstr)){
        if(asmstr == "isync" || asmstr == "ISB" || asmstr == "isb"){
          TB.fetch_fence(CurrentThread,POWERARMTraceBuilder::ISYNC);
        }else if(asmstr == "eieio"){
          TB.fetch_fence(CurrentThread,POWERARMTraceBuilder::EIEIO);
        }else if(asmstr == "lwsync"){
          TB.fetch_fence(CurrentThread,POWERARMTraceBuilder::LWSYNC);
        }else if(asmstr == "sync" || asmstr == "DMB" || asmstr == "DSB" ||
                 asmstr == "dmb" || asmstr == "dsb"){
          TB.fetch_fence(CurrentThread,POWERARMTraceBuilder::SYNC);
        }else if(asmstr == ""){ // Do nothing
        }
      }else{
        llvm::Function *F;
        int callee_idx = getCalleeOpIdx(I);
        if(!llvm::isa<llvm::Constant>(op_values[callee_idx])){
          throw std::logic_error("Unsupported: Function call with non-constant target function.");
        }
        F = (llvm::Function*)GVTOP(getConstantOperandValue(op_values[callee_idx]));
        if(F){
          if(F->getName() == "__VERIFIER_assume" ||
             F->getName() == "__assert_fail"){
            FI->IsBranch = true;
          }else if(F->getName() == "pthread_create"){
            create_thread = true;
            is_event = true;
            store_count = 2;
            FI->Operands[0].IsAddrOf = 0;
            assert(I.getOperand(0)->getType()->isPointerTy());
            assert(llvm::cast<llvm::PointerType>(I.getOperand(0)->getType())->getElementType()->isIntegerTy());
            llvm::IntegerType *ty =
              llvm::cast<llvm::IntegerType>(llvm::cast<llvm::PointerType>(I.getOperand(0)->getType())->getElementType());
#ifdef LLVM_EXECUTIONENGINE_DATALAYOUT_PTR
            int pthread_t_sz = int(getDataLayout()->getTypeStoreSize(ty));
#else
            int pthread_t_sz = int(getDataLayout().getTypeStoreSize(ty));
#endif
            MRef addr(0,pthread_t_sz);
            MBlock data(addr,pthread_t_sz);
            MBlock data1({0,1},1);
            *((uint8_t*)data1.get_block()) = 1;
            llvm::GenericValue Val;
            Val.IntVal = llvm::APInt(ty->getBitWidth(),Threads.size());
            StoreValueToMemory(Val,(llvm::GenericValue*)data.get_block(),ty);
            extra_accesses.push_back(ExtraAccess(0,data));
            extra_accesses.push_back(ExtraAccess(1,data1));
            TB.fetch_fence(CurrentThread,POWERARMTraceBuilder::LWSYNC);
          }else if(F->getName() == "pthread_join"){
            is_event = true;
            load_count = 1;
            FI->Operands[0].IsAddrOf = 0; // Actually the thread identifier
            FI->IsBranch = true;
            has_fence = true;
            fence = POWERARMTraceBuilder::LWSYNC;
          }else if(F->getName() == "pthread_mutex_destroy"){
            // Do nothing
          }else if(F->getName() == "pthread_mutex_init"){
            is_event = true;
            store_count = 1;
            FI->Operands[0].IsAddrOf = 0;
            MBlock data({0,1},1);
            *((uint8_t*)data.get_block()) = 0;
            extra_accesses.push_back(ExtraAccess(0,data));
          }else if(F->getName() == "pthread_mutex_lock"){
            is_event = true;
            load_count = 1;
            store_count = 1;
            FI->Operands[0].IsAddrOf = 0;
            FI->IsBranch = true;
            MBlock data({0,1},1);
            *((uint8_t*)data.get_block()) = 1;
            extra_accesses.push_back(ExtraAccess(1,data));
            has_fence = true;
            fence = POWERARMTraceBuilder::SYNC;
          }else if(F->getName() == "pthread_mutex_unlock"){
            is_event = true;
            store_count = 1;
            FI->Operands[0].IsAddrOf = 0;
            MBlock data({0,1},1);
            *((uint8_t*)data.get_block()) = 0;
            extra_accesses.push_back(ExtraAccess(0,data));
            TB.fetch_fence(CurrentThread,POWERARMTraceBuilder::SYNC);
          }else if(!F->isDeclaration()){
            is_internal_fun_call = true;
            FI->IsBranch = true;
            for(unsigned i = 0; i < FI->Operands.size(); ++i){
              FI->Operands[i].Available = true;
            }
            FI->Operands[callee_idx].Value = getConstantOperandValue(op_values[callee_idx]);
          }
        }
      }
    }
    break;
  default:
    if(I.mayReadOrWriteMemory()){
      std::string s;
      llvm::raw_string_ostream os(s);
      os << "Unsupported instruction: " << I;
      throw std::logic_error(s);
    }
    break;
  }

  /* Setup operands and dependencies */
  if(!is_internal_fun_call){
    assert(FI->Operands.size() == op_values.size());
    for(int i = 0; i < int(FI->Operands.size()); ++i){
      llvm::Value *O = op_values[i];
      if(llvm::isa<llvm::Constant>(O)){
        FI->Operands[i].Value = getConstantOperandValue(O);
        FI->Operands[i].Available = true;
      }else if(O->getType()->isLabelTy() ||
               O->getType()->isMetadataTy() ||
               llvm::isa<llvm::InlineAsm>(O)){
        /* The operand is considered available, but is left
         * uninitialized.
         */
        FI->Operands[i].Available = true;
      }else{
        assert(SF.Values.count(O));
        std::shared_ptr<FetchedInstruction> OFI = SF.Values[O];
        VecSet<int> deps;
        if(OFI->isEvent()){
          deps.insert(OFI->EventIndex);
        }else{
          assert(OFI->AddrDeps.empty());
          deps = OFI->DataDeps;
        }
        if(FI->Operands[i].isAddr()){
          FI->AddrDeps.insert(deps);
        }else{
          FI->DataDeps.insert(deps);
        }
        if(OFI->Committed){
          FI->Operands[i].Value = OFI->Value;
          FI->Operands[i].Available = true;
        }else{
          OFI->Dependents.insert({FI,i});
        }
      }
    }
  }

  if(FI->IsBranch && !is_event){
    assert(FI->AddrDeps.empty());
    if(FI->DataDeps.size()){
      TB.fetch_ctrl(CurrentThread,FI->DataDeps);
    }
  }

  if(is_event){
    IID<int> iid = TB.fetch(CurrentThread,load_count,store_count,FI->AddrDeps,FI->DataDeps);
    FI->EventIndex = iid.get_index();

    if(FI->IsBranch){
      TB.fetch_ctrl(CurrentThread,{iid.get_index()});
    }

    /* Register the available operands */
    for(unsigned i = 0; i < FI->Operands.size(); ++i){
      if(FI->Operands[i].Available){
        registerOperand(CurrentThread,*FI,i);
      }
    }

    if(create_thread){
      Threads.emplace_back(CPS.spawn(Threads[CurrentThread].cpid));
      Threads.back().CreateEvent = iid;
      TB.spawn(CurrentThread);
      extra_accesses.push_back(ExtraAccess(1,MRef((void*)Threads.back().status,1)));
    }

    /* Register the extra accesses */
    for(const ExtraAccess &EA : extra_accesses){
      if(EA.is_addr){
        TB.register_addr(iid,EA.aid,EA.addr);
      }else{
        TB.register_data(iid,EA.aid,EA.data);
      }
    }
  }

  if(has_fence){
    TB.fetch_fence(CurrentThread,fence);
  }

  return FI;
}

void POWERInterpreter::fetchAll(){
  while (!Threads[CurrentThread].ECStack.empty()) {
    // Interpret a single instruction & increment the "PC".
    ExecutionContext &SF = Threads[CurrentThread].ECStack.back();  // Current stack frame
    llvm::Instruction &I = *SF.CurInst++;         // Increment before execute

    std::shared_ptr<FetchedInstruction> FI(fetch(I));

    SF.Values[&I] = FI;
    CurInstr = FI; // Necessary for some methods, e.g. isEvent

    if(FI->committable()){
      if(FI->isEvent()){
        Threads[CurrentThread].CommittableEvents.insert(FI);
      }else{
        CommittableLocal.push_back(FI);
      }
    }

    if(FI->IsBranch){
      return;
    }
    if(I.isTerminator()){
      return;
    }
    if(!FI->isEvent() && (llvm::isa<llvm::CallInst>(I) || llvm::isa<llvm::InvokeInst>(I))){
      return;
    }
  }
}

bool POWERInterpreter::commitAllLocal(){
  bool retval = false;
  while(CommittableLocal.size()){
    std::shared_ptr<FetchedInstruction> FI = CommittableLocal.back();
    CommittableLocal.pop_back();
    retval = retval ||
      FI->I.isTerminator() ||
      (!FI->isEvent() && (llvm::isa<llvm::CallInst>(FI->I) || llvm::isa<llvm::InvokeInst>(FI->I)));
    commit(FI);
  }
  return retval;
}

void POWERInterpreter::commitLocalAndFetchAll(){
  while(commitAllLocal()){
    fetchAll();
  }
}

void POWERInterpreter::commit(){
  assert(!CurInstr->Committed);
  if(conf.ee_store_trace){
    TB.trace_register_metadata(CurrentThread,CurInstr->I.getMetadata("dbg"));
  }
  if(llvm::isa<llvm::PHINode>(CurInstr->I)){
    assert(CurInstr->Operands.size() == 1);
    assert(CurInstr->Operands[0].Available);
    CurInstr->Value = CurInstr->Operands[0].Value;
  }else if(llvm::isa<llvm::LoadInst>(CurInstr->I)){
    // Do nothing
  }else if(llvm::isa<llvm::StoreInst>(CurInstr->I)){
    // Do nothing
  }else{
    visit(CurInstr->I);
  }
  CurInstr->Committed = true;
  for(const FetchedInstruction::Dependent &D : CurInstr->Dependents){
    assert(!D.FI->Committed);
    assert(!D.FI->Operands[D.op_idx].Available);
    D.FI->Operands[D.op_idx].Value = CurInstr->Value;
    D.FI->Operands[D.op_idx].Available = true;
    registerOperand(CurrentThread,*D.FI,D.op_idx);
    if(D.FI->committable()){
      if(D.FI->isEvent()){
        Threads[CurrentThread].CommittableEvents.insert(D.FI);
      }else{
        CommittableLocal.push_back(D.FI);
      }
    }
  }
  CurInstr->Dependents.clear(); // Remove shared_ptrs to release ownership
}

void POWERInterpreter::run() {
  fetchAll();
  commitLocalAndFetchAll();

  bool done = false;
  while(!done){
    CurrentThread = 0;
    IID<int> iid;
    std::vector<MBlock> values;
    if(TB.schedule(&iid,&values)){
      CurrentThread = iid.get_pid();
      std::shared_ptr<FetchedInstruction> FI;
#ifndef NDEBUG
      bool found_event = false;
#endif
      for(std::shared_ptr<FetchedInstruction> f : Threads[CurrentThread].CommittableEvents){
        if(f->EventIndex == iid.get_index()){
          FI = f;
          Threads[CurrentThread].CommittableEvents.erase(f);
#ifndef NDEBUG
          found_event = true;
#endif
          break;
        }
      }
      assert(found_event);
      if(llvm::isa<llvm::LoadInst>(FI->I)){
        assert(values.size() == 1);
        LoadValueFromMemory(FI->Value,(llvm::GenericValue*)values[0].get_block(),FI->I.getType());
      }
      commit(FI);
      if(FI->IsBranch){
        fetchAll();
      }
      commitLocalAndFetchAll();
    }else{
      done = true;
    }
    CurrentThread = 0;
  }
}
