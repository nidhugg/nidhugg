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

#include <config.h>
#ifndef __SMTLIB_SAT_SOLVER_H__
#define __SMTLIB_SAT_SOLVER_H__

#include "SatSolver.h"

#ifndef NO_SMTLIB_SOLVER

#ifndef HAVE_BOOST
#  error "BOOST is required"
#endif

#include <boost/process.hpp>

class SmtlibSatSolver final: public SatSolver {
public:
  SmtlibSatSolver();
  virtual ~SmtlibSatSolver();
  virtual void reset();
  virtual void alloc_variables(unsigned count);
  virtual void add_edge(unsigned from, unsigned to) ;
  virtual void add_edge_disj(unsigned froma, unsigned toa,
                             unsigned fromb, unsigned tob);
  virtual bool check_sat();
  virtual std::vector<unsigned> get_model();

private:
  unsigned no_vars;
  boost::process::ipstream out;
  boost::process::opstream in;
  boost::process::child z3;
};

#endif // !defined(NO_SMTLIB_SOLVER)

#endif // !defined(__SMTLIB_SAT_SOLVER_H__)
