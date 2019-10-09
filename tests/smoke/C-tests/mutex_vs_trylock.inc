#include <pthread.h>

#ifndef N
#  define N 1
#endif

pthread_mutex_t m;
int x;

static void *t(void *arg) {
  if (pthread_mutex_trylock(&m)) return NULL;
  ++x;
  pthread_mutex_unlock(&m);
  return NULL;
}

int main(int argc, char *argv[]) {
  pthread_t tids[N];
  pthread_mutex_init(&m, NULL);

  for (unsigned i = 0; i < N; ++i)
    pthread_create(tids+i, NULL, t, NULL);

  pthread_mutex_lock(&m);
  ++x;
  pthread_mutex_unlock(&m);
}
