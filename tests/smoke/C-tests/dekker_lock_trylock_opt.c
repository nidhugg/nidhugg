// nidhuggc: -sc -optimal
/* Lock vs trylock for a simplified dekker's mutex */

#include <pthread.h>
#include <stdatomic.h>

void __VERIFIER_assume(int);

/* Dekker's mutex */
atomic_int x = 0, y = 0;

void *t(void *_arg){
  // trylock
  x = 1;
  (void)y;
  x = 0;

  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t t1;
  pthread_create(&t1,NULL,t,NULL);

  // lock
  y = 1;
  __VERIFIER_assume(x == 0);
  y = 0;

  return 0;
}
