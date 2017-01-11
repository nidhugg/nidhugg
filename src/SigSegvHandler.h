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

#ifndef __SIGSEGV_HANDLER_H__
#define __SIGSEGV_HANDLER_H__

/* The SigSegvHandler namespace provides functions for setting up and
 * using the signal handler for detecting segmentation faults.
 */
namespace SigSegvHandler {

  /* Install the signal handler associated with this namespace. Stores
   * the previous signal handler.
   *
   * This function should be called before setenv/unsetenv.
   */
  void setup_signal_handler();

  /* Uninstall the signal handler associated with this
   * namespace. Restore the previous signal handler.
   */
  void reset_signal_handler();

  /* Use setenv/unsetenv to execute code while checking for
   * segmentation faults. The usage should follow this pattern:
   *
   * if(setenv()){
   *   // There was a segmentation fault.
   *   // Handle it.
   * }else{
   *   // Run code here that should be checked for
   *   // segmentation faults.
   * }
   * unsetenv();
   */
  bool setenv();
  void unsetenv();

}

#endif
