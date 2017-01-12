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

#ifndef __CHECK_MODULE_H__
#define __CHECK_MODULE_H__

#if defined(HAVE_LLVM_IR_MODULE_H)
#include <llvm/IR/Module.h>
#elif defined(HAVE_LLVM_MODULE_H)
#include <llvm/Module.h>
#endif

#include <stdexcept>
#include <string>

/* The CheckModule namespace defines several functions that can be
 * used to check that a module is correctly defined.
 *
 * The check_* functions throw CheckModuleError if the given module
 * fails the check. Otherwise, they have no side-effects.
 */

namespace CheckModule {

  class CheckModuleError : public std::exception{
  public:
    CheckModuleError(const std::string &msg) : msg("CheckModuleError: "+msg) {};
    CheckModuleError(const CheckModuleError&) = default;
    virtual ~CheckModuleError() throw() {};
    CheckModuleError &operator=(const CheckModuleError&) = default;
    virtual const char *what() const throw() { return msg.c_str(); };
  private:
    std::string msg;
  };

  /* Check that declared/defined functions in M do not clash with the
   * expected signatures of external functions that require special
   * support. (E.g. pthread_* functions, __VERIFIER_* functions, etc.)
   */
  void check_functions(const llvm::Module *M);

  /* Same check as check_functions, but only for individual functions.
   */
  void check_pthread_create(const llvm::Module *M);
  void check_pthread_join(const llvm::Module *M);
  void check_pthread_self(const llvm::Module *M);
  void check_pthread_exit(const llvm::Module *M);
  void check_pthread_mutex_init(const llvm::Module *M);
  void check_pthread_mutex_lock(const llvm::Module *M);
  void check_pthread_mutex_trylock(const llvm::Module *M);
  void check_pthread_mutex_unlock(const llvm::Module *M);
  void check_pthread_mutex_destroy(const llvm::Module *M);
  void check_pthread_cond_init(const llvm::Module *M);
  void check_pthread_cond_signal(const llvm::Module *M);
  void check_pthread_cond_broadcast(const llvm::Module *M);
  void check_pthread_cond_wait(const llvm::Module *M);
  void check_pthread_cond_destroy(const llvm::Module *M);
  void check_malloc(const llvm::Module *M);
  void check_nondet_int(const llvm::Module *M); // __VERIFIER_nondet_int, __VERIFIER_nondet_uint
  void check_assume(const llvm::Module *M); // __VERIFIER_assume

}

#endif
