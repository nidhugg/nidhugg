#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>
#include <assert.h>

#ifndef N
#  warning "N was not defined, assuming 3"
#  define N 3
#endif

atomic_intptr_t x;

static void *thread(void *arg) {
  intptr_t thread = (intptr_t)arg;
  atomic_store(&x, thread);
  return NULL;
}

int main(int argc, char *argv[]) {
  pthread_t t[N+1];
  atomic_init(&x, 0);
  for (intptr_t i = 1; i <= N; i++)
    pthread_create(t+i, 0, thread, (void*)i);
  assert(atomic_load(&x) < N+1);
  for (intptr_t i = 1; i <= N; i++)
    pthread_join(t[i], 0);
  return 0;
}
