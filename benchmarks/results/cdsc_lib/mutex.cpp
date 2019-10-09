#include <mutex>
extern "C" {
#include <errno.h>
}
#include "../cdsc_include/_cdsc_mutex.h"

struct __pthread_mutex_t_impl {
    std::mutex mutex;
};

int pthread_mutex_init(pthread_mutex_t *mutex,
                       const pthread_mutexattr_t *attr) {
    mutex->impl = new __pthread_mutex_t_impl();
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex) {
    delete mutex->impl;
    mutex->impl = nullptr;
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex) {
    mutex->impl->mutex.lock();
    return 0;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex) {
    return mutex->impl->mutex.try_lock() ? 0 : EBUSY;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    mutex->impl->mutex.unlock();
    return 0;
}
