/* This benchmark is from the following JACM journal article
   https://dl.acm.org/citation.cfm?id=3073408
*/

#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#ifndef N
#  warning "N was not defined"
#  define N 5
#endif

atomic_int array[N+1];

void *thread_reader(void *unused) {
  for (int i = N; atomic_load(array+i) != 0; --i);
  return NULL;
}

void *thread_writer(void *arg) {
  int j = (intptr_t)arg;

  atomic_store(array+j, atomic_load(array+j-1) + 1);
  return NULL;
}


int main(int argc, char **argv) {
  pthread_t t[N+1];

#ifdef CDSC
  for (int i = 0; i <= N; i++)
    atomic_init(&array[i], 0);
#endif

  for (int i = 0; i <= N; i++) {
    if (i == 0) {
      pthread_create(&t[i], NULL, thread_reader, (void*)(intptr_t)i);
    } else {
      pthread_create(&t[i], NULL, thread_writer, (void*)(intptr_t)i);
    }
  }

  return 0;
}
