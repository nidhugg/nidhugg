/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[1]; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 

  atomic_init(&vars[0], 0);

  pthread_create(&thr0, NULL, t0, NULL);

  pthread_join(thr0, NULL);

  int v1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v2 = (v1 == 1);
  if (v2 == 1) assert(0);
  return 0;
}
