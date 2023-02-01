// nidhuggc: -sc -optimal
#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<assert.h>
#include<stdatomic.h>

#ifndef N
#  warning "N was not defined; defining it as 2"
#  define N 2
#endif

////////////////////////////////////////////////////////////////////////////////

/* The payload type passed to our messages */
struct _msg_arg {
  void (*func)(void *);
  void *arg;
  pthread_mutex_t *tid;
};

//int dummy = 0;

static void *_pthread_func(void *_msg_arg){
  struct _msg_arg *msg_arg = (struct _msg_arg *)_msg_arg;
  void (*func)(void *) = msg_arg->func;
  void *arg = msg_arg->arg;
  pthread_mutex_t *mutex = msg_arg->tid;
  pthread_mutex_lock(mutex);
  (*func)(arg);
  pthread_mutex_unlock(mutex);
  free(msg_arg);
  return NULL;
}

static void qthread_post_event(pthread_mutex_t *tid, void (*func)(void *), void *arg){
  pthread_t msg;
  struct _msg_arg *msg_arg = malloc(sizeof(struct _msg_arg));
  msg_arg->func = func;
  msg_arg->arg = arg;
  msg_arg->tid = tid;
  pthread_create(&msg, NULL, _pthread_func, msg_arg);
}

////////////////////////////////////////////////////////////////////////////////

struct device{
  atomic_int owner;
};

typedef struct{
  pthread_mutex_t *handler;
  atomic_int gid;
  int fd;
  int to_write;
} arg_t;

struct device dev;
atomic_int state = 0;
int num = 0;
int num_msg = 0;

void write(void *arg) {
}

void new_client(void *handler){
  atomic_int owner = atomic_load_explicit(&dev.owner, memory_order_seq_cst);
  if(owner > 0 && num_msg <= N){
    num_msg++;
  }else{
    atomic_store_explicit(&state, 0, memory_order_seq_cst);
    qthread_post_event(handler, &write, NULL);
  }
}

void listen(void *socket){
  pthread_mutex_t *handler = malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(handler, NULL);
  qthread_post_event(handler, &new_client, handler);
  num++;
  if(num < N) qthread_post_event(socket, &listen, socket);
}

int main(){
  atomic_store_explicit(&(dev.owner), 0, memory_order_seq_cst);
  pthread_mutex_t *socket = malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(socket, NULL);
  qthread_post_event(socket, &listen, socket);
  return 0;
}
