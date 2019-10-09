/* Adapted from: https://svn.sosy-lab.org/software/sv-benchmarks/trunk/c/pthread/fib_bench_true-unreach-call.c */

#include <assert.h>
#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>

#define IJ(I, J) ((uint64_t)(I) << 32 | (J))
#define I(ij) ((uint32_t)((ij) >> 32))
#define J(ij) ((uint32_t)((ij) & (((uint64_t)2<<32)-1)))

_Atomic uint64_t ij = IJ(1, 1);

#ifndef N
#  define N 1
#endif

void *t1(void* arg) {
    for (int k = 0; k < N; k++){
	// i+=j;
      uint64_t tmpj = 1;
	uint64_t tmpi = atomic_load_explicit(&ij, memory_order_acquire);
	atomic_store_explicit(&ij, IJ(I(tmpi)+J(tmpj), J(tmpj)),
			      memory_order_release);
    }

    return NULL;
}

void *t2(void* arg) {
    for (int k = 0; k < N; k++) {
	// j+=i;
      uint64_t tmpi = atomic_load_explicit(&ij, memory_order_acquire);
	uint64_t tmpj = atomic_load_explicit(&ij, memory_order_acquire);
	atomic_store_explicit(&ij, IJ(I(tmpi), I(tmpi)+J(tmpj)),
			      memory_order_release);
    }
  
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t id1, id2;

    pthread_create(&id1, NULL, t1, NULL);
    pthread_create(&id2, NULL, t2, NULL);

    uint64_t tmpi = atomic_load_explicit(&ij, memory_order_acquire);
    uint64_t tmpj = atomic_load_explicit(&ij, memory_order_acquire);

    return 0;
}
