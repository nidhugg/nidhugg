#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include "stdatomic.h"

atomic_int last_id, x;

int get_message(int tid) {
    return tid;
}

void *thread(void *arg)
{
    int j = (int)(intptr_t)arg;

    for (int i = 0; i < N; ++i) {
	atomic_store(&last_id, j);
	atomic_store(&x, get_message(j));
	if (atomic_load(&last_id) == j) {
	    return (void*) atomic_load(&x);
	}
    }
    
    return NULL;
}

int main()
{
    pthread_t t[N];

    for (int i = 0; i < 2; ++i) {
        pthread_create(&t[i], NULL, thread, (void*)(intptr_t)i);
    }

    return 0;
}
