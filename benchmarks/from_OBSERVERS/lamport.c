#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <pthread.h>
#include <assert.h>

/* Lamport's fast mutex algorithm 2
 * (https://users.soe.ucsc.edu/~scott/courses/Fall11/221/Papers/Sync/lamport-tocs87.pdf)
 */

#ifndef N
#  warning "N was not defined, assuming 3"
#  define N 3
#endif

void __VERIFIER_assume(intptr_t);

atomic_intptr_t b[N+1];
atomic_intptr_t x, y;
atomic_intptr_t in_critical;

static void lock(intptr_t thread) {
  while (1) {
    b[thread] = true;
    x = thread;
    if (y != 0) {
      b[thread] = false;
      __VERIFIER_assume(y == 0);
      continue;
    }
    y = thread;
    if (x != thread) {
      b[thread] = false;
      for (intptr_t j = 1; j <= N; j++)
	__VERIFIER_assume(b[j] == false);
      if (y != thread) {
	__VERIFIER_assume(y == 0);
	continue;
      }
    }
    break;
  }
}

static void unlock(intptr_t thread) {
  y = 0;
  b[thread] = false;
}

static void *thread(void *arg) {
  intptr_t thread = (intptr_t)arg;
  lock(thread);

  in_critical = thread;                               // WA
  assert(in_critical == thread);                      // RA

  unlock(thread);
  return NULL;
}

int main() {
  pthread_t t[N+1];
  for (intptr_t i = 1; i <= N; i++)
    pthread_create(t+i, 0, thread, (void*)i);
  // for (intptr_t i = 1; i <= N; i++)
  //   pthread_join(t[i], 0);
  return 0;
}
