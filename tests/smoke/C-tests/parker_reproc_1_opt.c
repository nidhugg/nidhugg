// nidhuggc: -sc -optimal
/* Reproduction case for an implementation bug in RF-SMC for Await
 * Based on a benchmark "parker", which is a recreation of the bug
 * http://bugs.sun.com/view_bug.do?bug_id=6822370
 *
 * based on the description in
 * https://blogs.oracle.com/dave/entry/a_race_in_locksupport_park
 */

void __VERIFIER_assume(int);

#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>

/* Testing */
atomic_int global_cond = 0; // Some global condition variable which the parker will wait for
atomic_int unparker_finished = 0; // Flag indicating that the unparker thread has finished

/* Small low-level mutex implementation */
atomic_int x = 0, y = 0;

/* The parker */
atomic_int parker_counter = 0;
atomic_int parker_cond = 0;

/* Testing */

void *parker(void *_arg){
  x = 1;
  if(y != 0){
    return NULL;
  }
  parker_cond = 1;
  x = 0;
  parker_counter = 0;

  return NULL;
}

void *unparker(void *_arg){
  y = 1;
  __VERIFIER_assume(x == 0);
  int s = parker_counter;
  parker_counter = 1;
  y = 0;
  if(s < 1){
    parker_cond = 0;
  }

  global_cond = 1;

  // Done
  unparker_finished = 1;

  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t t1;
  pthread_create(&t1,NULL,parker,NULL);
  unparker(NULL);
  return 0;
}
