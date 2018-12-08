/* Copyright (C) 2018
 * This benchmark is part of TRACER
 */

/* Adapted from: https://github.com/sosy-lab/sv-benchmarks/blob/master/c/pthread/queue_ok_true-unreach-call.c */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#ifndef N
#  warning "N was not defined"
#  define N 5
#endif


#define SIZE	(4)
#define EMPTY	(-1)
#define FULL	(-2)
#define FALSE	(0)
#define TRUE	(1)

atomic_int element[SIZE];
atomic_int head;
atomic_int tail;
atomic_int amount;
pthread_mutex_t  lock;
atomic_int stored_elements[SIZE];
atomic_int enqueue_flag, dequeue_flag;

void init(void)
{
  atomic_init(&head, 0);
  atomic_init(&tail, 0);
  atomic_init(&amount, 0);
  for (int i=0; i<SIZE; i++) {
    atomic_init(&element[i], 0);
  }
}

int empty(void)
{
  int _head, _tail, _amount;
  _head = atomic_load_explicit(&head, memory_order_seq_cst);
  _tail = atomic_load_explicit(&tail, memory_order_seq_cst);
  _amount = atomic_load_explicit(&amount, memory_order_seq_cst);
  
  if (_head == _tail && _amount==0) {
    return EMPTY;
  } else
    return 0;
}

int full(void)
{
  int _amount;
  _amount = atomic_load_explicit(&amount, memory_order_seq_cst);
  if (_amount == SIZE) {
    return FULL;
  } else
    return 0;
}

int enqueue(int x)
{
  int _tail, _amount;
  
  _tail = atomic_load_explicit(&tail, memory_order_seq_cst);
  atomic_store_explicit(&element[_tail], x, memory_order_seq_cst);
  
  _amount = atomic_load_explicit(&amount, memory_order_seq_cst);
  atomic_store_explicit(&amount, _amount+1, memory_order_seq_cst);
  
  _tail = atomic_load_explicit(&tail, memory_order_seq_cst);
  if (_tail == SIZE-1) {
    atomic_store_explicit(&tail, 0, memory_order_seq_cst);
  } else {
    _tail = atomic_load_explicit(&tail, memory_order_seq_cst);
    atomic_store_explicit(&tail, _tail+1, memory_order_seq_cst);
 	}
  
 	return 0;
}


int dequeue()
{
  int x, _head, _amount;
  
  _head = atomic_load_explicit(&head, memory_order_seq_cst);
  x = atomic_load_explicit(&element[_head], memory_order_seq_cst);
  
  _amount = atomic_load_explicit(&amount, memory_order_seq_cst);
 	atomic_store_explicit(&amount, _amount-1, memory_order_seq_cst);
  
  _head = atomic_load_explicit(&head, memory_order_seq_cst);
  if (_head == SIZE-1) {
    atomic_store_explicit(&head, 0, memory_order_seq_cst);
  } else {
  		_head = atomic_load_explicit(&head, memory_order_seq_cst);
    atomic_store_explicit(&head, _head+1, memory_order_seq_cst);
  }
  
  return x;
}

void *enqueuethread(void *arg)
{
  int value, i, _enqueue_flag;
  int enqueue_number = 0;
  value = *((int *)arg);
  
  pthread_mutex_lock(&lock);
  _enqueue_flag = atomic_load_explicit(&enqueue_flag, memory_order_seq_cst);
  
  if (_enqueue_flag)
  {
    for(i=0; i<SIZE; i++)
    {
      if (full() != FULL) {
        value += enqueue_number;
        enqueue_number++;
        enqueue(value);
        atomic_store_explicit(&stored_elements[i], value, memory_order_seq_cst);
        if (empty()) assert(0);
      }
    }
    atomic_store_explicit(&enqueue_flag, FALSE, memory_order_seq_cst);
    atomic_store_explicit(&dequeue_flag, TRUE, memory_order_seq_cst);
  }
  
  pthread_mutex_unlock(&lock);
  
  return NULL;
}


void *dequeuethread(void *arg)
{
  int i, _dequeue_flag, _stored_elements_i;
  
  pthread_mutex_lock(&lock);
  _dequeue_flag = atomic_load_explicit(&dequeue_flag, memory_order_seq_cst);
  
  if (_dequeue_flag)
  {
    for(i=0; i<SIZE; i++)
    {
      if (empty()!=EMPTY) {
        _stored_elements_i = atomic_load_explicit(&stored_elements[i], memory_order_seq_cst);
        if (!(dequeue() == _stored_elements_i)) {
          assert(0);
        }
      }
    }
    
    atomic_store_explicit(&dequeue_flag, FALSE, memory_order_seq_cst);
    atomic_store_explicit(&enqueue_flag, TRUE, memory_order_seq_cst);
  }
  
  pthread_mutex_unlock(&lock);
  
  return NULL;
}


int arg[N];
int main(int argc, char *argv[])
{
  int i;
  pthread_t t1s[N], t2s[N];
  
  atomic_init(&enqueue_flag, TRUE);
  atomic_init(&dequeue_flag, FALSE);
  pthread_mutex_init (&lock, NULL);
  
  init();
  
  if (!(empty()==EMPTY)) {
    assert(0);
  }
  
  for (i=0; i<SIZE; i++) {
    atomic_init(&stored_elements[i], 0);
  }
  
  for (i=0; i<N; i++) {
    arg[i] = i;
    pthread_create(&t1s[i], NULL, enqueuethread, &arg[i]);
  }
  
  for (i=0; i<N; i++) {
    pthread_create(&t2s[i], NULL, dequeuethread, NULL);
  }
  
  return 0;
}
