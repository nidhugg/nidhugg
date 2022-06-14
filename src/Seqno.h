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

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>

struct SeqnoRoot {
  std::atomic_uint_fast64_t thread_count{0};
};

template<unsigned maxnrthreads = 1024>
struct _seqno {
  typedef uint_fast64_t uf64;

  _seqno(SeqnoRoot &root) {
    tid = ++root.thread_count;
  }
  uf64 operator ++() {
    return ++n * maxnrthreads + tid;
  }

  private:
  uf64 tid, n{0};
};

typedef _seqno<> Seqno;

#endif
