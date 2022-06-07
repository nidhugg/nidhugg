/* Copyright (C) 2020 Magnus Lång
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
#ifndef __CPUBIND_H__
#define __CPUBIND_H__
#include <thread>

#ifndef HAVE_HWLOC

class Cpubind {
public:
  Cpubind(int n) {}
  void bind(int i) {}
  void bind(std::thread &thread, int i) {}
};

#else
#include <hwloc.h>

#include <stdexcept>
#include <vector>

class Cpubind {
public:
  Cpubind(int n);
  ~Cpubind();
  void bind(int i);
  void bind(std::thread &thread, int i);
private:
  /* Owning hwloc_topology_t */
  class Topology {
    hwloc_topology_t topology;
  public:
    Topology() {
      if (hwloc_topology_init(&topology) == -1)
        throw std::logic_error("Hwloc failed to initialise");
    }
    ~Topology() { hwloc_topology_destroy(topology); }
    operator hwloc_topology_t() { return topology; }
  } topo;
  std::vector<hwloc_cpuset_t> binds;
};
#endif /* defined(HAVE_HWLOC) */
#endif /* !defined(__CPUBIND_H__) */
