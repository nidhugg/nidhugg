/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */

/* Adapted from: 
 * Peterson's critical section algorithm, implemented with fences.
 *
 * Adapted from: https://svn.sosy-lab.org/software/sv-benchmarks/trunk/c/pthread-atomic/peterson_true-unreach-call.c
 */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#define LOOP 2

atomic_int flag0;
atomic_int flag1;
atomic_int turn;

atomic_int x; // to avoid race

void *p0(void *arg)
{
  	for (int j=0; j<LOOP; j++) {
    	atomic_store_explicit(&flag0, 1, memory_order_seq_cst);
    	atomic_store_explicit(&turn, 1, memory_order_seq_cst);

    	int ok = 0;
    	for (int i=0; i<LOOP; i++) { 
      		int _flag1 = atomic_load_explicit(&flag1, memory_order_seq_cst);
      		int _turn =  atomic_load_explicit(&turn, memory_order_seq_cst);
      		if ( _flag1 != 1 || _turn != 1) {
        		ok = 1;	
        		break;
      		} 
    	}
    
		if (ok==0) return NULL;
    	// critical section
    	atomic_store_explicit(&x, 1, memory_order_seq_cst);
    	assert(atomic_load_explicit(&x, memory_order_seq_cst) == 1);

    	atomic_store_explicit(&flag0, 0, memory_order_seq_cst);
  	}

	return NULL;
}

void *p1(void *arg)
{
  	for (int j=0; j<LOOP; j++) {
    	atomic_store_explicit(&flag1, 1, memory_order_seq_cst);
    	atomic_store_explicit(&turn, 0, memory_order_seq_cst);

    	int ok = 0;
    	for (int i=0; i<LOOP; i++) {
      		int _flag0 = atomic_load_explicit(&flag0, memory_order_seq_cst);
      		int _turn =  atomic_load_explicit(&turn, memory_order_seq_cst);
      		if (_flag0 != 1 || _turn != 0) {
        		ok = 1;
        		break;		
      		} 
    	}
    
		if (ok==0) return NULL; 

    	// critical section
    	atomic_store_explicit(&x, 2, memory_order_seq_cst);
    	assert(atomic_load_explicit(&x, memory_order_seq_cst) == 2);

    	atomic_store_explicit(&flag1, 0, memory_order_seq_cst);
  	}
	
	return NULL;
}

int main(int argc, char **argv)
{
 	pthread_t a, b;

 	atomic_init(&flag0, 0);
 	atomic_init(&flag1, 0);
 	atomic_init(&turn, 0);
 	atomic_init(&x, 0);

 	pthread_create(&a, NULL, p0, NULL);
 	pthread_create(&b, NULL, p1, NULL);

 	pthread_join(a, NULL);
  	pthread_join(b, NULL);

 	return 0;
}
