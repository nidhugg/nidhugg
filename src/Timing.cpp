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

#ifndef NO_TIMING

#include "Timing.h"

#include <llvm/Support/CommandLine.h>

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

namespace Timing {
  namespace impl {
    bool is_enabled;
  }
  namespace {
    Context *all_contexts = nullptr;
    clock global_clock;
    thread_local Guard *current_guard = nullptr;

    llvm::cl::opt<bool, true>
    cl_time("time", llvm::cl::desc("Print timing information."),
            llvm::cl::NotHidden, llvm::cl::location(impl::is_enabled));
  }

  Context::Context(std::string name)
    : name(name), next(all_contexts) {
    all_contexts = this;
  }

  Context::~Context() {
    /* Find us in all_contexts and unlink */
    Context **c = &all_contexts;
    while (*c != this) c = &(*c)->next;
    *c = next;
  }

  Context::Thread *Context::get_thread() {
    Thread *t = my_thread.get();
    if (!t) {
      t = new Thread();
      t->next = first_thread.load(std::memory_order_relaxed);
      while (!first_thread.compare_exchange_weak
             (t->next, t, std::memory_order_relaxed)) {}
    }
    return t;
  }

  Context::Thread::Thread() : inclusive(0), exclusive(0), count(0) {}

  void Guard::begin(Context *c) {
    context = c;
    start = global_clock.now();
    outer_scope = current_guard;
    current_guard = this;
  }

  void Guard::end() {
    clock::time_point end = global_clock.now();
    clock::duration inclusive = end - start;
    clock::duration exclusive = inclusive - subcontext_time;
    Context::Thread *t = context->get_thread();
    t->inclusive += inclusive;
    t->exclusive += exclusive;
    t->count++;
    if (outer_scope) {
      outer_scope->subcontext_time += inclusive;
    }
    current_guard = outer_scope;
  }

  void print_report() {
    assert(impl::is_enabled);
    struct result {
      result(Context *ctxt) : c(ctxt), inclusive(0), exclusive(0) {}
      Context *c;
      clock::duration inclusive, exclusive;
      uint_fast64_t count = 0;
    };
    std::vector<result> vec;
    for (Context *c = all_contexts; c; c = c->next) {
      vec.emplace_back(c);
      result &res = vec.back();
      for (Context::Thread *t = c->first_thread.load(std::memory_order_relaxed);
           t; t = t->next) {
        res.count += t->count;
        res.inclusive += t->inclusive;
        res.exclusive += t->exclusive;
      }
    }
    std::sort(vec.begin(), vec.end(), [](const result &a, const result &b) {
                                        return a.inclusive > b.inclusive;
                                      });

#define OUT(N,C,I,E)                          \
    std::cerr << std::setw(22) << (N)         \
              << std::setw(10) << (C)         \
              << std::setw(12) << (I)         \
              << std::setw(12) << (E) << "\n"
    OUT("Name", "Count", "Inclusive", "Exclusive");
    for (result &r : vec) {
      using std::chrono::duration_cast;
      using std::chrono::microseconds;
      OUT(r.c->name, r.count,
          duration_cast<microseconds>(r.inclusive).count(),
          duration_cast<microseconds>(r.exclusive).count());
    }
#undef OUT
  }

}  // namespace Timing

#endif /* !defined(NO_TIMING) */
