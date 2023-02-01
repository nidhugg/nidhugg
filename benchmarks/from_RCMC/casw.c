/* This benchmark is adapted from RCMC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#ifndef N
#  warning "N was not defined"
#  define N 4
#endif

atomic_int x;
int idx[N];

void *thread_n(void *arg)
{
	int r = 0, val = *((int *) arg);

	atomic_compare_exchange_strong_explicit(&x, &r, 1, memory_order_seq_cst,
						memory_order_seq_cst);
	atomic_store_explicit(&x, val + 3, memory_order_seq_cst);
	
	return NULL;

}

int main(int argc, char **argv)
{
	pthread_t t[N];

	atomic_init(&x, 0);

	for (int i = 0; i < N; i++) {
		idx[i] = i;
		pthread_create(&t[i], NULL, thread_n, &idx[i]);
	}

	return 0;
}
