/* Copyright (C) 2018
 * This benchmark is part of SWSC */

#include <stdatomic.h>
#include <pthread.h>

#ifdef N
#  if N != 1
#    error "No other values than N=1 implemented"
#  endif
#endif

atomic_int x, y, z;

void *t0(void *arg){
  z = 1;
  return NULL;
}

void *t1(void *arg){
  (void)z;
  y = 1;
  return NULL;
}

void *t2(void *arg){
  (void)y;
  x = 1;
  return NULL;
}

void *t3(void *arg){
  x = 2;
  return NULL;
}

void *t4(void *arg){
  (void)x;
  y = 2;
  (void)z;
  return NULL;
}

void *t5(void *arg){
  (void)x;
  z = 2;
  (void)y;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr[6];

  pthread_create(thr,   NULL, t0, NULL);
  pthread_create(thr+1, NULL, t1, NULL);
  pthread_create(thr+2, NULL, t2, NULL);
  pthread_create(thr+3, NULL, t3, NULL);
  pthread_create(thr+4, NULL, t4, NULL);
  pthread_create(thr+5, NULL, t5, NULL);

  for (unsigned i = 0; i < 6; ++i)
    pthread_join(thr[i], NULL);

  return 0;
}
