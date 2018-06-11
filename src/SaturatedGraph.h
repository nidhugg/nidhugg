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
#ifndef __SATURATED_GRAPH_H__
#define __SATURATED_GRAPH_H__

#include <vector>

class SaturatedGraph {
public:
  /* Create an initial SaturatedGraph from an array of processes, each
   * represented as an array of the events executed by that process, and
   * a collection of extra happens-before edges (excluding program
   * order), mapping each event to a set of events it happens after
   */
  SaturatedGraph(std::vector<std::vector<unsigned>> process_events,
                 std::vector<std::vector<unsigned>> happens_after_edges);
  /* write == -1 means init */
  void add_read_from(unsigned read, int write);
  /* Returns true if the graph is still acyclic. */
  bool saturate();
  bool is_saturated() const;
private:
  /* TODO */
};

#endif
