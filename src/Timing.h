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
#ifndef __TIMING_H__
#define __TIMING_H__

#include <string>

// Parallel RFSC cannot operated with Timing.
#define NO_TIMING
#ifdef NO_TIMING

namespace Timing {
  class Context {
    Context(Context &) = delete;
    Context & operator =(Context &other) = delete;
  public:
    Context(std::string name) {}
  };
  class Guard {
  public:
    Guard(Context &) {}
    Guard(Guard &) = delete;
    Guard & operator =(Guard &other) = delete;
  };
}

#else /* defined(NO_TIMING) */
#include <chrono>

namespace Timing {
  typedef std::chrono::steady_clock clock;

  class Context {
    Context(Context &) = delete;
    Context & operator =(Context &other) = delete;
  public:
    Context(std::string name);
    ~Context();
    std::string name;
    clock::duration inclusive, exclusive;
    unsigned long count;
    Context *next;
  };

  class Guard {
  public:
    Guard(Context &);
    Guard(Guard &) = delete;
    Guard & operator =(Guard &other) = delete;
    ~Guard();
  private:
    friend class Context;
    Context *context;
    Guard *outer_scope;
    clock::duration subcontext_time;
    clock::time_point start;
  };

  void print_report();
}

#endif /* !defined(NO_TIMING) */

#endif /* !defined(__TIMING_H__) */
