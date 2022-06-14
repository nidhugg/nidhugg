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

#include "PrefixHeuristic.h"
#include "Timing.h"

#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <utility>

#include <llvm/Support/CommandLine.h>

static Timing::Context heuristic_context("heuristic");

static llvm::cl::opt<bool>
cl_no_heuristic("no-heuristic",llvm::cl::NotHidden,
                llvm::cl::desc("Disable prefix heuristic."));

#ifdef TRACE
#  define IFTRACE(X) X
#else
#  define IFTRACE(X) ((void)0)
#endif

Option<std::vector<IID<int>>>
try_generate_prefix(SaturatedGraph g, std::vector<IID<int>> current_exec) {
  Timing::Guard timing_guard(heuristic_context);
  if (cl_no_heuristic) return nullptr;
  std::vector<IID<int>> ids;
  for (IID<int> iid : current_exec)
    if (g.has_event(iid))
      ids.push_back(iid);

  std::map<SymAddr,std::list<IID<int>>> total_co;

  // {
  //   std::ofstream out("saturated.dot");
  //   g.print_graph(out);
  // }


  for (IID<int> w : ids) {
    if (!g.event_is_store(w)) continue;
    const auto &wc = g.event_vc(w);
    SymAddr a = g.event_addr(w);

    auto ins_ptr = total_co[a].end();
    for ( ; ins_ptr != total_co[a].begin(); --ins_ptr) {
      IID<int> i = *std::prev(ins_ptr);
      const auto &ic = g.event_vc(i);
      assert(std::find(current_exec.begin(), current_exec.end(), w)
           > std::find(current_exec.begin(), current_exec.end(), i));
      if (!wc.leq(ic)) {
        break;
      }
    }

    if (ins_ptr != total_co[a].begin()) {
      IID<int> i = *std::prev(ins_ptr);
      IFTRACE(std::cout << "Guessing coherence order from " << i << " to " << w << "\n");
      g.add_edge(i, w);
    }
    if (ins_ptr != total_co[a].end()) {
      IID<int> j = *ins_ptr;
      IFTRACE(std::cout << "Guessing coherence order from " << w << " to " << j << "\n");
      g.add_edge(w, j);
    }
    total_co[a].insert(ins_ptr, w);

    if (!g.saturate()) {
      std::ofstream out("cycle.dot");
      g.print_graph(out);
      return nullptr;
    }
  }

  // {
  //   std::ofstream out("solved.dot");
  //   g.print_graph(out);
  // }

  return toposort(g, std::move(ids));
}

static void toposort(IID<int> id, const SaturatedGraph &g, std::set<IID<int>> &mark,
                     std::vector<IID<int>> &ret) {
  if (mark.count(id)) return;
  mark.emplace(id);
  for (IID<int> in : g.event_in(id)) {
    toposort(in, g, mark, ret);
  }
  ret.push_back(id);
}

std::vector<IID<int>> toposort(const SaturatedGraph &g, std::vector<IID<int>> ids) {
  /* Could be a more efficient data structure. Is probably not a
   * bottleneck, though.
   */
  std::set<IID<int>> mark;
  std::vector<IID<int>> ret;
  ret.reserve(ids.size());

  for (IID<int> id : ids)
    toposort(id, g, mark, ret);

  return ret;
}
