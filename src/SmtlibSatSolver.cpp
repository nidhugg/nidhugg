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

#include <llvm/Support/CommandLine.h>
#include <boost/variant.hpp>

static llvm::cl::opt<std::string> cl_cmd("smtlib-cmd",llvm::cl::NotHidden,
                                         llvm::cl::init("z3 -in"),
                                         llvm::cl::desc("The SMTLib-compatible solver command."));

SmtlibSatSolver::SmtlibSatSolver()
  : out(), in(),
    z3(std::string(cl_cmd), boost::process::std_out > out, boost::process::std_in < in) {
  in << "(set-option :produce-models true)\n";
  in << "(set-logic QF_IDL)\n";
}

SmtlibSatSolver::~SmtlibSatSolver() {}


void SmtlibSatSolver::reset() {
  in << "(reset)\n";
}

void SmtlibSatSolver::alloc_variables(unsigned count) {
  no_vars = count;
  for (unsigned i = 0; i < no_vars; ++i) {
    in << "(declare-fun e" << i << " () Int)\n";
    in << "(assert (>= e" << i << " 0))\n";
    in << "(assert (< e" << i << " " << no_vars << "))\n";
  }

  in << "(assert (distinct ";
  for (unsigned i = 0; i < no_vars; ++i) {
    in << "e" << i << " ";
  }
  in << "))" << std::endl;
}

void SmtlibSatSolver::add_edge(unsigned from, unsigned to) {
  in << "(assert (< e" << from << " e" << to << "))\n";
}

void SmtlibSatSolver::add_edge_disj(unsigned froma, unsigned toa,
                                    unsigned fromb, unsigned tob) {
  in << "(assert (or (< e" << froma << " e" << toa << ")"
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

class SExpr final {
public:
    SExpr() : SExpr(List{{}}) {}
    SExpr(int i) : SExpr(Int{i}) {}
    SExpr(const char *s) : SExpr(Token{s}) {}
    SExpr(std::string s) : SExpr(Token{std::move(s)}) {}
    SExpr(std::vector<SExpr> l) : SExpr(List{std::move(l)}) {}
    SExpr(std::initializer_list<SExpr> l) : SExpr(List{std::move(l)}) {}

    enum Kind : int {
        TOKEN,
        INT,
        LIST,
    };

    class Token {
    public:
        std::string name;
    };

    class Int {
    public:
        int value;
    };

    class List {
    public:
        std::vector<SExpr> elems;
    };

    Kind kind() const { return Kind(variant.which()); };
    const Token &token() const { return boost::get<Token>(variant); }
    const Int &num() const { return boost::get<Int>(variant); }
    const List &list() const { return boost::get<List>(variant); }

private:
    typedef boost::variant<Token,Int,List> vartype;
    SExpr(vartype variant) : variant(std::move(variant)) {}

    vartype variant;
};

typedef std::logic_error parse_error;

static bool endoftok(int c) {
    return c == EOF || std::isspace(c) || c == ')' || c == '(';
}

std::istream &operator>>(std::istream &is, SExpr &sexp) {
    std::ws(is);
    int c = is.get();
    assert(!(std::isspace(c)));
    if (c == EOF) throw parse_error("Got EOF");
    if (c == ')') throw parse_error("Unmatched closing paren");
    if (c == '(') {
        std::vector<SExpr> elems;
        while (std::ws(is).peek() != ')') {
            elems.emplace_back();
            is >> elems.back();
        }
        c = is.get();
        assert(c == ')');
        sexp = SExpr(std::move(elems));
    } else {
        std::string str;
        for(;!endoftok(c); c = is.get()) str.push_back(c);
        is.putback(c);
        char *end;
        int num = std::strtol(str.c_str(), &end, 10);
        if ((end - str.c_str()) != long(str.size())) {
            sexp = SExpr(str);
        } else {
            sexp = SExpr(num);
        }
    }
    return is;
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
    assert(l2.size() == 2);
    assert(l2[1].kind() == SExpr::INT);
    res[i] = l2[1].num().value;
  }

  return res;
}
