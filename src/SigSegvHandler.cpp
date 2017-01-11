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

#include "SigSegvHandler.h"

#include <csetjmp>
#include <csignal>
#include <stdexcept>

namespace SigSegvHandler {

  /* The previous signal handler, that was active before
   * setup_signal_handler.
   */
  static struct sigaction original_action;

  /* Marks the location in the stack of the last call to setenv. */
  sigjmp_buf env;

  /* Are we currently executing between calls to setenv and
   * unsetenv? */
  bool env_is_enabled = false;

  void sigsegv_handler(int signum){
    if(env_is_enabled){
      env_is_enabled = false;
      siglongjmp(env,1);
    }
  }

  void setup_signal_handler(){
    struct sigaction act;
    act.sa_handler = sigsegv_handler;
    act.sa_flags = SA_RESETHAND;
    sigemptyset(&act.sa_mask);
    if(sigaction(SIGSEGV,&act,&original_action)){
      throw std::logic_error("Failed to setup signal handler.");
    }
  }

  void reset_signal_handler(){
    sigaction(SIGSEGV,&original_action,0);
  }

  bool setenv(){
    env_is_enabled = true;
    return sigsetjmp(env,1);
  }

  void unsetenv(){
    env_is_enabled = false;
  }

}
