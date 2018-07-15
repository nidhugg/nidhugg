/* This benchmark is adapted from RCMC */


#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#ifndef N
#  warning "N was not defined"
#  define N 8
#endif

atomic_int x;
atomic_int idx[N+1];

void *thread_writer(void *unused)
{
	atomic_store_explicit(&x, 42, memory_order_seq_cst);
	return NULL;
}

void *thread_reader(void *arg)
{
	int r = *((int *) arg);
	atomic_load_explicit(&x, memory_order_seq_cst);
	return NULL;
}

int main(int argc, char *argv[])
{
	pthread_t t[N+1];

	atomic_init(&x, 0);

	for (int i = 0; i <= N; i++)
		atomic_init(&idx[i], i);

	for (int i = 0; i <= N; i++) {
		if (i == 0)
			pthread_create(&t[i], NULL, thread_writer, NULL);
		else
			pthread_create(&t[i], NULL, thread_reader, &idx[i-1]);
	}

	for (int i = 0; i <= N; i++) {
		pthread_join(t[i], NULL);
	}

	return 0;
}
