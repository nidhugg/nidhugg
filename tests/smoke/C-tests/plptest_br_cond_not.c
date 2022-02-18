// nidhuggc: -sc -optimal --no-spin-assume --unroll=3
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>

atomic_int x;

void *p(void *arg) {
  for (unsigned i = 0; i < 3; ++i)
    x = 0;

  x = 1;

  return arg;
}

int main() {
  pthread_t pt;
  pthread_create(&pt, NULL, p, NULL);

  while(!x);

  return 0;
}
