// nidhuggc: -sc -optimal -no-assume-await

#include <pthread.h>
#include <stdatomic.h>

atomic_int x;

void *p(void *arg) {
  x = 1;
  return arg;
}

int main(int argc, char *argv[]) {
  pthread_t pt[2];
  for (unsigned i = 0; i < 2; ++i) pthread_create(pt+i, NULL, p, NULL);

  int expected = x;
  while (!atomic_compare_exchange_weak(&x, &expected, expected+1));

  for (unsigned i = 0; i < 2; ++i) pthread_join(pt[i], NULL);
  return x;
}
