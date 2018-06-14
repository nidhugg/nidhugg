/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC
 */

/* My example: Get insprration from example in Figure 5, 
 * Data-centric dynamic partial order reduction
 *  https://dl.acm.org/citation.cfm?id=3158119
 * There are 3N^2+3N+1 weak traces
 */

#include <assert.h>
#include <stdint.h>
#include <pthread.h>


#define N 3

// shared variables
volatile int x;

void *thread1(void *arg) {
	for (int i=0; i<N; i++) x = 1;
	return NULL;
}

void *thread2(void *arg) {
	for (int i=0; i<N; i++) x = 2;
	return NULL;
}

void *thread3(void *arg) {
	int a = x;
	int b = x;
	return NULL;
}

int main(int argc, char **argv)
{
	pthread_t t1, t2, t3;
	x = 0;

	pthread_create(&t1, NULL, thread1, NULL);
	pthread_create(&t2, NULL, thread2, NULL);
	pthread_create(&t3, NULL, thread3, NULL);
	
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	pthread_join(t3, NULL);


	return 0;
}
