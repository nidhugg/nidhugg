/* Race parametric, with added acyclicity asasertion
 * Type: Litmus
 * Required MM for safety: very weak (<RA)
 */

#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <pthread.h>

#ifndef N
#  warning "N was not defined, assuming 2"
#  define N 2
#endif

#define T 2
#define B 42

atomic_int v;
atomic_int local[T];
atomic_int result[T];

void *t(void* arg) {
  intptr_t tid = (intptr_t)arg;
  for (int i = 0; i < B; ++i) {
    atomic_store(local+tid, i);
    (void)atomic_load(local+tid);
  }
  for (int i = 0; i < N; ++i) {
    atomic_store(&v, tid);
    int val = atomic_load(&v);
    atomic_store(result+tid, val);
  }
  return NULL;
}

static int *outs[T];
static bool visited[T] = { 0 };
static bool dfs(int i) {
  if (visited[i]) return true;
  visited[i] = true;
  for (int j = 0; outs[i][j] != -1; ++j) {
    if (dfs(outs[i][j])) return true;
  }
  visited[i] = false;
  return false;
}

static bool acyclic() {
  int outbuf[T * 2 + 1];
  int *alloc = outbuf;
  for (int i = 0; i < T; ++i) {
    for (int j = 0; j < T; ++j) {
      if (atomic_load(result+j) == i && j != i) {
        *alloc++ = j;
      }
      *(outs[i] = alloc++) = -1;
    }
  }

  for (int i = 0; i < T; ++i) {
    if (dfs(i)) return false;
  }
  return true;
}

int main(int argc, char **argv) {
  pthread_t tid[N];
  for (int i = 0; i < T; ++i) {
    pthread_create(tid+i, NULL, t, (void*)(intptr_t)i);
  }

  for (int i = 0; i < T; ++i) {
    pthread_join(tid[i], NULL);
  }

  assert(acyclic());
  return 0;
}
