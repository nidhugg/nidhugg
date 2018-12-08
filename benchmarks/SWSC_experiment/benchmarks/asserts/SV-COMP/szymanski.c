/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */

/* Adapted from:
 * Szymaski's critical section algorithm, implemented with fences.
 *
 * Adapted from: https://svn.sosy-lab.org/software/sv-benchmarks/trunk/c/pthread-atomic/szymanski_true-unreach-call.c
 */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#ifndef N
#  warning "N was not defined"
#  define N 2
#endif

// shared variables
atomic_int flag1;
atomic_int flag2;

atomic_int x;


void *p1(void *arg)
{
  int ok;
  for (int j=0; j<2; j++) {
    atomic_store_explicit(&flag1, 1, memory_order_seq_cst);
    
    ok = 0;
    for (int i=0; i<N; i++) {
      if (atomic_load_explicit(&flag2, memory_order_seq_cst) < 3) {
        ok = 1;
        break;
      }
    }
    
    if (ok==0) return NULL;
    atomic_store_explicit(&flag1, 3, memory_order_seq_cst);
    
    if (atomic_load_explicit(&flag2, memory_order_seq_cst) == 1) {
      atomic_store_explicit(&flag1, 2, memory_order_seq_cst);
      
      ok =0;
      for (int i=0; i<N; i++) {
        if (atomic_load_explicit(&flag2, memory_order_seq_cst) == 4) {
          ok = 1;
          break;
        }
      }
      
      if (ok==0) return NULL;
    }
    
    atomic_store_explicit(&flag1, 4, memory_order_seq_cst);
    
    ok = 0;
    for (int i=0; i<N; i++) {
      if (atomic_load_explicit(&flag2, memory_order_seq_cst) < 2) {
        ok = 1;
        break;
      }
    }
    
    if (ok==0) return NULL;
    // critical section
    atomic_store_explicit(&x, 1, memory_order_seq_cst);
    assert(atomic_load_explicit(&x, memory_order_seq_cst) == 1);
    
    ok = 0;
    for (int i=0; i<N; i++) {
      if (2 > atomic_load_explicit(&flag2, memory_order_seq_cst) ||
          atomic_load_explicit(&flag2, memory_order_seq_cst) > 3) {
        ok = 1;
        break;
      }
    }
    
    if (ok==0) return NULL;
    atomic_store_explicit(&flag1, 0, memory_order_seq_cst);
  }
  
  return NULL;
}

void *p2(void *arg)
{
  int ok;
  for (int j=0; j<2; j++) {
    atomic_store_explicit(&flag2, 1, memory_order_seq_cst);
    
    ok = 0;
    for (int i=0; i<N; i++) {
      if (atomic_load_explicit(&flag1, memory_order_seq_cst) < 3) {
        ok = 1;
        break;
      }
    }
    
    if (ok==0) return NULL;
    atomic_store_explicit(&flag2, 3, memory_order_seq_cst);
    
    if (atomic_load_explicit(&flag1, memory_order_seq_cst) == 1) {
      atomic_store_explicit(&flag2, 2, memory_order_seq_cst);
      
      ok =0;
      for (int i=0; i<N; i++) {
        if (atomic_load_explicit(&flag1, memory_order_seq_cst) == 4) {
          ok = 1;
          break;
        }
      }
      
      if (ok==0) return NULL;
      
    }
    
    atomic_store_explicit(&flag2, 4, memory_order_seq_cst);
    
    ok = 0;
    for (int i=0; i<N; i++) {
      if (atomic_load_explicit(&flag1, memory_order_seq_cst) < 2) {
        ok = 1;
        break;
      }
    }
    
    if (ok==0) return NULL;
    // critical section
    atomic_store_explicit(&x, 2, memory_order_seq_cst);
    assert(atomic_load_explicit(&x, memory_order_seq_cst) == 2);
    
    ok = 0;
    for (int i=0; i<N; i++) {
      if (2 > atomic_load_explicit(&flag1, memory_order_seq_cst) ||
          atomic_load_explicit(&flag1, memory_order_seq_cst) > 3) {
        ok = 1;
        break;
      }
    }
    
    if (ok==0) return NULL;
    atomic_store_explicit(&flag2, 0, memory_order_seq_cst);
  }
  
  return NULL;
}

int main(int argc, char *argv[])
{
  pthread_t a, b;
  
  atomic_init(&flag1, 0);
  atomic_init(&flag2, 0);
  atomic_init(&x, 0);
  
  pthread_create(&a, NULL, p1, NULL);
  pthread_create(&b, NULL, p2, NULL);
  
  pthread_join(a, NULL);
  pthread_join(b, NULL);
  
 	return 0;
}
