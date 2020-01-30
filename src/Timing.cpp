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

// Parallel RFSC cannot operated with Timing.
#define NO_TIMING
#ifndef NO_TIMING

#include "Timing.h"
#include <vector>
#include <iostream>
#include <algorithm>

namespace Timing {
  namespace {
    Context *all_contexts = nullptr;
    clock global_clock;
    Guard *current_guard = nullptr;
  }

  Context::Context(std::string name)
    : name(name), inclusive(0), exclusive(0), count(0), next(all_contexts) {
    all_contexts = this;
  }

  Context::~Context() {
    /* Find us in all_contexts and unlink */
    Context **c = &all_contexts;
    while (*c != this) c = &(*c)->next;
    *c = next;
  }

  Guard::Guard(Context &context)
    : context(&context), outer_scope(current_guard), subcontext_time(0),
      start(global_clock.now()) {
    current_guard = this;
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
    std::vector<Context*> vec;
    for (Context *c = all_contexts; c; c = c->next) vec.push_back(c);
    std::sort(vec.begin(), vec.end(), [](const Context *a, const Context *b) {
                                        return a->inclusive > b->inclusive;
                                      });

    std::cerr << "Name\tCount\tInclusive\tExclusive\n";
    for (Context *c : vec) {
      using namespace std::chrono;
      std::cerr << c->name << "\t" << c->count
                << "\t" << duration_cast<microseconds>(c->inclusive).count()
                << "\t" << duration_cast<microseconds>(c->exclusive).count() << "\n";
    }
  }
}

#endif /* !defined(NO_TIMING) */
