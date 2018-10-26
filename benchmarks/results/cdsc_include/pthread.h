#include <threads.h>
typedef thrd_t pthread_t;
typedef void pthread_attr_t;

static inline int pthread_create(pthread_t *t, pthread_attr_t *attr,
                                 void *(*s)(void*), void* a) {
  return thrd_create(t, (thrd_start_t)s, a);
}
