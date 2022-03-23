#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

#define N 6

atomic_int x;

void *p(void *arg) {
  for (int i = 0; i < N; ++i) {
    x = i;
  }

  return arg;
}

int main(int argc, char *argv[]) {
  pthread_t t;
  pthread_create(&t, NULL, p, NULL);

  x = 42;
  p(NULL);

  assert(0);
}
