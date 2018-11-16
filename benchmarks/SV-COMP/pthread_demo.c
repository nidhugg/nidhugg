/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */

/* Adapted from: https://github.com/sosy-lab/sv-benchmarks/blob/master/c/pthread-C-DAC/pthread-demo-datarace_true-unreach-call.c */


#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#ifndef N
# warning "N is not defined, assuming 2"
# define N 2
#endif

#define LOOP 5

// shared variables
atomic_int  myglobal; 
pthread_mutex_t mymutex;

void *thread_function_mutex(void* arg)
{
  for (int i=0; i<LOOP; i++ )
  {
    pthread_mutex_lock(&mymutex);
    int j = atomic_load_explicit(&myglobal, memory_order_seq_cst);
	j = j + 1;
	atomic_store_explicit(&myglobal, j, memory_order_seq_cst);
	pthread_mutex_unlock(&mymutex);
  }
  
  return NULL;
}

int main(int argc, char *argv[])
{
	pthread_t threads[N];
	
	pthread_mutex_init (&mymutex, NULL);	
	atomic_init(&myglobal, 0);

	for (int i=0; i<N; i++)
    	pthread_create(&threads[i], NULL, thread_function_mutex, NULL);
  	for (int i=0; i<N; i++)
  		pthread_join(threads[i], NULL);
	
	return 0;
}
