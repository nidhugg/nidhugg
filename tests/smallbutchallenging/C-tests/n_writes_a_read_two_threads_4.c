/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC
 */

/* Example in Fig.5 in the paper 
 * https://arxiv.org/pdf/1610.01188.pdf
 * 2N+1 weak traces
 */

#include <assert.h>
#include <stdint.h>
#include <pthread.h>


#define N 4

// shared variables
volatile int x;
int args[2];

void *thread_routine(void* arg) {
  int tid = *((int *)arg);
  
  for (int i = 0; i < N; i++) x = tid;
  int a = x;
}

int main(int argc, char *argv[])
{
  	pthread_t ts[2];
  	x = 0;
  
  	for (int i = 0; i < 2; i++) {
 		args[i] = i;
		pthread_create(&ts[i], NULL, thread_routine, &args[i]);
 	}
  
  	for (int i = 0; i < 2; i++) {
    	pthread_join(ts[i], NULL);
  	}
  
  	return 0;
}
