/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */

/* There are 2N^2+N+1 weak traces */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#ifndef N
#  warning "N was not defined"
#  define N 5
#endif

atomic_int vars[1];
atomic_int atom_1_r1_1;
atomic_int atom_1_r1_2;

void *writer(void *arg){
  int tid = *((int *)arg);
  atomic_store_explicit(&vars[0], tid, memory_order_seq_cst);
  atomic_store_explicit(&vars[0], tid, memory_order_seq_cst);
  
  return NULL;
}


void *reader(void *arg){
  atomic_store_explicit(&vars[0], 3, memory_order_seq_cst);
  int v2_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v12 = (v2_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v12, memory_order_seq_cst);
  int v13 = (v2_r1 == 2);
  atomic_store_explicit(&atom_1_r1_2, v13, memory_order_seq_cst);
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
  
  int v3 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v4 = (v3 == 3);
  int v5 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v6 = atomic_load_explicit(&atom_1_r1_2, memory_order_seq_cst);
//  int v7_disj = v5 | v6;
//  int v8_conj = v4 & v7_disj;
  int v9 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
//  int v10 = (v9 == 1);
//  int v11_disj = v8_conj | v10;
//  if (v11_disj == 1) assert(0);
  return 0;
}
