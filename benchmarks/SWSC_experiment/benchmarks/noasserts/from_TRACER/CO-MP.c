/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */

/* There are 4N^2+N+1 weak traces */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#ifndef N
#  warning "N was not defined"
#  define N 5
#endif

atomic_int  vars[1];
atomic_int  atom_1_r0_2;
atomic_int  atom_1_r1_1;
atomic_int  atom_1_r1_0;
atomic_int  atom_1_r0_1;

void *writer(void *arg){
  int tid = *((int *)arg);
  atomic_store_explicit(&vars[0], tid, memory_order_seq_cst);
  atomic_store_explicit(&vars[0], tid, memory_order_seq_cst);
  
  return NULL;
}

void *reader(void *arg){
  int v2_r0 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v4_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v16 = (v2_r0 == 2);
  atomic_store_explicit(&atom_1_r0_2, v16, memory_order_seq_cst);
  int v17 = (v4_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v17, memory_order_seq_cst);
  int v18 = (v4_r1 == 0);
  atomic_store_explicit(&atom_1_r1_0, v18, memory_order_seq_cst);
  int v19 = (v2_r0 == 1);
  atomic_store_explicit(&atom_1_r0_1, v19, memory_order_seq_cst);
  return NULL;
}

int arg[N];
int main(int argc, char *argv[]){
  pthread_t ws[N];
  pthread_t r;
  
  atomic_init(&vars[0], 0);
  
  for (int i=0; i<N; i++) {
    arg[i]=i;
    pthread_create(&ws[i], NULL, writer, &arg[i]);
  }
  
  pthread_create(&r, NULL, reader, NULL);
  
  for (int i=0; i<N; i++)
    pthread_join(ws[i], NULL);
  
  pthread_join(r, NULL);
  
  int v5 = atomic_load_explicit(&atom_1_r0_2, memory_order_seq_cst);
  int v6 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v7 = atomic_load_explicit(&atom_1_r0_2, memory_order_seq_cst);
  int v8 = atomic_load_explicit(&atom_1_r1_0, memory_order_seq_cst);
  int v9 = atomic_load_explicit(&atom_1_r0_1, memory_order_seq_cst);
  int v10 = atomic_load_explicit(&atom_1_r1_0, memory_order_seq_cst);
//  int v11_conj = v9 & v10;
//  int v12_disj = v8 | v11_conj;
//  int v13_conj = v7 & v12_disj;
//  int v14_disj = v6 | v13_conj;
//  int v15_conj = v5 & v14_disj;
//  if (v15_conj == 1) assert(0);
  return 0;
}
