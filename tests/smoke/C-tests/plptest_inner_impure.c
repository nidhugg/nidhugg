// nidhuggc: -unroll=2 -sc -optimal
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <assert.h>

void __VERIFIER_assume(bool b);

atomic_int x;
atomic_int y;
atomic_int z;
atomic_int w = 0;
atomic_int *p;

static void *test(void *arg) {
  while(true) {
    int a = x;
    bool did_z = false;
    int b;
    /* Can only increment z once per iteration of the outer loop, and
     * only exit once b != 0
     */
    while(!(b = y)) {
      if (!did_z) { z++; did_z = true; }
    }
    if (a) break;
    (void)*p;
    /* PLP would insert __VERIFIER_assume(!b) here, trying to say that
     * the outer loop is pure if the inner loop is not taken.
     * However, this actually filters out executions where the inner
     * loop is taken; thus making the assertion in main unsatisfiable.
     */
  }

  return arg;
}

int main() {
  pthread_t t;
  p = &w;
  pthread_create(&t, NULL, test, NULL);
  y = 1;
  y = 0;
  y = 1;
  x = 1;
  assert(z < 2);
}
