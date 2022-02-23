// nidhuggc: -sc -optimal
#include <assert.h>
#include <stdatomic.h>
#include <pthread.h>
#include <stdbool.h>

atomic_int a = 0;

#ifndef N
#  warning "N was not defined, assuming 3"
#  define N 3
#endif

#ifndef STORE
#  warning "STORE was not defined, assuming 0"
#  define STORE 0
#endif

#ifndef SKIP
#  warning "SKIP was not defined, assuming 2"
#  define SKIP 2
#endif

void __VERIFIER_assume(bool truth);
int __VERIFIER_xchg_await_aint(atomic_int *var, int value, int cond,
			       int comp_operand);
#define AWAIT_COND_NE 0b101
#define AWAIT_COND_SLT 0b1100

static int do_exchange_await(atomic_int *var, int value, int cond,
			     int comp_operand) {
#ifdef USE_ASSUME
  int res = atomic_exchange(var, value);
  assert(cond == AWAIT_COND_SLT);
  bool comp_res = res < comp_operand;
  __VERIFIER_assume(comp_res);
#else
  __VERIFIER_xchg_await_aint(var, value, cond, comp_operand);
#endif
}

void *incrementer(void* arg) {
  for (unsigned n = 0; n < N; ++n) {
    atomic_fetch_add(&a, 1);
  }
}

int
main(int argc, char **argv)
{
  pthread_t it;
  pthread_create(&it, NULL, incrementer, NULL);

  do_exchange_await(&a, STORE, AWAIT_COND_SLT, SKIP);

  return 0;
}
