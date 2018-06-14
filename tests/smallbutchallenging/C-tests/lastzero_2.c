/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC
 */

/* From Optimal dynamic partial order reduction, POPL'14 
 * https://dl.acm.org/citation.cfm?id=2535845
 */

#include <assert.h>
#include <stdint.h>
#include <pthread.h>


#define N 2

volatile int array[N+1];
int args[N+1];

void *thread_reader(void *unused)
{
	for (int i = N; array[i] != 0; i--) ;
	return NULL;
}

void *thread_writer(void *arg)
{
	int j = *((int *) arg);
	
	int a = array[j - 1]; 
	array[j] = a + 1;

	return NULL;
}


int main(int argc, char *argv[])
{
	pthread_t t[N+1];

	for (int i =0; i <= N; i++)
		array[i] = 0;

	for (int i = 0; i <= N; i++) {
		args[i] = i;
		if (i == 0) {
			pthread_create(&t[i], NULL, thread_reader, NULL);
		} else {
			pthread_create(&t[i], NULL, thread_writer, &args[i]);
		}
	}

	return 0;
}
