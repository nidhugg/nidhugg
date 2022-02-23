// nidhuggc: -sc -optimal -commute-rmws
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>

void __VERIFIER_assume(bool truth);
int __VERIFIER_xchg_await_aint(atomic_int *var, int value, int cond,
			       int comp_operand);
#define AWAIT_COND_EQ 0b010

static int do_exchange_await(atomic_int *var, int value, int cond,
			     int comp_operand) {
  int res;
#ifdef USE_ASSUME
  res = atomic_exchange(var, value);
  assert(cond == AWAIT_COND_EQ);
  bool comp_res = res == comp_operand;
  __VERIFIER_assume(comp_res);
#else
  res = __VERIFIER_xchg_await_aint(var, value, cond, comp_operand);
#endif
  return res;
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

  do_exchange_await(&x, 999, AWAIT_COND_EQ, 7);

  return 0;
}
