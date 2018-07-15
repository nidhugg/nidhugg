/* This benchmark is in the journal JACM 2017
   https://dl.acm.org/citation.cfm?id=3073408
*/

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#ifndef N
#  warning "N was not defined"
#  define N 5
#endif

atomic_int array[N+1];
int idx[N+1];

void *thread_reader(void *unused)
{
	for (int i = N; atomic_load_explicit(&array[i], memory_order_seq_cst) != 0; i--);
	return NULL;
}

void *thread_writer(void *arg)
{
	int j = *((int *) arg);

	atomic_store_explicit(&array[j],
			      atomic_load_explicit(&array[j - 1], memory_order_seq_cst) + 1,
			      memory_order_seq_cst);
}


int main(int argc, char **argv)
{
	pthread_t t[N+1];

	for (int i =0; i <= N; i++)
		atomic_init(&array[i], 0);

	for (int i = 0; i <= N; i++) {
		idx[i] = i;
		if (i == 0) {
			pthread_create(&t[i], NULL, thread_reader, &idx[i]);
		} else {
			pthread_create(&t[i], NULL, thread_writer, &idx[i]);
		}
	}

	return 0;
}
