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
#ifndef __SAT_SOLVER_H__
#define __SAT_SOLVER_H__

#include <vector>

class SatSolver {
public:
  virtual ~SatSolver() {}
  /* Clear all variables and constraints. */
  virtual void reset() = 0;
  /* Allocate variables [0,count). Must not be called twice without an
   * intervening call to reset().
   */
  virtual void alloc_variables(unsigned count) = 0;
  /* Add the constraint that from must be before to. */
  virtual void add_edge(unsigned from, unsigned to) = 0;
  /* Add the constraint that either froma is before toa, or fromb is
   * before tob.
   */
  virtual void add_edge_disj(unsigned froma, unsigned toa,
                             unsigned fromb, unsigned tob) = 0;
  /* Returns true iff the constraints are satisfiable. */
  virtual bool check_sat() = 0;
  /* Returns the satisfying assignment, if solve() returned true. */
  virtual std::vector<unsigned> get_model() = 0;
};

#endif
