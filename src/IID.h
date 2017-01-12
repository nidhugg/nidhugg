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

#ifndef __IID_H__
#define __IID_H__

#include <ostream>
#include <string>

#include <cassert>

#include <llvm/Support/raw_ostream.h>

/* An IID is an identifier for a particular event (instruction or
 * system event) in a computation. It is a pair (p,i) where p is the
 * pid of the executing process, and i is a natural number denoting
 * the number of instructions or events executed by the process p
 * before and including the indicated instruction or event. I.e.,
 * (p,1) is the first instruction executed by p, (p,2) is the second,
 * and so forth.
 *
 * (p,0) is a dummy instruction identifier used as a null value. All
 * null identifiers are considered to be the same object.
 */
template<typename Pid_t>
class IID{
public:
  /* Create the null IID. */
  IID() : idx(0) {};
  /* Create the IID (p,i). */
  IID(const Pid_t &p, int i) : pid(p), idx(i) { assert(i >= 0); };
  IID(const IID&) = default;
  IID &operator=(const IID&) = default;

  const Pid_t &get_pid() const { return pid; };
  int get_index() const { return idx; };

  bool valid() const { return idx > 0; };
  bool is_null() const { return idx == 0; };

  IID &operator++() { ++idx; return *this; };
  IID operator++(int) { return IID(pid,idx++); };
  IID &operator--() { assert(idx > 0); --idx; return *this; };
  IID operator--(int) { assert(idx > 0); return IID(pid,idx--); };
  IID operator+(int d) const { return IID(pid,idx+d); };
  IID operator-(int d) const { return IID(pid,idx-d); };

  /* Comparison:
   * The comparison operators implement a total order over IIDs.
   *
   * For each process p, the order is such that (p,i) < (p,j) iff i < j.
   */
  bool operator==(const IID &iid) const {
    return (idx == 0 && iid.idx == 0) ||
      (idx == iid.idx && pid == iid.pid);
  };
  bool operator!=(const IID &iid) const { return !((*this) == iid); };
  bool operator<(const IID &iid) const {
    return (idx == 0 && iid.idx != 0) ||
      (idx && iid.idx && (pid < iid.pid || (pid == iid.pid && idx < iid.idx)));
  };
  bool operator<=(const IID &iid) const { return (*this) < iid || (*this) == iid; };
  bool operator>(const IID &iid) const { return iid < (*this); };
  bool operator>=(const IID &iid) const { return iid <= (*this); };

  std::string to_string() const;
private:
  /* This IID is (pid,idx) */
  Pid_t pid;
  int idx;
};

#include "IID.tcc"

template<typename Pid_t>
std::ostream &operator<<(std::ostream &os, const IID<Pid_t> &iid){
  return os << iid.to_string();
}

template<typename Pid_t>
llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const IID<Pid_t> &iid){
  return os << iid.to_string();
}

#endif
