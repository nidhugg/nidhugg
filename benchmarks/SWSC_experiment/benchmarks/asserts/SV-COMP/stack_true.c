/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */

/* Adapted from: https://github.com/sosy-lab/sv-benchmarks/blob/master/c/pthread/stack_true-unreach-call.c */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#ifndef N
#  warning "N was not defined"
#  define N 5
#endif


#define TRUE      (1)
#define FALSE     (0)
#define SIZE      (4)
#define OVERFLOW  (-1)
#define UNDERFLOW (-2)
#define FULL (-3)


// shared variables
atomic_int top;
atomic_int  stack[SIZE];
pthread_mutex_t  lock;

void inc_top(void)
{
  int _top;
  _top = atomic_load_explicit(&top, memory_order_seq_cst);
  atomic_store_explicit(&top, _top+1, memory_order_seq_cst);
}

void dec_top(void)
{
  int _top;
  _top = atomic_load_explicit(&top, memory_order_seq_cst);
  atomic_store_explicit(&top, _top-1, memory_order_seq_cst);
}

int get_top(void)
{
  int _top;
  _top = atomic_load_explicit(&top, memory_order_seq_cst);
  return _top;
}

int push(unsigned int x)
{
  if (get_top() > SIZE) {
    return OVERFLOW;
  } else if (get_top() == SIZE) { // full state of the stack
    return FULL;
  } else {
    atomic_store_explicit(&stack[get_top()], x, memory_order_seq_cst);
    inc_top();
  }
  
  return 0;
}

int pop(void)
{
  if (get_top()==0) {
    return UNDERFLOW;
  } else {
    dec_top();
    int _return = atomic_load_explicit(&stack[get_top()], memory_order_seq_cst);
    return _return;
  }
  
  return 0;
}


void *pushthread(void *arg)
{
  int i, tid;
  unsigned int tmp;
  tid = *((int *)arg);
  
  for(i=0; i<SIZE; i++)
  {
    pthread_mutex_lock(&lock);
    tmp = tid % SIZE;
    if (push(tmp) == OVERFLOW)
      assert(0);
    pthread_mutex_unlock(&lock);
  }
  
  return NULL;
}

void *popthread(void *arg)
{
  int i, _top;
  
  for(i=0; i<SIZE; i++)
  {
    pthread_mutex_lock(&lock);
    _top = atomic_load_explicit(&top, memory_order_seq_cst);
    if (_top > 0){
      if (!(pop() != UNDERFLOW))
        assert(0);
    }
    pthread_mutex_unlock(&lock);
  }
  
  return NULL;
}

int arg[N];
int main(int argc, char *argv[])
{
  int i;
  pthread_t t1s[N], t2s[N];
  
  atomic_init(&top, 0);
  pthread_mutex_init (&lock, NULL);
  
  for (int i=0; i<SIZE; i++)
    atomic_init(&stack[i], 0);
  
  for (int i=0; i<N; i++) {
    arg[i] = i;
    pthread_create(&t1s[i], NULL, pushthread, &arg[i]);
  }
  
  for (int i=0; i<N; i++) {
    pthread_create(&t2s[i], NULL, popthread, NULL);
  }
  
  return 0;
}
