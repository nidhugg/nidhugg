/* Implements approximate list insertion as per
 * Parallel Synchronization-Free Approximate Data Structure Construction (HotPar'13)
 * https://www.usenix.org/conference/hotpar13/workshop-program/presentation/rinard
 *
 * N threads try to append one element each onto a list with M slots.
 * The main thread then joins the others and reads out the contents of
 * the list
 */

#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef N
#  define N 4
#endif

#ifndef M
# define M (N-2)
#endif

typedef int E;
typedef atomic_int atomic_E;

struct list {
  atomic_E elements[M];
  atomic_int next;
};

static void init(struct list *l) {
  for (unsigned i = 0; i < M; ++i) atomic_init(l->elements + i, 0);
  atomic_init(&l->next, 0);
}

static bool append(struct list *l, E e) {
  int i = atomic_load(&(l->next));
  if (M <= i) return false;
  atomic_store(l->elements + i, e);
  atomic_store(&(l->next), i + 1);
  return true;
}

static bool pop(struct list *l, E *e) {
  int i = atomic_load(&(l->next));
  do {
    if (i == 0) return false;
    *e = atomic_load(l->elements+(i-1));
  } while (atomic_load(&(l->next)) != i);
  atomic_store(&(l->next), i-1);
  return true;
}

static struct list l;

static void *try_append(void *arg) {
  append(&l, (intptr_t)arg);
  return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t ts[N];

    init(&l);

    for (unsigned i = 0; i < N; ++i)
      pthread_create(ts+i, NULL, try_append, (void*)(intptr_t)i);

    for (unsigned i = 0; i < N; ++i)
      pthread_join(ts[i], NULL);

    int scratch;
    while(pop(&l, &scratch));

    return 0;
}
