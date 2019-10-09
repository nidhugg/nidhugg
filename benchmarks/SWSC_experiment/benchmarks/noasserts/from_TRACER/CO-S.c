/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */

/* There are 2N^2+2N+1 weak traces */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#ifndef N
#  warning "N was not defined"
#  define N 5
#endif

atomic_int vars[1];
atomic_int atom_1_r0_2;
atomic_int atom_1_r0_1;

void *writer(void *arg){
  int tid = *((int *)arg);
  atomic_store_explicit(&vars[0], tid, memory_order_seq_cst);
  atomic_store_explicit(&vars[0], tid, memory_order_seq_cst);
  
  return NULL;
}


void *reader(void *arg){
  int v2_r0 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  atomic_store_explicit(&vars[0], 3, memory_order_seq_cst);
  int v12 = (v2_r0 == 2);
  atomic_store_explicit(&atom_1_r0_2, v12, memory_order_seq_cst);
  int v13 = (v2_r0 == 1);
  atomic_store_explicit(&atom_1_r0_1, v13, memory_order_seq_cst);
  return NULL;
}

int arg[N];
int main(int argc, char **argv){
  pthread_t ws[N];
  pthread_t r;
  
  atomic_init(&vars[0], 0);
  
  for (int i=0; i<N; i++) {
    arg[i]=i;
    pthread_create(&ws[i], NULL, writer, &arg[i]);
  }
  
  pthread_create(&r, NULL, reader, NULL);
  
  for (int i=0; i<N; i++) {
    pthread_join(ws[i], NULL);
  }
  
  pthread_join(r, NULL);
  
  int v3 = atomic_load_explicit(&atom_1_r0_2, memory_order_seq_cst);
  int v4 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v5 = (v4 == 2);
  int v6 = atomic_load_explicit(&atom_1_r0_1, memory_order_seq_cst);
  int v7 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
//  int v8 = (v7 == 1);
//  int v9_conj = v6 & v8;
//  int v10_disj = v5 | v9_conj;
//  int v11_conj = v3 & v10_disj;
//  if (v11_conj == 1) assert(0);
  return 0;
}
