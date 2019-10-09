/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC
 */


/* 
 * Adapted from: https://svn.sosy-lab.org/software/sv-benchmarks/trunk/c/pthread/fib_bench_true-unreach-call.c 
 * Written by Magnus Lang
*/

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

#define N 1

#define IJ(I, J) ((uint64_t)(I) << 32 | (J))
#define I(ij) ((uint32_t)((ij) >> 32))
#define J(ij) ((uint32_t)((ij) & (((uint64_t)2<<32)-1)))

volatile uint64_t ij;

void *t1(void* arg) {
    for (int k = 0; k < N; k++){
      	// i+=j;
      	uint64_t tmpj = ij;
      	uint64_t tmpi = ij;
		ij = IJ(I(tmpi)+J(tmpj), J(tmpj));
    }
    return NULL;
}

void *t2(void* arg) {
    for (int k = 0; k < N; k++) {
      	// j+=i;
      	uint64_t tmpi = ij;
      	uint64_t tmpj = ij;
      	ij = IJ(I(tmpi), I(tmpi)+J(tmpj));
    }
    return NULL;
}

int fib(int n) {
    int cur = 1, prev = 0;
    while(n--) {
      int next = prev+cur;
      prev = cur;
      cur = next;
    }
    return prev;
}

int main(int argc, char *argv[]) {
    pthread_t id1, id2;
  
    ij = IJ(1, 1);
  
    pthread_create(&id1, NULL, t1, NULL);
    pthread_create(&id2, NULL, t2, NULL);

    int correct = fib(2+2*N);
    uint64_t tmpi = ij;
    uint64_t tmpj = ij;
    if (I(tmpi) > correct || J(tmpj) > correct) {
      assert(0);
    }

    return 0;
}
