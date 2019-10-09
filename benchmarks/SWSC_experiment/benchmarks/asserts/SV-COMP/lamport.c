/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */

/* Adapted from: https://svn.sosy-lab.org/software/sv-benchmarks/trunk/c/pthread-atomic/lamport_true-unreach-call.c */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#ifndef N
#  warning "N was not defined"
#  define N 2
#endif

// shared variables
atomic_int x, y;
atomic_int b1, b2;

atomic_int var;


void *p1(void *arg)
{
  int ok;
  for (int jj=0; jj<2; jj++) {
    ok = 0;
    
    for (int i=0; i<N; i++) {
      atomic_store_explicit(&b1, 1, memory_order_seq_cst);
      atomic_store_explicit(&x, 1, memory_order_seq_cst);
      
      if (atomic_load_explicit(&y, memory_order_seq_cst) != 0) {
        atomic_store_explicit(&b1, 0, memory_order_seq_cst);
        
        for (int i=0; i<N; i++) {
          if (atomic_load_explicit(&y, memory_order_seq_cst) == 0)
            goto breaklbl0;
        }
        
        goto breaklbl;
      breaklbl0:;
        continue;
      }
      
      atomic_store_explicit(&y, 1, memory_order_seq_cst);
      
      if (atomic_load_explicit(&x, memory_order_seq_cst) != 1) {
        atomic_store_explicit(&b1, 0, memory_order_seq_cst);
        
        for (int i=0; i<N; i++) {
          if (atomic_load_explicit(&b2, memory_order_seq_cst) < 1)
            goto breaklbl1;
        }
        
        goto breaklbl;
      breaklbl1:;
        
        if (atomic_load_explicit(&y, memory_order_seq_cst) != 1) {
          for (int i=0; i<N; i++) {
            if (atomic_load_explicit(&y, memory_order_seq_cst) == 0)
              goto breaklbl2;
          }
          
          goto breaklbl;
        breaklbl2:;
          continue;
        }
      }
      
      ok = 1;
      goto breaklbl;
    }
    
  breaklbl:;
    if (ok==0) return NULL;
    
    // critical section
    atomic_store_explicit(&var, 1, memory_order_seq_cst);
    assert(atomic_load_explicit(&var, memory_order_seq_cst) == 1);
    
    atomic_store_explicit(&y, 0, memory_order_seq_cst);
    atomic_store_explicit(&b1, 0, memory_order_seq_cst);
  }
  
  return NULL;
  
}

void *p2(void *arg)
{
  int ok;
  for (int jj=0; jj<2; jj++) {
    ok = 0;
    
    for (int i=0; i<N; i++) {
      atomic_store_explicit(&b2, 1, memory_order_seq_cst);
      atomic_store_explicit(&x, 2, memory_order_seq_cst);
      
      if (atomic_load_explicit(&y, memory_order_seq_cst) != 0) {
        atomic_store_explicit(&b2, 0, memory_order_seq_cst);
        
        for (int i=0; i<N; i++) {
          if (atomic_load_explicit(&y, memory_order_seq_cst) == 0)
            goto breaklbl0;
        }
        
        goto breaklbl;
      breaklbl0:;
        continue;
        
      }
      
      atomic_store_explicit(&y, 2, memory_order_seq_cst);
      
      if (atomic_load_explicit(&x, memory_order_seq_cst) != 2) {
        atomic_store_explicit(&b2, 0, memory_order_seq_cst);
        
        for (int i=0; i<N; i++) {
          if (atomic_load_explicit(&b1, memory_order_seq_cst) < 1)
            goto breaklbl1;
        }
        
        goto breaklbl;
      breaklbl1:;
        
        if (atomic_load_explicit(&y, memory_order_seq_cst) != 2) {
          for (int i=0; i<N; i++) {
            if (atomic_load_explicit(&y, memory_order_seq_cst) == 0)
              goto breaklbl2;
          }
          
          goto breaklbl;
        breaklbl2:;
          continue;
        }
      }
      
      ok = 1;
      goto breaklbl;
    }
    
  breaklbl:;
    if (ok==0) return NULL;
    
    // critical section
    atomic_store_explicit(&var, 2, memory_order_seq_cst);
    assert(atomic_load_explicit(&var, memory_order_seq_cst) == 2);
    
    atomic_store_explicit(&y, 0, memory_order_seq_cst);
    atomic_store_explicit(&b2, 0, memory_order_seq_cst);
  }
  
  return NULL;
}

int main(int argc, char *argv[])
{
  pthread_t a, b;
  
  atomic_init(&x, 0);
  atomic_init(&y, 0);
  atomic_init(&b1, 0);
  atomic_init(&b2, 0);
  atomic_init(&var, 0);
  
  pthread_create(&a, NULL, p1, NULL);
  pthread_create(&b, NULL, p2, NULL);
  
  pthread_join(a, NULL);
  pthread_join(b, NULL);
  
  return 0;
}
