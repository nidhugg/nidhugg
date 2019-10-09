/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */

/* Adapted from the benchmark with the same name in
 the OOPSLA'15 paper http://plrg.eecs.uci.edu/satcheck/
 */


#include <assert.h>
#include <atomic>
#include <pthread.h>

#ifndef N
#  warning "N was not defined"
#  define N 5
#endif


std::atomic<bool> flag0, flag1;
std::atomic<int> turn;
std::atomic<int> var;

extern "C" {
  void __VERIFIER_assume(int b);
}

void p0() {
  flag0.store(true, std::memory_order_seq_cst);
  
  while (flag1.load(std::memory_order_seq_cst)) {
    if (turn.load(std::memory_order_seq_cst) !=0)
    {
      flag0.store(false, std::memory_order_seq_cst);
      while (turn.load(std::memory_order_seq_cst) != 0) {
        __VERIFIER_assume(0);
      }
      flag0.store(true, std::memory_order_seq_cst);
    }
    else
      __VERIFIER_assume(0);
  }
  
  // critical section
  var.store(1, std::memory_order_seq_cst);
  assert(var.load(std::memory_order_seq_cst) == 1);
  
  turn.store(1, std::memory_order_seq_cst);
  flag0.store(false, std::memory_order_seq_cst);
}

void p1() {
  flag1.store(true, std::memory_order_seq_cst);
  
  while (flag0.load(std::memory_order_seq_cst)) {
    if (turn.load(std::memory_order_seq_cst) != 1) {
      flag1.store(false, std::memory_order_seq_cst);
      while (turn.load(std::memory_order_seq_cst) != 1) {
        __VERIFIER_assume(0);
      }
      flag1.store(true, std::memory_order_seq_cst);
    }
    else
      __VERIFIER_assume(0);
  }
  
  // critical section
  var.store(2, std::memory_order_seq_cst);
  assert(var.load(std::memory_order_seq_cst) == 2);
  
  turn.store(0, std::memory_order_seq_cst);
  flag1.store(false, std::memory_order_seq_cst);
}

void * p0l(void *arg) {
  // parameterized code
  for(int i = 0; i < N; i++) {
    p0();
  }
  
  return NULL;
}

void * p1l(void *arg) {
  // parameterized code
  for(int i = 0; i < N; i++) {
    p1();
  }
  
  return NULL;
}

int main(int argc, char **argv)
{
  pthread_t a, b;
  
  atomic_init(&flag0, false);
  atomic_init(&flag1, false);
  atomic_init(&turn, 0);
  atomic_init(&var, 0);
  
  pthread_create(&a, 0, p0l, NULL);
  pthread_create(&b, 0, p1l, NULL);
  
  pthread_join(a, 0);
  pthread_join(b, 0);
  
  return 0;
}
