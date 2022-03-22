// nidhuggc: -sc -rf
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

#define N 6

atomic_int x;

void *p(void *arg) {
  for (int i = 0; i < N; ++i) {
    atomic_exchange(&x, i);
  }

  return arg;
}

int main(int argc, char *argv[]) {
  pthread_t t;
  pthread_create(&t, NULL, p, NULL);

  int n = x;
  p(NULL);

  assert(n != (N - 1));
}
