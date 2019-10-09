/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */

/* Adapted from filesystem example in PLDI paper
   https://dl.acm.org/citation.cfm?id=1040315
 */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#define NUMBLOCKS  26
#define NUMINODE  32

#ifndef N
#  warning "N was not defined"
#  define N 16
#endif

#define LOOP 10

pthread_mutex_t  locki[NUMINODE];
atomic_int inode[NUMINODE];
pthread_mutex_t  lockb[NUMBLOCKS];
atomic_int busy[NUMBLOCKS];

pthread_t  tids[N];

// thread code
void *thread_routine(void * arg)
{
  	int tid;
  	int i, b;
  	tid = *((int *)arg);
  
  	i = tid % NUMINODE;
  	pthread_mutex_lock(&locki[i]);
  	if (atomic_load_explicit(&inode[i], memory_order_seq_cst)==0) {
    	b = (i*2) % NUMBLOCKS;
    	for (int j=0; j<LOOP; j++) { // using loop instead of while
			pthread_mutex_lock(&lockb[b]);
      		if (!atomic_load_explicit(&busy[b], memory_order_seq_cst)) {
        		atomic_store_explicit(&busy[b], 1, memory_order_seq_cst);
        		atomic_store_explicit(&inode[i], b+1, memory_order_seq_cst);
        		pthread_mutex_unlock(&lockb[b]);
        		break;
      		}
			pthread_mutex_unlock(&lockb[b]);
     	 	b = (b+1) % NUMBLOCKS;
    	}
  	}
	pthread_mutex_unlock(&locki[i]);

	return NULL;
}

int arg[N];
int main(int argc, char *argv[])
{
  	int i;

  	// init
  	for (i = 0; i < NUMINODE; i++) {
    	pthread_mutex_init (&locki[i], NULL);
    	atomic_init(&inode[i], 0);
  	}
  
  	// init
  	for (i = 0; i < NUMBLOCKS; i++) {
		pthread_mutex_init (&lockb[i], NULL);
		atomic_init(&busy[i], 0);
  	}

  	// create threads
  	for (i = 0; i < N; i++){
    	arg[i] = i;
    	pthread_create(&tids[i], NULL, thread_routine, &arg[i]);
  	}


  	for (i = 0; i < N; i++){
    	pthread_join(tids[i], NULL);
  	} 


  	return 0;
}


