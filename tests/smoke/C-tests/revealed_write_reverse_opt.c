// nidhuggc: -sc -optimal
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>

atomic_uint mem[2];
#define y mem[0]
#define x mem[1]
extern void __VERIFIER_assume(int cond);

static void *thread(void *arg) {
  __VERIFIER_assume(x == 0);
  y = 1;
  return NULL;
}

int main() {
    pthread_t t;
    pthread_create(&t, NULL, thread, NULL);

    int ret;
    x = 1;
    __VERIFIER_assume(y == 1);

    pthread_join(t, NULL);
}
