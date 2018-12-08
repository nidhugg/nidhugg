/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */

/* Adapted from:
 * Burn's critical section algorithm, implemented with fences.
 *
 * URL:
 *   http://www.cs.yale.edu/homes/aspnes/pinewiki/attachments/MutualExclusion/burns-lynch-1993.pdf
 */

#include <assert.h>
#include <atomic>
#include <pthread.h>

#ifndef N
#  warning "N was not defined"
#  define N 5
#endif

#define THREADS 2

// shared variables
std::atomic<int> flags[N];
std::atomic<int> x;

void *t(void *arg)
{
  int tid = *((int *)arg);
  int ok = 0;
  int restart = 0;
  
  for (int j=0; j<N; j++) {
    // down
    atomic_store_explicit(&flags[tid], 0, std::memory_order_seq_cst);
    for (int i=0; i<tid; i++) {
      ok = 0;
      for (int jj=0; jj<N; jj++) {
        if (atomic_load_explicit(&flags[i], std::memory_order_seq_cst)==0) {
          ok = 1;
          break;
        }
      }
      if (ok==0) return NULL;
    }
    // up
    atomic_store_explicit(&flags[tid], 1, std::memory_order_seq_cst);
    
	  	for (int i=0; i<tid; i++) {
        ok = 0;
        for (int jj=0; jj<N; jj++) {
          if (atomic_load_explicit(&flags[i], std::memory_order_seq_cst)==0) {
            ok = 1;
            break;
          }
        }
        if (ok==0) {
          restart = 1;
          break;
        };
      }
    if (restart==0) {
	    	ok = 1;
	    	break;
    } else ok = 0;
  }
  
  if (ok==0) return NULL;
  
  
  for (int i=tid+1; i<N; i++) {
    ok = 0;
    for (int jj=0; jj<N; jj++) {
      if (atomic_load_explicit(&flags[i], std::memory_order_seq_cst)==0) {
        ok = 1;
        break;
      }
    }
    if (ok==0) return NULL;
  }
  
  // critical section
  atomic_store_explicit(&x, tid, std::memory_order_seq_cst);
  assert(atomic_load_explicit(&x, std::memory_order_seq_cst) == tid);
  
  atomic_store_explicit(&flags[tid], 0, std::memory_order_seq_cst);
  
  return NULL;
}

int arg[THREADS];
int main(int argc, char **argv)
{
  pthread_t ts[THREADS];
  
  for (int i=0; i<THREADS; i++) {
	  	atomic_init(&flags[i], 0);
  }
  
  atomic_init(&x, 0);
  
  for (int i=0; i<THREADS; i++) {
	  	arg[i] = i;
	  	pthread_create(&ts[i], NULL, t, &arg[i]);
  }
  
  for (int i=0; i<THREADS; i++) {
  		pthread_join(ts[i], NULL);
  }
  
  return 0;
}
