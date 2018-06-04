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

#include <regex>

SmtlibSatSolver::SmtlibSatSolver()
  : out(), in(),
    z3("z3", "-in",
       boost::process::std_out > out, boost::process::std_in < in,
       boost::process::shell) {
  in << "(set-logic QF_IDL)\n";
}

SmtlibSatSolver::~SmtlibSatSolver() {}


void SmtlibSatSolver::reset() {
  in << "(reset)\n";
}

void SmtlibSatSolver::alloc_variables(unsigned count) {
  no_vars = count;
  for (unsigned i = 0; i < no_vars; ++i) {
    in << "(declare-const e" << i << " Int)\n";
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

std::vector<unsigned> SmtlibSatSolver::get_model() {
  std::vector<unsigned> res;
  res.reserve(no_vars);

  in << "(get-value (";
  for (unsigned i = 0; i < no_vars; ++i) {
    in << "e" << i << " ";
  }
  in << "))" << std::endl;

  std::regex sexprr(R"([ (]\(e\d+ (\d+)\)+)");
  for (unsigned i = 0; i < no_vars; ++i) {
    assert(out);
    std::string line;
    std::getline(out, line);
    assert(!line.empty());
    std::smatch sm;
    bool matched = std::regex_match(line,sm,sexprr);
    assert(matched);
    res[i] = std::stoi(sm[1].str());
  }

  return res;
}
