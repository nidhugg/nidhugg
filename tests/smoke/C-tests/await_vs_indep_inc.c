// nidhuggc: -sc -optimal -commute-rmws
#include <pthread.h>
#include <stdatomic.h>

static void await(atomic_int *var, int value) {
  while (*var != value);
}

atomic_int x;

static void *p(void *arg) {
  atomic_fetch_add(&x, (int)(intptr_t)arg);
  return arg;
};

int main(int argc, char *argv[]) {
  pthread_t pt[4];
  pthread_create(pt+0, NULL, p, (void*)(intptr_t)2);
  pthread_create(pt+1, NULL, p, (void*)(intptr_t)4);
  pthread_create(pt+2, NULL, p, (void*)(intptr_t)3);
  pthread_create(pt+3, NULL, p, (void*)(intptr_t)1);

  await(&x, 7);

  return 0;
}
