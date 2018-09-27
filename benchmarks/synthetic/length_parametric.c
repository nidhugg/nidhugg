#include <assert.h>
#include <stdatomic.h>
#include <pthread.h>

#ifndef N
#  warning "N was not defined, assuming 2"
#  define N 2
#endif

#define T 2

atomic_int v;
atomic_int local[T];

void *t(void* arg) {
  intptr_t tid = (intptr_t)arg;
  for (int i = 0; i < N; ++i) {
    local[tid] = i;
    (void)local[tid];
  }
  v = tid;
  (void)v;
  return NULL;
}

int main(int argc, char **argv) {
  for (int i = 0; i < T; ++i) {
    pthread_create(NULL, NULL, t, (void*)(intptr_t)i);
  }

  return 0;
}
