/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */

/* Example in Fig.5 in the paper 
 * https://arxiv.org/pdf/1610.01188.pdf
 * There are 2N+1 weak traces
 */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#ifndef N
#  warning "N was not defined"
#  define N 5
#endif

// shared variables
atomic_int x;


void *t1(void* arg) {
  	int tid = *((int *)arg);
  
  	for (int i=0; i<N; i++)
    	atomic_store_explicit(&x, tid, memory_order_seq_cst);
  
  	atomic_load_explicit(&x, memory_order_seq_cst);

	return NULL;

}

void *t2(void* arg) {
  	int tid = *((int *)arg);
  
  	for (int i=0; i<N; i++)
    	atomic_store_explicit(&x, tid, memory_order_seq_cst);
  
  	atomic_load_explicit(&x, memory_order_seq_cst);

	return NULL;

}

int arg[2];
int main(int argc, char **argv)
{
  	pthread_t tids[2];
  
  	arg[0]=0;
  	pthread_create(&tids[0], NULL, t1, &arg[0]);
  
  	arg[1]=1;
  	pthread_create(&tids[1], NULL, t2, &arg[1]);
  
  	for (int i = 0; i < 2; ++i) {
    	pthread_join(tids[i], NULL);
  	}
  
  	return 0;
}
