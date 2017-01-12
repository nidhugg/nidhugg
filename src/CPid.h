/* Copyright (C) 2014-2017 Carl Leonardsson
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

#ifndef __CPID_H__
#define __CPID_H__

#include <initializer_list>
#include <map>
#include <ostream>
#include <vector>

#include <llvm/Support/raw_ostream.h>

/* A CPid is a "complex process identifier". For real processes, it is
 * an integer sequence <p0.....pn> where p0 = 0, The first process of
 * a system has CPid <0>. The i:th process that is spawned by a
 * process with CPid <p0.....pn> has CPid <p0.....pn.i>. For
 * auxilliary processes, the CPid of the i:th auxilliary process of a
 * real process with CPid <p0.....pn> is <p0.....pn/i>.
 */
class CPid{
public:
  /* Creates the initial CPid <0>. */
  CPid();
  /* Creates the CPid <p0.....pn> where pvec == [p0,...,pn]. */
  CPid(const std::vector<int> &pvec);
  /* Creates the CPid <p0.....pn> where pvec == [p0,...,pn]. */
  CPid(const std::initializer_list<int> &pvec);
  /* Creates the (auxilliary) CPid <p0.....pn/i> where pvec == [p0,...,pn]. */
  CPid(const std::vector<int> &pvec, int i);
  CPid(const CPid&) = default;
  CPid &operator=(const CPid&) = default;

  /* Returns the CPid <p0.....pn.pn1> where this CPid is
   * <p0.....pn>. */
  CPid spawn(int pn1) const;
  /* Returns the CPid <p0.....pn/i> where this CPid is <p0.....pn>.
   */
  CPid aux(int i) const;
  /* Returns the CPid <p0.....pn> where this CPid is either
   * <p0.....pn.pn1> or <pr.....pn/i>.
   */
  CPid parent() const;

  bool is_auxiliary() const;

  /* True for all CPids except <0>. */
  bool has_parent() const;

  /* Returns i where this CPid is <p0.....pn/i>.
   *
   * Pre: is_auxilliary()
   */
  int get_aux_index() const;

  std::string to_string() const;

  /* Comparison implements a total order over CPids. */
  bool operator==(const CPid &c) const { return compare(c) == 0; };
  bool operator!=(const CPid &c) const { return compare(c) != 0; };
  bool operator<(const CPid &c) const { return compare(c) < 0; };
  bool operator<=(const CPid &c) const { return compare(c) <= 0; };
  bool operator>(const CPid &c) const { return compare(c) > 0; };
  bool operator>=(const CPid &c) const { return compare(c) >= 0; };
private:
  /* For a CPid <p0.p1.....pn> or <p0.p1.....pn/i>, the vector
   * proc_seq is [p1,...,pn]. */
  std::vector<int> proc_seq;
  /* For the CPid of a real process, aux_idx == -1. For the CPid
   * <p0.....pn/i> of an auxilliary process, aux_idx == i.
   */
  int aux_idx;

  int compare(const CPid &c) const;
};

inline std::ostream &operator<<(std::ostream &os, const CPid &c){
  return os << c.to_string();
}

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const CPid &c){
  return os << c.to_string();
}

/* A CPidSystem is a set of CPid:s which satisfies that for each
 * member <p0.....pn>, the set also contains its parent, as well as
 * its predecessor <p0.....(pn-1)> if pn>0. Furthermore for each
 * member <p0.....pn/i> the set also contains its parent <p0.....pn>,
 * as well as its predecessor <p0.....pn/(i-1)> if i>0.
 *
 * The purpose of such a system is typically to easily compute the
 * CPid of the next process (auxilliary or real) that is spawned.
 */
class CPidSystem{
public:
  /* Creates the set {<0>}. */
  CPidSystem();
  CPidSystem(const CPidSystem&) = default;
  CPidSystem &operator=(const CPidSystem&) = default;

  /* Spawn a new (meaning not in this set) real process child for c,
   * add it to this set and return it.
   *
   * Pre: c is in this set.
   */
  CPid spawn(const CPid &c);
  /* Create a new (meaning not in this set) auxilliary process child
   * for c, add it to this set and return it.
   *
   * Pre: c is in this set.
   */
  CPid new_aux(const CPid &c);
private:
  /* Each CPid in the set is identified by a natural number: its index
   * in the sequence of CPids in creation order. Each CPid with
   * identifying integer i is stored in cpids[i].
   *
   * For each CPid cpids[i], we have that real_children[i]
   * (resp. aux_children[i]) contains the identifiers of all real
   * (resp. auxilliary) process children of cpids[i] in this set,
   * ordered increasingly.
   *
   * For each CPid cpids[i], except <0>, we have that parent[i] is the
   * identifier of the parent of cpids[i].
   *
   * identifiers[c] is the identifier of c.
   */
  std::vector<std::vector<int> > real_children;
  std::vector<std::vector<int> > aux_children;
  std::vector<int> parent;
  std::vector<CPid> cpids;
  std::map<CPid,int> identifiers;
};

#endif
