// nidhuggc: -sc -optimal
#include <stdbool.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int y, z;

static void *t(void *arg) {
  y = 1;
  return arg;
}

atomic_int x, w;
static void *v(void *arg) {
  /* Thread 2 of sub-program */
  x = 1;
  w = 1;
  return arg;
}

static void *u(void *arg) {
  if (y == 0) {
    /* An entire program */
    pthread_t vt;
    pthread_create(&vt, NULL, v, NULL);
    atomic_load(&x);
    pthread_join(vt, NULL);
    z = 1; /* With changes to the await preceding some loads */
    atomic_load(&w);
    /* Ending with an unblock of the initial await */
    z = 42;
  }
  return arg;
}

int main(int argc, char *argv[]) {
  pthread_t tt, ut;
  pthread_create(&tt, NULL, t, NULL);
  pthread_create(&ut, NULL, u, NULL);

  /* Load-await */
  while (z != 42);

  return 0;
}
