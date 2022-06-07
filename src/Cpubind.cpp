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

#include "Cpubind.h"

#ifdef HAVE_HWLOC

#include <llvm/Support/CommandLine.h>
#include <iostream>
#include <string>

namespace {
static llvm::cl::opt<bool>
cl_nobind("no-cpubind", llvm::cl::NotHidden,
          llvm::cl::desc("Do not cpubind threads"));
static llvm::cl::opt<bool>
cl_no_bind_singlify("no-cpubind-singlify", llvm::cl::NotHidden,
                    llvm::cl::desc("Do not singlify cpubind masks"));
static llvm::cl::opt<hwloc_obj_type_t>
cl_pack("cpubind-pack", llvm::cl::NotHidden, llvm::cl::init(HWLOC_OBJ_CORE),
        llvm::cl::desc("Pack threads down to this level"),
        llvm::cl::values(clEnumValN(HWLOC_OBJ_PU,"pu","Processing unit")
                        ,clEnumValN(HWLOC_OBJ_CORE,"core","Physical cores (default)")
#ifdef HWLOC_OBJ_L2CACHE
                        ,clEnumValN(HWLOC_OBJ_L2CACHE,"l2cache","L2 Cache")
                        ,clEnumValN(HWLOC_OBJ_L3CACHE,"l3cache","L3 Cache")
#endif
                        ,clEnumValN(HWLOC_OBJ_NUMANODE,"numanode","NUMA Node")
                        ,clEnumValN(HWLOC_OBJ_PACKAGE,"package","Package")
                        ,clEnumValN(HWLOC_OBJ_MACHINE,"machine","Machine (maximum spread)")
#ifdef LLVM_CL_VALUES_USES_SENTINEL
                        ,clEnumValEnd
#endif
                        ));

  std::string bitmap_to_string(hwloc_bitmap_t bitmap) {
    std::string str;
    int length = hwloc_bitmap_list_snprintf(NULL, 0, bitmap);
    str.resize(length+1);
    hwloc_bitmap_list_snprintf(&str[0], length+1, bitmap);
    str.resize(length);
    return str;
  }

  int count_children(hwloc_obj_t obj) {
    if (obj->type == HWLOC_OBJ_PU) return 1;
    int sum = 0;
    for (unsigned i = 0; i < obj->arity; ++i) {
      sum += count_children(obj->children[i]);
    }
    return sum;
  }
}  // namespace

Cpubind::Cpubind(int n) {
  if (cl_nobind) return;
  if (hwloc_topology_load(topo) == -1)
    throw std::logic_error("Hwloc failed to load");
  hwloc_obj_type_t root_type = cl_pack;
  int root_count = hwloc_get_nbobjs_by_type(topo, root_type);
  if (root_count == 0) {
    std::cerr << "cpubind: Failed to realise pack: There are no objects of that type\n";
    root_type = HWLOC_OBJ_MACHINE;
    root_count = 1;
  }
  std::vector<hwloc_obj_t> roots;
  std::vector<int> root_children;
  int total_children = 0;
  roots.reserve(root_count);
  root_children.reserve(root_count);
  for (int i = 0; i < root_count; ++i) {
    roots.push_back(hwloc_get_obj_by_type(topo, root_type, i));
    int child_count = count_children(roots.back());
    root_children.push_back(child_count);
    total_children += child_count;
  }
  binds.resize(n);
  for (int i = 0, pos = 0; i < root_count; ++i) {
    int picks_left = n - pos;
    int count = (root_children[i]*picks_left + total_children - 1) / total_children;
    if (count)
      hwloc_distrib(topo, &roots[i], 1, binds.data()+pos, count, INT_MAX, 0);
    pos += count;
    total_children -= root_children[i];
  }
  if (!cl_no_bind_singlify) {
    for (int i = 0; i < n; ++i) {
      hwloc_bitmap_singlify(binds[i]);
    }
  }
}

void Cpubind::bind(int i) {
  if (cl_nobind) return;
  if (hwloc_set_cpubind(topo, binds[i], HWLOC_CPUBIND_THREAD) == -1) {
    std::cerr << "Failed to cpubind: " << strerror(errno) << "\n";
  }
}

void Cpubind::bind(std::thread &thread, int i) {
  if (cl_nobind) return;
  if (hwloc_set_thread_cpubind(topo, thread.native_handle(), binds[i], 0) == -1) {
    std::cerr << "Failed to cpubind: " << strerror(errno) << "\n";
  }
}

Cpubind::~Cpubind() {
  for (hwloc_cpuset_t cpuset : binds)
    hwloc_bitmap_free(cpuset);
}

#endif /* defined(HAVE_HWLOC) */
