// nidhuggc: -tso
#include <pthread.h>
#include <stdint.h>
#include <assert.h>

#ifndef N
#  define N 1
#endif

volatile int wants_to_enter[2];
volatile int turn;
volatile int critical_section;

void *p(void *arg) {
  intptr_t i = (intptr_t)arg;

  for (unsigned times = 0; times < N; ++times) {
    // lock
    wants_to_enter[i] = 1;
    __asm__ volatile ("mfence");
    while (wants_to_enter[!i]) {
      __asm__ volatile ("");
      if (turn != i) {
	wants_to_enter[i] = 0;
	while (turn != i);
	wants_to_enter[i] = 1;
      }
      // __asm__ volatile ("mfence"); // To be supported by plp, needed for N>1
    }

    // cs
    critical_section = i;
    __asm__ volatile ("");
    assert(critical_section == i);

    // unlock
    turn = !i;
    wants_to_enter[i] = 0;
  }
  return NULL;
}

int main() {
  pthread_t ts[2];
  pthread_create(ts+0, NULL, p, (void*)(intptr_t)0);
  pthread_create(ts+1, NULL, p, (void*)(intptr_t)1);
  return 0;
}
