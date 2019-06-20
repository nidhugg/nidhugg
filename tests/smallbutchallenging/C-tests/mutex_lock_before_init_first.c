#include <pthread.h>

pthread_mutex_t m;
int x;

static void *t(void *arg) {
  pthread_mutex_init(&m, NULL);
  return NULL;
}

int main(int argc, char *argv[]) {
  pthread_t tid;
  pthread_create(&tid, NULL, t, NULL);

  pthread_mutex_lock(&m);
  ++x;
  pthread_mutex_unlock(&m);
}
