#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>

atomic_uint x = 0;

static void *t(void *arg) {
  unsigned tid = (unsigned)(uintptr_t)arg;
  atomic_compare_exchange_strong(&x, &tid, tid+1);
  return NULL;
}

int main() {
  pthread_t tid[N];

  for (unsigned i = 0; i < N; ++i)
    pthread_create(tid + i, NULL, t, (void*)(uintptr_t)i);

  for (unsigned i = 0; i < N; ++i)
    pthread_join(tid[i], NULL);

  return 0;
}
