#include <pthread.h>
#include <stdatomic.h>

#define NUM_DATA 8

struct data {
  int field;
} data[NUM_DATA];

atomic_int x;

void *p(void *arg) {
  x = NUM_DATA + 1;
  return arg;
}

void problem() {
  struct data *p = data;
  for (int i = 0; i < NUM_DATA - 2; ++i) {
    while (i > p->field) p++;
    x = 42;
  }
}

int main(int argc, char *argv[]) {
  for (int i = 0; i < NUM_DATA; ++i) data[i].field = i;
  problem();
  pthread_t pt;
  pthread_create(&pt, NULL, p, NULL);
  return x;
}

