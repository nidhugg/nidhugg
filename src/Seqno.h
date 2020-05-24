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
#ifndef __SEQNO_H__
#define __SEQNO_H__

#include <atomic>
#include <cassert>
#include <algorithm>

struct SeqnoRoot {
  std::atomic<unsigned long> thread_count{0};
};

template<unsigned long maxnrthreads = 1024>
struct _seqno {
  typedef unsigned long ulong;

  _seqno(SeqnoRoot &root) {
    tid = ++root.thread_count;
  }
  ulong operator ++() {
    return ++n * maxnrthreads + tid;
  }

  private:
  ulong tid, n{0};
};

typedef _seqno<> Seqno;

#endif
