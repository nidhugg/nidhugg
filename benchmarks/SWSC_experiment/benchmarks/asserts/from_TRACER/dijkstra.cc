/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */

/* Adapted from:
 * Dijkstra's critical section algorithm, implemented with fences.
 *
 * URL:
 *   https://www.eecs.yorku.ca/course_archive/2007-08/W/6117/DijMutexNotes.pdf
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
std::atomic<int> interested[THREADS];
std::atomic<int> passed[THREADS];
std::atomic<int> k;
std::atomic<int> x;

void *t(void *arg)
{
  int ok = 0;
  int done, _k;
 	int tid = *((int *)arg);
  
  atomic_store_explicit(&interested[tid], 1, std::memory_order_seq_cst);
  
  done = 0;
  for (int jj=0; jj<N; jj++) {
  		for (int j=0; j<N; j++) {
        _k = atomic_load_explicit(&k, std::memory_order_seq_cst);
        if (_k==tid) {
          ok = 1;
          break;
        }
        if (atomic_load_explicit(&interested[_k], std::memory_order_seq_cst)==0)
          atomic_store_explicit(&k, tid, std::memory_order_seq_cst);
      }
    
	  	if (ok==0) return NULL;
	  	atomic_store_explicit(&passed[tid], 1, std::memory_order_seq_cst);
    
	  	done = 1;
	  	for (int j=0; j<THREADS; j++) {
        if (j==tid) continue;
        if (atomic_load_explicit(&passed[j], std::memory_order_seq_cst)==1) {
          atomic_store_explicit(&passed[tid], 0, std::memory_order_seq_cst);
          done = 0;
        }
      }
    
	  	if (done==1) {
        ok = 1;
        break;
      } else
        ok = 0;
  }
  
  if (ok==0) return NULL;
  
  // critical section
  atomic_store_explicit(&x, tid, std::memory_order_seq_cst);
  assert(atomic_load_explicit(&x, std::memory_order_seq_cst) == tid);
  
  atomic_store_explicit(&passed[tid], 0, std::memory_order_seq_cst);
  atomic_store_explicit(&interested[tid], 0, std::memory_order_seq_cst);
  return NULL;
}

int arg[THREADS];

int main(int argc, char **argv)
{
  pthread_t ts[THREADS];
  
  for (int i=0; i<THREADS; i++) {
	  	atomic_init(&interested[i], 0);
	  	atomic_init(&passed[i], 0);
  }
  atomic_init(&k, 0);
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
