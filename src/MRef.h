/* Copyright (C) 2014 Carl Leonardsson
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

#ifndef __MREF_H__
#define __MREF_H__

#include <assert.h>

/* An MRef object identifies a continuous non-empty sequence of bytes
 * in memory.
 */
class MRef {
public:
  /* The address of the first byte in the sequence. */
  void *ref;
  /* The number of bytes in the sequence. */
  int size;
  MRef(void *r, int sz) : ref(r), size(sz) { assert(0 < size); };
};

/* A ConstMRef object is as an MRef object, but refers to const
 * memory. */
class ConstMRef {
public:
  /* The address of the first byte in the sequence. */
  const void *ref;
  /* The number of bytes in the sequence. */
  int size;
  ConstMRef(const void *r, int sz) : ref(r), size(sz) { assert(0 < size); };
  ConstMRef(const MRef &R) : ref(R.ref), size(R.size) { assert(0 < size); };
};

#endif

