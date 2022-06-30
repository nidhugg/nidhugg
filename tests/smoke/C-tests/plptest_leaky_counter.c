// nidhuggc: -sc -rf --no-spin-assume --unroll=3
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>

atomic_int x;

void *p(void *arg) {
  x = 3;

  return arg;
}

int main() {
  pthread_t pt;
  pthread_create(&pt, NULL, p, NULL);

  int v, c = 0;
  while((v = x) < 3) {
    c++;
  }
  return v;
}
