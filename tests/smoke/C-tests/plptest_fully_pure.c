// nidhuggc: -sc -rf --no-spin-assume --unroll=3
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>

atomic_int x;

void *p(void *arg) {
  x = (int)x; // just x = x, but silence warning

  return arg;
}

int main() {
  pthread_t pt;
  pthread_create(&pt, NULL, p, NULL);

  while(true) {
    if (x < 3) continue;

    x++;
  }
  return 0;
}
