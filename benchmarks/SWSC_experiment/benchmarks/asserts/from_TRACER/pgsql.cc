/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */

/* Adapted from:
 * PostgreSQL bug under Power.
 *
 * URL: Listing 1.1 in
 *   https://arxiv.org/pdf/1207.7264.pdf
 */

#include <assert.h>
#include <atomic>
#include <pthread.h>

#ifndef N
#  warning "N was not defined"
#  define N 5
#endif


#define THREADS 3

// shared variables
std::atomic<int> latch[THREADS];
std::atomic<int> flag[THREADS];
std::atomic<int> x;


void *t(void *arg)
{
  for (int j=0; j<N; j++) {
    int tid = *((int *)arg);
    int ok = 0;
    
    for (int i=0; i<N; i++) {
      if (atomic_load_explicit(&latch[tid], std::memory_order_seq_cst)==1) {
        ok = 1;
        break;
      }
    }
    
    if (ok==0) return NULL;
    assert(atomic_load_explicit(&latch[tid], std::memory_order_seq_cst)==0 ||
           atomic_load_explicit(&flag[tid], std::memory_order_seq_cst)==1);
    
    atomic_store_explicit(&latch[tid], 0, std::memory_order_seq_cst);
    
    if (atomic_load_explicit(&flag[tid], std::memory_order_seq_cst)==1) {
      atomic_store_explicit(&flag[tid], 0, std::memory_order_seq_cst);
      atomic_store_explicit(&flag[(tid+1)%THREADS], 1, std::memory_order_seq_cst);
      atomic_store_explicit(&latch[(tid+1)%THREADS], 1, std::memory_order_seq_cst);
    }
  }
  
  return NULL;
  
}

int arg[THREADS];
int main(int argc, char **argv)
{
  pthread_t ts[THREADS];
  
  for (int i=1; i<THREADS; i++) {
	  	atomic_init(&latch[i], 0);
	  	atomic_init(&flag[i], 0);
  }
  atomic_init(&latch[0], 1);
  atomic_init(&flag[0], 1);
  
  for (int i=0; i<THREADS; i++) {
	  	arg[i] = i;
	  	pthread_create(&ts[i], NULL, t, &arg[i]);
  }
  
  for (int i=0; i<THREADS; i++) {
  		pthread_join(ts[i], NULL);
  }
  
  return 0;
}
