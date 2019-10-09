/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */

/* There are N weak traces */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#ifndef N
#  warning "N was not defined"
#  define N 5
#endif

atomic_int vars[1];

void *t(void *arg){
  	int tid = *((int *)arg);
  	atomic_store_explicit(&vars[0], tid, memory_order_seq_cst);
  	atomic_store_explicit(&vars[0], tid, memory_order_seq_cst);

	return NULL;
}

int arg[N];
int main(int argc, char *argv[]){
  	pthread_t ts[N];

  	atomic_init(&vars[0], 0);
  
  	for (int i=0; i<N; i++) {
   		arg[i]=i;
    	pthread_create(&ts[i], NULL, t, &arg[i]);
  	}
  
  	for (int i=0; i<N; i++)
    	pthread_join(ts[i], NULL);
  
  	int v1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  	int v2 = (v1 == N);
  	if (v2 == 1) assert(0);

  	return 0;
}
