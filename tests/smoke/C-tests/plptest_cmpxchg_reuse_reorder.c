// nidhuggc: -sc -optimal -no-assume-await

#include <pthread.h>
#include <stdatomic.h>

atomic_int x;
atomic_int y = 1;

void *p(void *arg) {
  x = 1;
  return arg;
}

int main(int argc, char *argv[]) {
  pthread_t pt[2];
  for (unsigned i = 0; i < 2; ++i) pthread_create(pt+i, NULL, p, NULL);

  /* It is unsafe to reorder the loads (at least since they are SC loads) */
  int expected = x;
  int other = y;
  while (!atomic_compare_exchange_strong(&x, &expected, expected+other));

  for (unsigned i = 0; i < 2; ++i) pthread_join(pt[i], NULL);
  return x;
}
