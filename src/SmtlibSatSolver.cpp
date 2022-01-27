/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
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

#include "SmtlibSatSolver.h"

#ifndef NO_SMTLIB_SOLVER

#include "SExpr.h"

#include <llvm/Support/CommandLine.h>
#include <boost/variant.hpp>
#include <iomanip>

static llvm::cl::opt<bool>
cl_bv("smtlib-bv",llvm::cl::NotHidden,
      llvm::cl::desc("Encode integers as bitvectors in SMTLib."));

static llvm::cl::opt<std::string>
cl_cmd("smtlib-cmd",llvm::cl::NotHidden,
       llvm::cl::init("z3 -in"),
       llvm::cl::desc("The SMTLib-compatible solver command."));

static llvm::cl::opt<bool>
cl_lia("smtlib-lia",llvm::cl::NotHidden,
       llvm::cl::desc("Use logic QF_LIA rather than QF_IDL."));

SmtlibSatSolver::SmtlibSatSolver()
  : out(), in(),
    z3(std::string(cl_cmd), boost::process::std_out > out, boost::process::std_in < in) {
  in << "(set-option :produce-models true)\n";
  if (cl_bv) in << "(set-logic QF_BV)\n";
  else if (cl_lia) in << "(set-logic QF_LIA)\n";
  else in << "(set-logic QF_IDL)\n";
}

SmtlibSatSolver::~SmtlibSatSolver() {}


void SmtlibSatSolver::reset() {
  in << "(reset)\n";
}

static unsigned ilog2(unsigned v) {
  const unsigned int b[] = {0x2, 0xC, 0xF0, 0xFF00, 0xFFFF0000};
  const unsigned int S[] = {1, 2, 4, 8, 16};
  int i;

  unsigned int r = 0; // result of log2(v) will go here
  for (i = 4; i >= 0; i--) { // unroll for speed...
    if (v & b[i]) {
      v >>= S[i];
      r |= S[i];
    }
  }
  return r;
}

void SmtlibSatSolver::alloc_variables(unsigned count) {
  no_vars = count;
  unsigned hexdigs = ilog2(count-2)/4+1;
  for (unsigned i = 0; i < no_vars; ++i) {
    if (cl_bv) {
      in << "(declare-fun e" << i << " () (_ BitVec " << (hexdigs*4) << "))\n";
      if (count != (1u << (hexdigs*4)))
        in << "(assert (bvult e" << i << " #x"
           << std::setfill('0') << std::setw(hexdigs) << std::hex << no_vars << std::dec << "))\n";
    } else {
      in << "(declare-fun e" << i << " () Int)\n";
      in << "(assert (>= e" << i << " 0))\n";
      in << "(assert (< e" << i << " " << no_vars << "))\n";
    }
  }

  in << "(assert (distinct ";
  for (unsigned i = 0; i < no_vars; ++i) {
    in << "e" << i << " ";
  }
  in << "))" << std::endl;
}

void SmtlibSatSolver::add_edge(unsigned from, unsigned to) {
  if (cl_bv) in << "(assert (bvult e" << from << " e" << to << "))\n";
  else in << "(assert (< e" << from << " e" << to << "))\n";
}

void SmtlibSatSolver::add_edge_disj(unsigned froma, unsigned toa,
                                    unsigned fromb, unsigned tob) {
  if (cl_bv) in << "(assert (or (bvult e" << froma << " e" << toa << ")"
               "(bvult e" << fromb << " e" << tob << ")))\n";
  else in << "(assert (or (< e" << froma << " e" << toa << ")"
         "(< e" << fromb << " e" << tob << ")))\n";
}

bool SmtlibSatSolver::check_sat() {
  in << "(check-sat)" << std::endl;

  std::string res;
  std::getline(out, res);
  if (res == "unsat") return false;
  assert(res == "sat");
  return true;
}

std::vector<unsigned> SmtlibSatSolver::get_model() {
  std::vector<unsigned> res;
  res.reserve(no_vars);

  in << "(get-value (";
  for (unsigned i = 0; i < no_vars; ++i) {
    in << "e" << i << " ";
  }
  in << "))" << std::endl;

  SExpr sx;
  out >> sx;
  assert(sx.kind() == SExpr::LIST);
  const auto &l = sx.list().elems;

  for (unsigned i = 0; i < no_vars; ++i) {
    const SExpr &e = l[i];
    assert(e.kind() == SExpr::LIST);
    const auto &l2 = e.list().elems;
    assert(l2.size() == 2 && l2[0].kind() == SExpr::TOKEN
           && l2[0].token().name == ("e" + std::to_string(i)));
    if (cl_bv) {
      if(l2[1].kind() == SExpr::TOKEN) {
        const std::string &t = l2[1].token().name;
        assert(t.size() > 2 && t[0] == '#');
        int base;
        if (t[1] == 'x') base = 16;
        else if (t[1] == 'b') base = 2;
        else abort();
        char *end;
        res[i] = std::strtol(t.c_str()+2, &end, base);
        assert((end - t.c_str()) == long(t.size()));
      } else {
        assert(l2[1].kind() == SExpr::LIST);
        const auto &l3 = l2[1].list().elems;
        assert(l3.size() == 3 && l3[1].kind() == SExpr::TOKEN);
        const std::string &t = l3[1].token().name;
        assert(t.size() > 2 && t[0] == 'b' && t[1] == 'v');
        char *end;
        res[i] = std::strtol(t.c_str()+2, &end, 10);
        assert((end - t.c_str()) == long(t.size()));
      }
    } else {
      assert(l2[1].kind() == SExpr::INT);
      res[i] = l2[1].num().value;
    }
  }

  return res;
}

#endif // !defined(NO_SMTLIB_SOLVER)
