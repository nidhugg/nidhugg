/* This benchmark is adapted from RCMC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#ifndef N
#  warning "N was not defined"
#  define N 5
#endif

atomic_int x;
atomic_int y;

void *thread_n(void *unused)
{
	atomic_fetch_add_explicit(&x, 1, memory_order_seq_cst);
	atomic_fetch_add_explicit(&y, 1, memory_order_seq_cst);
	return NULL;
}

int main(int argc, char **argv)
{
	pthread_t t[N];

	atomic_init(&x, 0);
	atomic_init(&y, 0);

	for (int i = 0; i < N; i++)
		pthread_create(&t[i], NULL, thread_n, NULL);

	return 0;
}
