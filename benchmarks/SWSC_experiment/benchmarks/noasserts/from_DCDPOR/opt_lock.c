#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>

#define T 2
#ifndef N
#  warning "N not defined, assuming 6"
#  define N 6
#endif

atomic_int x[1], y[1];

void *opt_lock(void *arg){
  int id = (intptr_t)arg;

  for(int i = 0; i<N; ++i){
    atomic_store(x, id);
    atomic_store(y, 1);

    if(atomic_load(x) == id){
      atomic_load(y);
      break;
    }
  }

  return NULL;
}

int main(int argc, char *argv[]){
  int i;
  pthread_t threads[T+1];

  for(i=0; i<T; i++){
    pthread_create(&threads[i],NULL,opt_lock,(void*)(intptr_t)i);
  }
  return 0;
}
