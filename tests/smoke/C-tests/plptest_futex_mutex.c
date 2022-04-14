// nidhuggc: -std=gnu11 -- -sc -optimal
#include <stdatomic.h>
#include <pthread.h>
#include <stdbool.h>
#include <assert.h>

static atomic_int mutex, signal;
int value;

#define compare_exchange(var, expected, desired) ({			\
      int __expected = (expected);					\
      atomic_compare_exchange_strong((var), &__expected, (desired)); })


static void futex_wait(atomic_int *var, int if_equals) {
  int old_sig = signal;
  if (*var != if_equals) return;
  while (signal == old_sig);
}

static int mutex_trylock(atomic_int *mutex) {
  return compare_exchange(mutex, 0, 2);
}

static void mutex_lock(atomic_int *mutex) {
  if (compare_exchange(mutex, 0, 1)) return;

  while (1) {
    compare_exchange(mutex, 1, 2);

    futex_wait(mutex, 2);
    if (mutex_trylock(mutex)) return;
  }
}

static void mutex_unlock(atomic_int *mutex) {
  if (compare_exchange(mutex, 1, 0)) return;
  *mutex = 0;
  signal++;
}

static void *p(void *arg) {
  mutex_lock(&mutex);
  value = (int)(intptr_t)arg;
  assert(value == (int)(intptr_t)arg);
  mutex_unlock(&mutex);
  return arg;
}

int main(int argc, char *argv[]) {
  pthread_t ts[2];
  for (unsigned i = 0; i < 2; ++i) {
    pthread_create(ts+i, NULL, p, (void*)(intptr_t)i);
  }
}
