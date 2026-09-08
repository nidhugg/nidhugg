#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

#ifndef N
# define N 3
#endif

atomic_int x, y;

void *thread_n(void *unused) {
 atomic_fetch_add(&y, 1);
 return NULL;
}

void *thread_1(void *unused) {
 atomic_fetch_add(&x, 1);
 return NULL;
}

void *thread_2(void *unused) {
 pthread_t t[N];

 for (int i = 0; i < N; i++)
   pthread_create(&t[i], NULL, thread_n, NULL);

 // This program is a reproduction of an RF-SMC bug.
 // These joins, which should not really affect anything, caused RF-SMC
 // to fail to explore all traces.
 for (int i = 0; i < N; i++)
   pthread_join(t[i], NULL);

 atomic_fetch_add(&x, 1);
 return NULL;
}

int main(void) {
 pthread_t t1, t2;
 pthread_create(&t1, NULL, thread_1, NULL);
 pthread_create(&t2, NULL, thread_2, NULL);
 return 0;
}
