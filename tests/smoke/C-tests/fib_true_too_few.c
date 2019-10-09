/* Adapted from: https://svn.sosy-lab.org/software/sv-benchmarks/trunk/c/pthread/fib_bench_true-unreach-call.c */

#include <assert.h>

#include <pthread.h>

volatile int i=1, j=1;

void *
t1(void* arg)
{
  {
    int tj = 1; // j;
    int ti = 1; // i;
    i = ti + tj;
  }
  {
    int tj = j;
    int ti = 1; // i;
    // i = ti + tj;
  }

  pthread_exit(NULL);
}

void *
t2(void* arg)
{
  {
    int ti = 1; // i;
    int tj = 1; // j;
    j = tj + ti;
  }
  {
    int ti = i;
    int tj = 1; // j;
    j = tj + ti;
  }

  pthread_exit(NULL);
}

int
main(int argc, char **argv)
{
  #ifndef GOTO
  pthread_t id1, id2;

  pthread_create(&id1, NULL, t1, NULL);
  pthread_create(&id2, NULL, t2, NULL);
  #else
  __CPROVER_ASYNC_0: t1(NULL);
  __CPROVER_ASYNC_1: t2(NULL);
  #endif

  int ti = 1; // i;
  int tj = 1; // j;
  if (ti > 144 || tj > 144) {
    assert(0);
  }

  return 0;
}
