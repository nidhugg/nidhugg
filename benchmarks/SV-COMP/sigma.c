/* Adapted from: https://raw.githubusercontent.com/sosy-lab/sv-benchmarks/master/c/pthread/sigma_false-unreach-call.c */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#ifndef N
#  warning "N was not defined"
#  define N 5
#endif

// shared variables
atomic_int array[N];
atomic_int array_index;

void *thread(void *arg) {
  atomic_store(array + atomic_load(&array_index), 1);

  return NULL;
}

int main(int argc, char *argv[]) {
	pthread_t tids[N];
	atomic_init(&array_index, -1);
	
#ifdef CDSC
	for (int i = 0; i < N; i++){
		atomic_init(&array[i], 0);
	}
#endif

	for (int i = 0; i < N; i++){
		atomic_store(&array_index, atomic_load(&array_index)+1);
		pthread_create(&tids[i], NULL, thread, NULL);
	}
	
	for (int i = 0; i < N; i++){
		pthread_join(tids[i], NULL);
	}

	return 0;
}
