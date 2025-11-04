/* Adapted from: https://svn.sosy-lab.org/software/sv-benchmarks/trunk/c/pthread/fib_bench_true-unreach-call.c */

#include <assert.h>
#include <atomic>
#include <pthread.h>

std::atomic<int> i, j;

#ifndef N
#  warning "N not defined, assuming 3"
#  define N 3
#endif

void *t1(void* arg){
  int k = 0;
  
  for (k = 0; k < N; k++)
    atomic_store(&i, atomic_load(&i) + atomic_load(&j));
  
  return NULL;
}

void *t2(void* arg){
  int k = 0;
  
  for (k = 0; k < N; k++)
    atomic_store(&j, atomic_load(&j) + atomic_load(&i));
  
  return NULL;
}

int fib(int n) {
  int cur = 1, prev = 0;
  while(n--) {
    int next = prev+cur;
    prev = cur;
    cur = next;
  }
  return prev;
}

int main(int argc, char **argv) {
  pthread_t id1, id2;
  
  atomic_init(&i, 1);
  atomic_init(&j, 1);
  
  pthread_create(&id1, NULL, t1, NULL);
  pthread_create(&id2, NULL, t2, NULL);
  
  int correct = fib(2+2*N);
  assert(atomic_load(&i) <= correct && atomic_load(&j) <= correct);
  
  return 0;
}
