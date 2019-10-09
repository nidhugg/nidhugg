#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>

#define N 3

pthread_mutex_t m;
atomic_uint x = 0;

static void *t(void *arg) {
  unsigned tid = (unsigned)(uintptr_t)arg;
  pthread_mutex_lock(&m);
  if (atomic_fetch_add(&x, 1) == tid)
    pthread_mutex_unlock(&m);
  return NULL;
}

int main() {
  pthread_t tid[N];
  pthread_mutex_init(&m, NULL);

  for (unsigned i = 0; i < N; ++i)
    pthread_create(tid + i, NULL, t, (void*)(uintptr_t)i);

  for (unsigned i = 0; i < N; ++i)
    pthread_join(tid[i], NULL);

  return 0;
}
