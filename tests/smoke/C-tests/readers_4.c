/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC
 */

#include <assert.h>
#include <stdint.h>
#include <pthread.h>


#define N 4

// shared variables
volatile int x;
volatile args[N+1];

void *thread_writer(void *unused)
{
	x = 42;
	return NULL;
}

void *thread_reader(void *arg)
{
	int r = *((int *) arg);
	int a = x;
	return NULL;
}

int main(int argc, char *argv[])
{
	pthread_t t[N+1];

	x = 0;
	for (int i = 0; i <= N; i++)
		args[i] = i;

	for (int i = 0; i <= N; i++) {
		if (i == 0)
			pthread_create(&t[i], NULL, thread_writer, NULL);
		else
			pthread_create(&t[i], NULL, thread_reader, &args[i-1]);
	}

	for (int i = 0; i <= N; i++) {
		pthread_join(t[i], NULL);
	}

	return 0;
}
