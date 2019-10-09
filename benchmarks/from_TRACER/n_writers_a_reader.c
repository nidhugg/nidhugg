/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */

/* Adaptd from:
 * CDSChecker example 
 * https://dl.acm.org/citation.cfm?id=2914585.2806886, page 10, section 5.4
 * There are N+1 weak traces
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

void *writer(void *arg) {
	atomic_store_explicit(&x, 1, memory_order_seq_cst);
	return NULL;
}



void *reader(void *arg) {
	atomic_load_explicit(&x, memory_order_seq_cst);
	return NULL;
}




int main(int argc, char **argv)
{
	pthread_t ws[N], r;

	atomic_init(&x, 0);

	for (int i=0; i<N; i++) {
		pthread_create(&ws[i], NULL, writer, NULL);
	}

	pthread_create(&r, NULL, reader, NULL);

	return 0;
}
