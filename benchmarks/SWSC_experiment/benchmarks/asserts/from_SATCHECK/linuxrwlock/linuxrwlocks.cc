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


#define RW_LOCK_BIAS            0x00100000
#define WRITE_LOCK_CMP          RW_LOCK_BIAS


typedef union {
  std::atomic<int> lock;
} rwlock_t;

rwlock_t mylock;
std::atomic<int> var;


static inline int write_trylock(rwlock_t *rw)
{
  int priorvalue = atomic_fetch_sub(&rw->lock, RW_LOCK_BIAS);
  if (priorvalue == RW_LOCK_BIAS)
    return 1;
  
  atomic_fetch_add(&rw->lock, RW_LOCK_BIAS);
  return 0;
}


static inline void write_unlock(rwlock_t *rw)
{
  atomic_fetch_add(&rw->lock, RW_LOCK_BIAS);
}


void * a(void *obj) {
  // parameterized code
  for(int i = 0; i < N; i++) {
    if (write_trylock(&mylock)) {
      // critical section
      var.store(1, std::memory_order_seq_cst);
      assert(var.load(std::memory_order_seq_cst) == 1);
      write_unlock(&mylock);
    }
  }
  return NULL;
}

void * b(void *obj) {
  // parameterized code
  for(int i = 0; i < N; i++) {
    if (write_trylock(&mylock)) {
      // critical section
      var.store(2, std::memory_order_seq_cst);
      assert(var.load(std::memory_order_seq_cst) == 2);
      write_unlock(&mylock);
    }
  }
  return NULL;
}


int main(int argc, char **argv)
{
  pthread_t t1, t2;
  mylock.lock = RW_LOCK_BIAS;
  var=0;
  
  pthread_create(&t1, 0, &a, NULL);
  pthread_create(&t2, 0, &b, NULL);
  
  pthread_join(t1, 0);
  pthread_join(t2, 0);
  
  return 0;
}
