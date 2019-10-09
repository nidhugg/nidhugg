/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */

/* Adapted from:
 * Lamport's Bakery critical section algorithm, implemented with fences.
 *
 * Adapted from the benchmark in the URL:
 *   https://www.geeksforgeeks.org/operating-system-bakery-algorithm/
 */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#define LOOP 5
#define THREAD_COUNT 2

// shared variables
atomic_int tickets[THREAD_COUNT];
atomic_int choosing[THREAD_COUNT];
atomic_int resource;

int lock(int thread) {
  	int ok = 0;
 	int max_ticket = 0;

	atomic_store_explicit(&choosing[thread], 1, memory_order_seq_cst); 
	 
  	for (int i = 0; i < THREAD_COUNT; ++i) {
    	int ticket = atomic_load_explicit(&tickets[i], memory_order_seq_cst);
    	max_ticket = ticket > max_ticket ? ticket : max_ticket;
  	}
  
	atomic_store_explicit(&tickets[thread], max_ticket + 1, memory_order_seq_cst);
  	atomic_store_explicit(&choosing[thread], 0, memory_order_seq_cst);
  
	for (int other = 0; other < THREAD_COUNT; ++other) {
    	ok = 0;
    	for (int i=0; i<LOOP; i++) {
      		if (!atomic_load_explicit(&choosing[other], memory_order_seq_cst)) {
        		ok = 1;
        		break;
      		}
    	}
    
    	if (ok==0) return 0;
    	else {
      		ok = 0;
      		for (int i=0; i<LOOP; i++) {
        		int other_ticket = atomic_load_explicit(&tickets[other], memory_order_seq_cst);
        		int thread_ticket = atomic_load_explicit(&tickets[thread], memory_order_seq_cst);
        		if (!(other_ticket != 0 &&
              		(other_ticket < thread_ticket || (other_ticket == thread_ticket && other < thread)))) {
                	ok = 1;
                	break;
              	}
      		}
      
			if (ok==0) return 0;
    	}
  	}
  	
	return 1;
}

void unlock(int thread) {
  	atomic_store_explicit(&tickets[thread], 0, memory_order_seq_cst);
}

void use_resource(int thread) {
  	if (atomic_load_explicit(&resource, memory_order_seq_cst) != 0) {
    	assert(0); // raise mutex assertion violation
  	}
  	atomic_store_explicit(&resource, thread, memory_order_seq_cst);
  	atomic_store_explicit(&resource, 0, memory_order_seq_cst);
}

void *thread_body(void *arg) {
  	int thread = *((int *)arg);
  	if (lock(thread)) {
    	use_resource(thread);
    	unlock(thread);
  	}
	return NULL;
}

int arg[THREAD_COUNT];
int main(int argc, char *argv[]) {
  	pthread_t threads[THREAD_COUNT];

  	for (int i=0; i<THREAD_COUNT; i++) {
    	atomic_init(&tickets[i], 0);
    	atomic_init(&choosing[i], 0);
  	}
	
  	atomic_init(&resource, 0);
  
  	for (int i = 0; i < THREAD_COUNT; ++i) {
    	arg[i]=i;
    	pthread_create(&threads[i], NULL, thread_body, &arg[i]);
  	}
  
  	for (int i = 0; i < THREAD_COUNT; ++i) {
    	pthread_join(threads[i], NULL);
  	}
  
  	return 0;
}
