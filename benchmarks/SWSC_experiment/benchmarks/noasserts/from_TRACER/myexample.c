/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */

/* Example in Fig.1 in our OOPSLA paper */

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

void *t(void* arg) {
  	int tid = *((int *)arg);
  	atomic_store_explicit(&x, tid, memory_order_seq_cst);
  	atomic_load_explicit(&x, memory_order_seq_cst);
	return NULL;
}

int arg[N];
int main(int argc, char *argv[])
{
  pthread_t tids[N];
  
  for (int i = 0; i < N; ++i) {
    arg[i] = i;
    pthread_create(&tids[i], NULL, t, &arg[i]);
  }
  
  for (int i = 0; i < N; ++i) {
    pthread_join(tids[i], NULL);
  }
  
  return 0;
}
