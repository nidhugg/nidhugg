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
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#define LOOP 5
#define N 5

// shared variables
atomic_int latch[N];
atomic_int flag[N];
atomic_int x;


void *t(void *arg)
{
  	int tid = *((int *)arg);
  	int ok = 0;
  
    for (int i=0; i<LOOP; i++) {
    	if (atomic_load_explicit(&latch[tid], memory_order_seq_cst)==1) {
    		ok = 1;
    		break;
    	}    
    }
    
    if (ok==0) return NULL;
	assert(atomic_load_explicit(&latch[tid], memory_order_seq_cst)==0 ||
			atomic_load_explicit(&flag[tid], memory_order_seq_cst)==1);

	atomic_store_explicit(&latch[tid], 0, memory_order_seq_cst);

	if (atomic_load_explicit(&flag[tid], memory_order_seq_cst)==1) {
		atomic_store_explicit(&flag[tid], 0, memory_order_seq_cst);
		atomic_store_explicit(&flag[(tid+1)%N], 1, memory_order_seq_cst);
		atomic_store_explicit(&latch[(tid+1)%N], 1, memory_order_seq_cst);	
	} 

	return NULL;
  
}

int arg[N];
int main(int argc, char **argv)
{
  	pthread_t ts[N];
  	
	for (int i=1; i<N; i++) {
	  	atomic_init(&latch[i], 0);
	  	atomic_init(&flag[i], 0);	
	}
	atomic_init(&latch[0], 1);
	atomic_init(&flag[0], 1);
	  
	for (int i=0; i<N; i++) {
	  	arg[i] = i;
	  	pthread_create(&ts[i], NULL, t, &arg[i]);
	}
  
  	for (int i=0; i<N; i++) {
  		pthread_join(ts[i], NULL);
  	}
  
  	return 0;
}
