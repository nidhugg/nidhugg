// nidhuggc: -sc -optimal
/*
 * Simplified dekker's mutex
 * Safety: Safe
 * Traces: 2
 * Await-blocked traces: 1
 */
#include <pthread.h>
#include <stdatomic.h>

atomic_int x, y;

static void *t(void *arg) {
  x = 1;
  while (y != 0);
  x = 0;
  return arg;
}

int main(int argc, char *argv[]) {
  pthread_t tid;
  pthread_create(&tid, NULL, t, NULL);

  y = 1;
  while(x != 0);
  y = 0;

  pthread_join(tid, NULL);
}
