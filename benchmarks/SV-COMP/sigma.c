/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC
 */

/* Adapted from: https://raw.githubusercontent.com/sosy-lab/sv-benchmarks/master/c/pthread/sigma_false-unreach-call.c */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#ifndef SIGMA
#  warning "SIGMA was not defined"
#  define SIGMA 5
#endif

// shared variables
atomic_int array[SIGMA]; 
atomic_int array_index;

void *thread(void *arg) {
	int _array_index;
	_array_index = atomic_load_explicit(&array_index, memory_order_seq_cst);
	atomic_store_explicit(&array[_array_index], 1, memory_order_seq_cst);
	
	return NULL;
}



int main(int argc, char *argv[])
{
	int _array_index;
	pthread_t tids[SIGMA];
	atomic_init(&array_index, -1);
	
	for (int i = 0; i < SIGMA; i++){
		atomic_init(&array[i], 0);
	}

	for (int i = 0; i < SIGMA; i++){
		_array_index = atomic_load_explicit(&array_index, memory_order_seq_cst);
		atomic_store_explicit(&array_index, _array_index+1, memory_order_seq_cst);
		pthread_create(&tids[i], NULL, thread, NULL);
	}
	
	for (int i = 0; i < SIGMA; i++){
		pthread_join(tids[i], NULL);
	}

	return 0;
}
