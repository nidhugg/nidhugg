// nidhuggc: -sc -optimal --unroll=3
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <assert.h>

atomic_int x, y, z;

static void *p(void *arg) {
  while(true) {
    if (x) {
      if (y) {
	x = 2;
      }
    } else {
      x = 3;
    }
    if (z) break;
  }
  return arg;
}

int main() {
  pthread_t pt;
  pthread_create(&pt, NULL, p, NULL);
  z = 1;
  x = 1;
  y = 1;
  (void)x;
}
