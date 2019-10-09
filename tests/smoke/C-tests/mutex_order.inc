#include <pthread.h>

#ifndef N
#  warning "N was not defined, assuming 2"
#  define N 2
#endif

pthread_mutex_t m;
int x;

static void *t(void *arg) {
  pthread_mutex_lock(&m);
  ++x;
  pthread_mutex_unlock(&m);
  return NULL;
}

int main(int argc, char *argv[]) {
  pthread_t tids[N];
  pthread_mutex_init(&m, NULL);

  for (unsigned i = 0; i < N; ++i)
    pthread_create(tids+i, NULL, t, NULL);
}
