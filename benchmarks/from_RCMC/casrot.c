/* This benchmark is adapted from RCMC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#ifndef N
#  warning "N was not defined"
#  define N 6
#endif

atomic_int x;
int idx[N];

void *thread_n(void *arg)
{
	int new = *((int *) arg);
	int exp = new - 1;

	atomic_compare_exchange_strong_explicit(&x, &exp, new, memory_order_seq_cst,
						memory_order_seq_cst);
	return NULL;
}

int main(int argc, char *argv[])
{
	pthread_t t[N];

	atomic_init(&x, 0);

	for (int i = 1; i < N + 1; i++) {
		idx[i - 1] = i;
		pthread_create(&t[i - 1], NULL, thread_n, &idx[i - 1]);
	}

	return 0;

}
