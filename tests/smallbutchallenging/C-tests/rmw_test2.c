#include <assert.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int a = 0;

#ifndef N
#  warning "N was not defined, assuming 2"
#  define N 2
#endif

#ifndef M
#  warning "M was not defined, assuming N"
#  define M N
#endif

#ifndef O
#  warning "O was not defined, assuming M"
#  define O M
#endif

void *tfa(void* arg) {
  atomic_fetch_add(&a, (int)(intptr_t)arg);
}

void *tl(void* arg) {
  atomic_load(&a);
}

void *ts(void* arg) {
  atomic_store(&a, (int)(intptr_t)arg + N);
}

int
main(int argc, char **argv)
{
  pthread_t tfaid[N], tlid[M], tsid[O];

  for (int i = 0; i < N || i < M || i < O; ++i) {
    if (i < N) pthread_create(tfaid + i, NULL, tfa, (void*)(intptr_t)(i+1));
    if (i < M) pthread_create(tlid + i, NULL, tl, (void*)(intptr_t)(i+1));
    if (i < O) pthread_create(tsid + i, NULL, ts, (void*)(intptr_t)(i+1));
  }

  return 0;
}
