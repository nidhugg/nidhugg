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

#include "Timing.h"
#include <vector>
#include <iostream>

namespace Timing {
  namespace {
    std::vector<Context*> all_contexts;
    clock global_clock;
    Guard *current_guard = nullptr;
  }

  Context::Context(std::string name)
    : name(name), inclusive(0), exclusive(0), count(0) {
    all_contexts.push_back(this);
  }

  std::unique_ptr<Guard> Context::enter() {
    auto ret = std::make_unique<Guard>
      (this, current_guard, global_clock.now());
    current_guard = ret.get();
    return std::move(ret);
  }

  Guard::Guard(Context *context, Guard *outer_scope, clock::time_point start)
    : context(context), outer_scope(outer_scope), subcontext_time(0),
      start(start) {
  }

  Guard::~Guard() {
    clock::time_point end = global_clock.now();
    clock::duration inclusive = end - start;
    clock::duration exclusive = inclusive - subcontext_time;
    context->inclusive += inclusive;
    context->exclusive += exclusive;
    context->count++;
    if (outer_scope) {
      outer_scope->subcontext_time += inclusive;
    }
    current_guard = outer_scope;
  }

  void print_report() {
    std::cerr << "Name\tCount\tInclusive\tExclusive\n";
    for (Context *c : all_contexts) {
      using namespace std::chrono;
      std::cerr << c->name << "\t" << c->count
                << "\t" << duration_cast<microseconds>(c->inclusive).count()
                << "\t" << duration_cast<microseconds>(c->exclusive).count() << "\n";
    }
  }
}
