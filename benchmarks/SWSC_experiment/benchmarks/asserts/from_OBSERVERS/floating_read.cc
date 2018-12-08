#include <assert.h>
#include <atomic>
#include <pthread.h>

#ifndef N
#  warning "N was not defined, assuming 3"
#  define N 3
#endif

std::atomic<int> x;

static void *thread(void *arg) {
  int tid = *((int *)arg);
  x.store(tid, std::memory_order_seq_cst);
  return NULL;
}

int arg[N+1];
int main(int argc, char **argv) {
  pthread_t t[N+1];
  atomic_init(&x, 0);
  
  for (int i = 1; i <= N; i++) {
    arg[i]=i;
    pthread_create(&t[i], NULL, thread, &arg[i]);
  }
  
  assert(x.load(std::memory_order_seq_cst) < N+1);
  
  for (int i = 1; i <= N; i++)
    pthread_join(t[i], NULL);
  
  return 0;
}
