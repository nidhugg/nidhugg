// nidhuggc: -sc -optimal
/*
 * A too simplistic mutual exclusion protocol.
 * Safety: Safe
 * Traces: 1 + 2 blocked
 */
#include <pthread.h>
#include <stdatomic.h>

atomic_int x;

static void *t(void *arg) {
  while (x != 0);
  x = 1;
  return arg;
}

int main(int argc, char *argv[]) {
  pthread_t tid;
  pthread_create(&tid, NULL, t, NULL);

  while(x != 0);
  x = 1;

  pthread_join(tid, NULL);
}
