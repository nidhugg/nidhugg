/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */

/* Adapted from: https://svn.sosy-lab.org/software/sv-benchmarks/trunk/c/pthread/indexer_true-unreach-call.c */

#include <assert.h>
#include <atomic>
#include <pthread.h>


#define SIZE  128
#define MAX   4

#ifndef N
#  warning "N was not defined"
#  define N 14
#endif

#define LOOP 5

std::atomic<int> table[SIZE];
pthread_mutex_t  cas_mutex[SIZE];

pthread_t  tids[N];

int cas(std::atomic<int> * tab, int h, int val, int new_val)
{
  int ret_val = 0;
  
  pthread_mutex_lock(&cas_mutex[h]);
  if (atomic_load_explicit(&tab[h], std::memory_order_seq_cst) == val ) {
    atomic_store_explicit(&tab[h], new_val,std::memory_order_seq_cst);
    ret_val = 1;
  }
  pthread_mutex_unlock(&cas_mutex[h]);
  
  
  return ret_val;
}


void *thread_routine(void * arg)
{
  int tid;
  int m = 0, w, h;
  tid = *((int *)arg);
  
  
  for (int i=0; i<LOOP; i++) {
    if ( m < MAX ){
      w = (++m) * 11 + tid;
    } else{
      return NULL;
    }
    
    h = (w * 7) % SIZE;
    
    if (h<0) assert(0);
    
    while (cas(table, h, 0, w) == 0){
      h = (h+1) % SIZE;
    }
    
  }
  
  return NULL;
}

int arg[N];
int main(int argc, char *argv[])
{
  int i;
  
  for (i = 0; i < SIZE; i++)
    pthread_mutex_init (&cas_mutex[i], NULL);
  
  for (i = 0; i < SIZE; i++)
    atomic_init(&table[i], 0);
  
  for (i = 0; i < N; i++){
    arg[i]=i;
    pthread_create(&tids[i], NULL, thread_routine, &arg[i]);
  }
  
  for (i = 0; i < N; i++){
    pthread_join(tids[i], NULL);
  }
  
  return 0;
}


