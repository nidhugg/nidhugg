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

  int v, c = 0;
  while(true) {
    v = x;
    if (v < 3) continue;

    x++;
    if (c++ > 3) break;
  }
  return v;
}
