#include <pthread.h>
#include <stdint.h>

#define N 3

pthread_mutex_t m;
int x;

void __VERIFIER_assume(int truth);

static void *t(void *arg) {
  pthread_mutex_lock(&m);
  ++x;
  __VERIFIER_assume((intptr_t)arg != 0);
  pthread_mutex_unlock(&m);
  return NULL;
}

int main(int argc, char *argv[]) {
  pthread_t tids[N];
  pthread_mutex_init(&m, NULL);

  for (unsigned i = 0; i < N; ++i)
    pthread_create(tids+i, NULL, t, (void*)(intptr_t)i);
}
