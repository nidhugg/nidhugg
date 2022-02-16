// nidhuggc: -sc -optimal -no-assume-await

#include <pthread.h>
#include <stdatomic.h>

struct s {
  atomic_int x;
} s;

void *p(void *arg) {
  s.x = 1;
  return arg;
}

static void increment(struct s *sp) {
  int expected = sp->x;
  while (!atomic_compare_exchange_strong(&sp->x, &expected, expected+1));
};

int main(int argc, char *argv[]) {
  pthread_t pt[2];
  for (unsigned i = 0; i < 2; ++i) pthread_create(pt+i, NULL, p, NULL);

  increment(&s);

  for (unsigned i = 0; i < 2; ++i) pthread_join(pt[i], NULL);
  return s.x;
}
