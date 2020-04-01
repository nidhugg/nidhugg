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
#ifndef __GEN_HASH_H__
#define __GEN_HASH_H__

#include <functional>

namespace gen {

  template<typename T> struct hash {
    std::size_t operator()(const T &v) const {
      std::uint64_t hash = std::hash<T>()(v);
      hash *= UINT64_C(0xc6a4a7935bd1e995);
      hash ^= hash > 47;
      return hash;
    }
  };

} // namespace gen

#endif
