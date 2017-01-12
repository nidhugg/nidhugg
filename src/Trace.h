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

/* This file declares classes that are used to describe a particular
 * execution of a program: the Trace class and a number of Error
 * classes.
 */

#include <config.h>

#ifndef __TRACE_H__
#define __TRACE_H__

#include "CPid.h"
#include "IID.h"

#include <string>
#include <vector>

#if defined(HAVE_LLVM_IR_METADATA_H)
#include <llvm/IR/Metadata.h>
#elif defined(HAVE_LLVM_METADATA_H)
#include <llvm/Metadata.h>
#endif

/* This abstract class describes an error that was detected in a
 * trace. Particular types of errors should be represented by classes
 * that inherit from Error.
 */
class Error{
public:
  /* An error that was discovered in the event loc. */
  Error(const IID<CPid> &loc) : loc(loc) {};
  virtual ~Error() {};
  Error(const Error&) = delete;
  Error &operator=(const Error&) = delete;
  /* Create a deep copy of this error. */
  virtual Error *clone() const = 0;
  /* The location (event) where the error was discovered. */
  virtual IID<CPid> get_location() const { return loc; };
  /* A human-readable representation of the error. */
  virtual std::string to_string() const = 0;
protected:
  /* The location where the error was discovered. */
  IID<CPid> loc;
};

/* An error detected by a failing assertion. */
class AssertionError : public Error{
public:
  /* An assertion error which was discovered at loc. The failing
   * condition is described by assert_condition.
   */
  AssertionError(const IID<CPid> &loc, std::string assert_condition)
    : Error(loc), condition(assert_condition) {};
  virtual Error *clone() const;
  virtual std::string to_string() const;
private:
  /* The failed assert condition. */
  std::string condition;
};

/* An error where the phreads library was called erroneously. */
class PthreadsError : public Error{
public:
  PthreadsError(const IID<CPid> &loc, std::string msg)
    : Error(loc), msg(msg) {};
  virtual Error *clone() const;
  virtual std::string to_string() const;
private:
  std::string msg;
};

class SegmentationFaultError : public Error{
public:
  SegmentationFaultError(const IID<CPid> &loc)
    : Error(loc) {};
  virtual Error *clone() const;
  virtual std::string to_string() const;
};

/* An error where the program behaved in a non-robust manner: I.e.,
 * where there is a cycle in the Shasha-Snir trace corresponding to
 * this trace.
 *
 * loc is some event which may or may not be useful for illustrating
 * the error.
 */
class RobustnessError : public Error{
public:
  RobustnessError(const IID<CPid> &loc)
    : Error(loc) {};
  virtual Error *clone() const;
  virtual std::string to_string() const;
};

/* An error related to memory management. */
class MemoryError : public Error{
public:
  MemoryError(const IID<CPid> &loc, std::string msg)
    : Error(loc), msg(msg) {};
  virtual Error *clone() const;
  virtual std::string to_string() const;
private:
  std::string msg;
};

/* A Trace describes one execution of the program. Mainly it is a
 * sequence of events (IIDs).
 */
class Trace{
public:
  /* A Trace containing some errors.
   *
   * This object takes ownership of errors.
   */
  Trace(const std::vector<Error*> &errors, bool blocked = false);
  virtual ~Trace();
  Trace(const Trace&) = delete;
  Trace &operator=(const Trace&) = delete;
  /* The trace keeps ownership of the errors. */
  const std::vector<Error*> &get_errors() const { return errors; };
  bool has_errors() const { return errors.size(); };
  /* A multi-line, human-readable string representation of this
   * Trace. Indentation will be in multiples of ind spaces.
   */
  virtual std::string to_string(int ind = 0) const;
  /* Was the exploration of this execution (sleep set) blocked? */
  virtual bool is_blocked() const { return blocked; };
  virtual void set_blocked(bool b = true) { blocked = b; };
protected:
  std::vector<Error*> errors;
  bool blocked;

  /* Attempt to find the directory, file name and line number
   * corresponding to the metadata m.
   *
   * On success, return true and set *lineno, *file_name, *dir_name to
   * line number, file name and directory correspondingly. On failure
   * return false.
   */
  static bool get_location(const llvm::MDNode *m,
                           int *lineno,
                           std::string *file_name,
                           std::string *dir_name);
  /* Attempt to find the location (directory, file name, line number)
   * described by m, read and return the corresponding line of source
   * code.
   *
   * On success, returns the code line, whitespace stripped. On
   * failure return "".
   */
  static std::string get_src_line_verbatim(const llvm::MDNode *m);
  static std::string basename(const std::string &fname);
  static bool is_absolute_path(const std::string &fname);
};

/* This class represents traces that are expressed as sequences of
 * IIDs.
 */
class IIDSeqTrace : public Trace {
public:
  /* A Trace corresponding to the event sequence computation.
   *
   * For each i s.t. 0 <= i < computation.size(), it should be the
   * case that either computation_md[i] points to the LLVM Metadata
   * (kind "dbg") for the event computation[i], or computation_md[i]
   * is null.
   *
   * errors contains all errors that were discovered in this
   * trace. This object takes ownership of errors.
   */
  IIDSeqTrace(const std::vector<IID<CPid> > &computation,
              const std::vector<const llvm::MDNode*> &computation_md,
              const std::vector<Error*> &errors,
              bool blocked = false);
  virtual ~IIDSeqTrace();
  IIDSeqTrace(const IIDSeqTrace&) = delete;
  IIDSeqTrace &operator=(const IIDSeqTrace&) = delete;
  /* The sequence of events. */
  virtual const std::vector<IID<CPid> > &get_computation() const { return computation; };
  /* The sequence of metadata (see above). */
  virtual const std::vector<const llvm::MDNode*> &get_computation_metadata() const{
    return computation_md;
  };
  virtual std::string to_string(int ind = 0) const;
protected:
  std::vector<IID<CPid> > computation;
  std::vector<const llvm::MDNode*> computation_md;
};


#endif
