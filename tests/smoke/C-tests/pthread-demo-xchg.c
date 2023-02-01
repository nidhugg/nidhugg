/***********************************************************************************
                         C-DAC Tech Workshop : hyPACK-2013
                             October 15-18,2013
  Example     : pthread-demo-datarace.c

  Objective   : Write Pthread code to illustrate Data Race Condition
            and its solution using MUTEX.

  Input       : Nothing.

  Output      : Value of Global variable with and without using Mutex.

  Created     :MAY-2013

  E-mail      : hpcfte@cdac.in

****************************************************************************/

/*
  Modifications are made to remove non-standard library dependencies by Yihao from
  VSL of University of Delaware.
*/
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdatomic.h>

extern void __VERIFIER_assume(int);

int myglobal;                                        // declaration of global variable
atomic_int mymutex;  // initialization of MUTEX variable

#ifndef N
#  warning "N was not defined, assuming 20"
#  define N 3
#endif

static void lock() {
  __VERIFIER_assume(atomic_exchange(&mymutex, 1) == 0);
}

static void unlock() {
  atomic_store(&mymutex, 0);
}

void *thread_function_mutex(void *arg)   // Function which operates on myglobal using mutex
{
  int i,j;
  for ( i=0; i<N; i++ )
    {
      lock();
      j=myglobal;
      j=j+1;
      //        fflush(stdout);
      myglobal=j;
      unlock();
    }
  return NULL;
}

int main(void)
{
  pthread_t mythread;
  int i;

  atomic_init(&mymutex, 0);

  myglobal = 0;

  if ( pthread_create( &mythread, NULL, thread_function_mutex, NULL) ) // calling thread_function_mutex
    {
      return -1;
    }
  for ( i=0; i<N; i++)
    {
      lock();
      myglobal=myglobal+1;
      unlock();
    }
  //      fflush(stdout);
  if ( pthread_join ( mythread, NULL ) )
    {
      return -1;
    }

  assert(myglobal == (2*N));
  return 0;
}
