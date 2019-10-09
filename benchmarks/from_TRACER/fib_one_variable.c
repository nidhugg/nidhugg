/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */

/* 
 * Adapted from: https://svn.sosy-lab.org/software/sv-benchmarks/trunk/c/pthread/fib_bench_true-unreach-call.c 
*/

#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include "include/stdatomic.h"

#ifndef N
#  warning "N was not defined"
#  define N 3
#endif

#define IJ(I, J) ((uint64_t)(I) << 32 | (J))
#define I(ij) ((uint32_t)((ij) >> 32))
#define J(ij) ((uint32_t)((ij) & (((uint64_t)2<<32)-1)))


_Atomic(uint64_t) ij;


void __VERIFIER_assume(int);

void *t1(void* arg) {
    for (int k = 0; k < N; k++){
      // i+=j;
      uint64_t tmpj = atomic_load_explicit(&ij, memory_order_seq_cst);
      uint64_t tmpi = atomic_load_explicit(&ij, memory_order_seq_cst);
      atomic_store_explicit(&ij, IJ(I(tmpi)+J(tmpj), J(tmpj)), memory_order_seq_cst);
    }
    return NULL;
}

void *t2(void* arg) {
    for (int k = 0; k < N; k++) {
      // j+=i;
      uint64_t tmpi = atomic_load_explicit(&ij, memory_order_seq_cst);
      uint64_t tmpj = atomic_load_explicit(&ij, memory_order_seq_cst);
      atomic_store_explicit(&ij, IJ(I(tmpi), I(tmpi)+J(tmpj)), memory_order_seq_cst);
    }
    return NULL;
  }

static uint64_t fib(int n) {
    uint64_t cur = 1, prev = 0;
    while(n--) {
      uint64_t next = prev+cur;
      prev = cur;
      cur = next;
    }
    return prev;
}

int main(int argc, char *argv[]) {
    pthread_t id1, id2;

    atomic_init(&ij, IJ(1, 1));
  
    pthread_create(&id1, NULL, t1, NULL);
    pthread_create(&id2, NULL, t2, NULL);
    
	uint64_t correct = fib(2+2*N);
  	uint64_t tmpi = atomic_load_explicit(&ij, memory_order_seq_cst);
  	uint64_t tmpj = atomic_load_explicit(&ij, memory_order_seq_cst);
  	if (I(tmpi) > correct || J(tmpj) > correct) {
  		assert(0);
  	}

    return 0;
}
