#ifdef __cplusplus
extern "C" {
#endif
typedef struct pthread_mutex {
  struct __pthread_mutex_t_impl *impl;
} pthread_mutex_t;

typedef void pthread_mutexattr_t;

#define pthread_mutex_init    _cdsc_mutex_init
#define pthread_mutex_destroy _cdsc_mutex_destroy
#define pthread_mutex_lock    _cdsc_mutex_lock
#define pthread_mutex_trylock _cdsc_mutex_trylock
#define pthread_mutex_unlock  _cdsc_mutex_unlock

int pthread_mutex_init(pthread_mutex_t * mutex,
		       const pthread_mutexattr_t * attr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
#ifdef __cplusplus
}
#endif
