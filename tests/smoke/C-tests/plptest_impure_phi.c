// nidhuggc: -unroll=2 -sc -optimal
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <assert.h>

atomic_int x;

static void *p(void *arg) {
  bool b = true;
  while(true) {
    /* The phi node for b here will differ in constant value, but will
     * not use a "register defined in the loop" */
    if (x) break;
    b = false;
  }
  assert(b);
  return arg;
}

int main() {
  pthread_t pt;
  pthread_create(&pt, NULL, p, NULL);
  x = 1;
}
