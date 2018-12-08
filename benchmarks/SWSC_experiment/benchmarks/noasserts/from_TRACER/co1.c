/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */

/* There are N^3+2N weak traces */


#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#ifndef N
#  warning "N was not defined"
#  define N 5
#endif

atomic_int  vars[1];
atomic_int  atom_2_r1_1;
atomic_int  atom_2_r2_2;
atomic_int  atom_2_r1_2;
atomic_int  atom_2_r2_1;
atomic_int  atom_2_r2_0;


void *writer(void *arg){
  int tid = *((int *)arg);
  atomic_store_explicit(&vars[0], tid, memory_order_seq_cst);
  
  return NULL;
}


void *reader(void *arg){
  int v2_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v4_r2 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v24 = (v2_r1 == 1);
  atomic_store_explicit(&atom_2_r1_1, v24, memory_order_seq_cst);
  int v25 = (v4_r2 == 2);
  atomic_store_explicit(&atom_2_r2_2, v25, memory_order_seq_cst);
  int v26 = (v2_r1 == 2);
  atomic_store_explicit(&atom_2_r1_2, v26, memory_order_seq_cst);
  int v27 = (v4_r2 == 1);
  atomic_store_explicit(&atom_2_r2_1, v27, memory_order_seq_cst);
  int v28 = (v4_r2 == 0);
  atomic_store_explicit(&atom_2_r2_0, v28, memory_order_seq_cst);
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
  
  int v5 = atomic_load_explicit(&atom_2_r1_1, memory_order_seq_cst);
  int v6 = atomic_load_explicit(&atom_2_r2_2, memory_order_seq_cst);
  int v7 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
//  int v8 = (v7 == 1);
//  int v9_conj = v6 & v8;
//  int v10_conj = v5 & v9_conj;
  int v11 = atomic_load_explicit(&atom_2_r1_2, memory_order_seq_cst);
  int v12 = atomic_load_explicit(&atom_2_r2_1, memory_order_seq_cst);
  int v13 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
//  int v14 = (v13 == 2);
//  int v15_conj = v12 & v14;
//  int v16_conj = v11 & v15_conj;
  int v17 = atomic_load_explicit(&atom_2_r1_1, memory_order_seq_cst);
  int v18 = atomic_load_explicit(&atom_2_r1_2, memory_order_seq_cst);
//  int v19_disj = v17 | v18;
  int v20 = atomic_load_explicit(&atom_2_r2_0, memory_order_seq_cst);
//  int v21_conj = v19_disj & v20;
//  int v22_disj = v16_conj | v21_conj;
//  int v23_disj = v10_conj | v22_disj;
  //if (v23_disj == 1) assert(0);
  return 0;
}
