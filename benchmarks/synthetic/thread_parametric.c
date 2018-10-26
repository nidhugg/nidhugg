#include <assert.h>
#include <stdatomic.h>
#include <pthread.h>

#ifndef N
#  warning "N was not defined, assuming 2"
#  define N 2
#endif

#define B 42

atomic_int v;
atomic_int local[N];

void *t(void* arg) {
  intptr_t tid = (intptr_t)arg;
  for (int i = 0; i < B; ++i) {
    atomic_store(local+tid, i);
    (void)atomic_load(local+tid);
  }
  if (tid < 2) {
    atomic_store(&v, tid);
    (void)atomic_load(&v);
  }
  return NULL;
}

int main(int argc, char **argv) {
  pthread_t tid[N];
  for (int i = 0; i < N; ++i) {
    pthread_create(tid+i, NULL, t, (void*)(intptr_t)i);
  }

  return 0;
}
