/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */

/* Adapted from the benchmark with the same name in
 the OOPSLA'15 paper http://plrg.eecs.uci.edu/satcheck/
 */



#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <atomic>



#ifndef N
#  warning "N was not defined"
#  define N 5
#endif

typedef union {
  std::atomic<int> lock;
} lock_t;

lock_t mylock;
std::atomic<int> var;

static inline bool write_trylock(lock_t *rw) {
  int oldvalue=0;
  return rw->lock.compare_exchange_strong(oldvalue, 1,
                                          std::memory_order_seq_cst,
                                          std::memory_order_seq_cst);
}


static inline void write_unlock(lock_t *rw)
{
  atomic_store_explicit(&rw->lock, 0, std::memory_order_seq_cst);
}


static void fooa() {
  bool flag=write_trylock(&mylock);
  if (flag) {
    // critical section
    var.store(1, std::memory_order_seq_cst);
    assert(var.load(std::memory_order_seq_cst) == 1);
    write_unlock(&mylock);
  }
}

static void foob() {
  bool flag=write_trylock(&mylock);
  if (flag) {
    // critical section
    var.store(2, std::memory_order_seq_cst);
    assert(var.load(std::memory_order_seq_cst) == 2);
    write_unlock(&mylock);
  }
}

void * a(void *obj)
{
  // parameterized code
  for(int i = 0; i < N; i++) {
    fooa();
  }
  
  return NULL;
}

void * b(void *obj)
{
  // parameterized code
  for(int i = 0; i < N; i++) {
    foob();
  }
  
  return NULL;
}

int main(int argc, char **argv)
{
  pthread_t t1, t2;
  mylock.lock=0;
  var=0;
  
  pthread_create(&t1, 0, a, NULL);
  pthread_create(&t2, 0, b, NULL);
  
  pthread_join(t1, 0);
  pthread_join(t2, 0);
  
  return 0;
}
