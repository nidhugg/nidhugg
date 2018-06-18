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
#include <list>
#include <iostream>
#include <fstream>

Option<std::vector<unsigned>> try_generate_prefix(SaturatedGraph g) {
  std::vector<unsigned> ids = g.event_ids();
  std::sort(ids.begin(), ids.end());

  std::map<SymAddr,std::list<unsigned>> total_co;

  // {
  //   std::ofstream out("saturated.dot");
  //   g.print_graph(out);
  // }


  for (unsigned w : ids) {
    if (!g.event_is_store(w)) continue;
    const auto &wc = g.event_vc(w);
    SymAddr a = g.event_addr(w);

    auto ins_ptr = total_co[a].end();
    for (;ins_ptr != total_co[a].begin(); --ins_ptr) {
      unsigned i = *std::prev(ins_ptr);
      const auto &ic = g.event_vc(i);
      assert(w > i);
      if (!wc.leq(ic)) {
        break;
      }
    }

    if (ins_ptr != total_co[a].begin()) {
      unsigned i = *std::prev(ins_ptr);
      // std::cout << "Guessing coherence order from " << i << " to " << w << "\n";
      g.add_edge(i, w);
    }
    if (ins_ptr != total_co[a].end()) {
      unsigned j = *ins_ptr;
      // std::cout << "Guessing coherence order from " << w << " to " << j << "\n";
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

static void toposort(unsigned id, const SaturatedGraph &g, std::vector<bool> &mark,
                     std::vector<unsigned> &ret) {
  if (mark[id]) return;
  mark[id] = true;
  for (unsigned in : g.event_in(id)) {
    toposort(in, g, mark, ret);
  }
  ret.push_back(id);
}

std::vector<unsigned> toposort(const SaturatedGraph &g, std::vector<unsigned> ids) {
  std::vector<bool> mark(*std::max_element(ids.begin(), ids.end())+1);
  std::vector<unsigned> ret;
  ret.reserve(ids.size());

  for (unsigned id : ids)
    toposort(id, g, mark, ret);

  return ret;
}
