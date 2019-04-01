#include <pthread.h>
#include <assert.h>
#include <stdatomic.h>

#ifndef N
#  define N 3
#endif
#ifndef CHECK
#  define CHECK 1
#endif
#define SET (N-1)

static atomic_int a[1], b[1];

static void *setThread(void *param) {
  atomic_store(a, 1);
  atomic_store(b, -1);

  return NULL;
}

static void *checkThread(void *param) {
  assert((atomic_load(a) == 0 && atomic_load(b) == 0)
      || (atomic_load(a) == 1 && atomic_load(b) == -1)
      || 1);

  return NULL;
}

int main(int argc, char *argv[]) {
  pthread_t setPool[SET];
  pthread_t checkPool[CHECK];

  for (unsigned i = 0; i < SET; i++)
    pthread_create(&setPool[i], NULL, &setThread, NULL);
  for (unsigned i = 0; i < CHECK; i++)
    pthread_create(&checkPool[i], NULL, &checkThread, NULL);

  for (unsigned i = 0; i < SET; i++)
    pthread_join(setPool[i], NULL);
  for (unsigned i = 0; i < CHECK; i++)
    pthread_join(checkPool[i], NULL);
}
