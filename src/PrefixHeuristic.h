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
#ifndef __PREFIX_HEURISTIC_H__
#define __PREFIX_HEURISTIC_H__

#include "SaturatedGraph.h"
#include "Option.h"

#include <vector>

Option<std::vector<IID<int>>>
try_generate_prefix(SaturatedGraph g, std::vector<IID<int>> current_exec);

std::vector<IID<int>> toposort(const SaturatedGraph &g,
                               std::vector<IID<int>> ids);

#endif
