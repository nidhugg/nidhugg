// nidhuggc: -sc -optimal -unroll=3
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
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#define N 2

// shared variables
atomic_int flags[N];
atomic_int x;

void *t(void *arg)
{
    int tid = *((int *)arg);
    int ok = 0;
    int restart = 0;
  
    while(1) {
	// down
	atomic_store_explicit(&flags[tid], 0, memory_order_seq_cst);
	for (int i=0; i<tid; i++) {
	    ok = 0;
	    while(1) {
		if (atomic_load_explicit(&flags[i], memory_order_seq_cst)==0) {
		    ok = 1;
		    break;
		}
	    }
	    if (ok==0) return NULL;
	}
	// up
	atomic_store_explicit(&flags[tid], 1, memory_order_seq_cst);

	for (int i=0; i<tid; i++) {
	    ok = 0;
	    while(1) {
		if (atomic_load_explicit(&flags[i], memory_order_seq_cst)==0) {
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
	while(1) {
	    if (atomic_load_explicit(&flags[i], memory_order_seq_cst)==0) {
		ok = 1;
		break;
	    }
      	}
      	if (ok==0) return NULL;
    }
  
    // critical section
    atomic_store_explicit(&x, tid, memory_order_seq_cst);
    assert(atomic_load_explicit(&x, memory_order_seq_cst) == tid);
  
    atomic_store_explicit(&flags[tid], 0, memory_order_seq_cst);

    return NULL;
}

int arg[N];
int main(int argc, char **argv)
{
    pthread_t ts[N];

    for (int i=0; i<N; i++) {
	atomic_init(&flags[i], 0);	
    }

    atomic_init(&x, 0);
	  
    for (int i=0; i<N; i++) {
	arg[i] = i;
	pthread_create(&ts[i], NULL, t, &arg[i]);
    }
  
    for (int i=0; i<N; i++) {
	pthread_join(ts[i], NULL);
    }
  
    return 0;
}
