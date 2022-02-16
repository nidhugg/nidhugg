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

static void *q(void *arg) {
  atomic_fetch_add(&x, 1);
  atomic_fetch_add(&x, 1);
  atomic_fetch_add(&x, 8);
  return arg;
}

static void *r(void *arg) {
  await(&x, 10);
}

int main(int argc, char *argv[]) {
  pthread_t pt[3], qt, rt;
  pthread_create(pt+0, NULL, p, (void*)(intptr_t)3);
  pthread_create(&qt,  NULL, q, NULL);
  pthread_create(pt+1, NULL, p, (void*)(intptr_t)3);
  pthread_create(pt+2, NULL, p, (void*)(intptr_t)4);
  pthread_create(&rt,  NULL, r, NULL);

  return 0;
}
