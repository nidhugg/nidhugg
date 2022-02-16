// nidhuggc: -sc -optimal -no-commute-rmws -unroll=4
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

#define RW_LOCK_BIAS            0x00100000

void __VERIFIER_assume(int);

/* Reproduction case for await bug where nidhugg incorrectly thought
 * that an assume-condition would be satisfied. */

typedef union {
  atomic_int lock;
} rwlock_t;

static inline void write_unlock(rwlock_t *rw) {
  atomic_fetch_add(&rw->lock, RW_LOCK_BIAS);
}

rwlock_t lock;
int shareddata;

void *threadR(void *arg) {
  int priorvalue = atomic_fetch_sub(&lock.lock, 1);
  while (priorvalue <= 0) {
    atomic_fetch_add(&lock.lock, 1);
    while (atomic_load(&lock.lock) <= 0);
    priorvalue = atomic_fetch_sub(&lock.lock, 1);
  }
  return NULL;
}

void *threadW1(void *arg) {
  {
    int priorvalue = atomic_exchange(&lock.lock, 0);
    __VERIFIER_assume(!(priorvalue != RW_LOCK_BIAS));
  }
  write_unlock(&lock);
  return NULL;
}

void *threadW2(void *arg) {
  {
    int priorvalue = atomic_fetch_sub(&lock.lock, RW_LOCK_BIAS);
    __VERIFIER_assume(!(priorvalue != RW_LOCK_BIAS));
  }
  return NULL;
}

int main() {
  pthread_t r, w1, w2;

  atomic_init(&lock.lock, RW_LOCK_BIAS);
  pthread_create(&w1, NULL, threadW1, NULL);
  pthread_create(&w2, NULL, threadW2, NULL);
  pthread_create(&r, NULL, threadR, NULL);

  return 0;
}
