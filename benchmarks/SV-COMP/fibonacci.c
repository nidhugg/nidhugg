/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */

/* Adapted from: https://svn.sosy-lab.org/software/sv-benchmarks/trunk/c/pthread/fib_bench_false-unreach-call.c */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#define NUM 4

// shared variables
atomic_int i, j;

void *t1(void *arg)
{
	int _i, _j;
	for (int k=0; k<NUM; k++) {
		_i = atomic_load_explicit(&i, memory_order_seq_cst);
		_j = atomic_load_explicit(&j, memory_order_seq_cst);
		atomic_store_explicit(&i, _i+_j, memory_order_seq_cst);
	}
	return NULL;
}

void *t2(void *arg)
{
	int _i, _j;
	for (int k=0; k<NUM; k++) {
		_i = atomic_load_explicit(&i, memory_order_seq_cst);
		_j = atomic_load_explicit(&j, memory_order_seq_cst);
		atomic_store_explicit(&j, _i+_j, memory_order_seq_cst);
	}
	return NULL;
}

static int fib(int n) {
	int cur = 1, prev = 0;
	while(n--) {
		int next = prev+cur;
		prev = cur;
		cur = next;
	}
	return prev;
}

int main(int argc, char *argv[])
{
	pthread_t a, b;

	atomic_init(&i, 1);
	atomic_init(&j, 1);

	pthread_create(&a, NULL, t1, NULL);
	pthread_create(&b, NULL, t2, NULL);

	int correct = fib(2+2*NUM);
	int _i = atomic_load_explicit(&i, memory_order_seq_cst);;
	int _j = atomic_load_explicit(&j, memory_order_seq_cst);

	if (_i > correct || _j > correct) {
		assert(0);
	}

	return 0;
}
