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
#ifndef __TIMING_H__
#define __TIMING_H__

#ifdef NO_TIMING

namespace Timing {
  class Context {
    Context(Context &) = delete;
    Context & operator =(Context &other) = delete;
  public:
    Context(std::string name) {}
  };
  class Guard {
  public:
    Guard(Context &) {}
    Guard(Guard &) = delete;
    Guard & operator =(Guard &other) = delete;
  };

  constexpr bool timing_enabled() { return false; }
}  // namespace Timing

#else /* defined(NO_TIMING) */

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <pthread.h>
#include <string>

namespace Timing {
  typedef std::chrono::steady_clock clock;

  namespace impl {
    template<class T>
    class tls_ptr {
      pthread_key_t key;
      static void destructor(void *ptr) noexcept {};
    public:
      tls_ptr() { if (pthread_key_create(&key, destructor)) abort(); }
      ~tls_ptr() { pthread_key_delete(key); }
      T* get() const { return (T*)pthread_getspecific(key); }
      void set(T* val) { if (pthread_setspecific(key, val)) abort(); }
    };

    extern bool is_enabled;
  }  // namespace impl

  class Context {
    Context(Context &) = delete;
    Context & operator =(Context &other) = delete;
  public:
    Context(std::string name);
    ~Context();
    std::string name;
    Context *next;
    struct Thread {
      Thread();
      clock::duration inclusive, exclusive;
      uint_fast64_t count;
      Thread *next;
    };
    std::atomic<Thread*> first_thread;
    impl::tls_ptr<Thread> my_thread;
    Thread *get_thread();
  };

  class Guard {
  public:
    Guard(Context &context) : subcontext_time(0) {
      if (impl::is_enabled) begin(&context);
    }
    Guard(Guard &) = delete;
    Guard & operator =(Guard &other) = delete;
    ~Guard() {
      if (impl::is_enabled) end();
    }
  private:
    void begin(Context *c);
    void end();
    friend class Context;
    Context *context;
    Guard *outer_scope;
    clock::duration subcontext_time;
    clock::time_point start;
  };

  void print_report();
  inline bool timing_enabled() { return impl::is_enabled; }
}  // namespace Timing

#endif /* !defined(NO_TIMING) */

#endif /* !defined(__TIMING_H__) */
