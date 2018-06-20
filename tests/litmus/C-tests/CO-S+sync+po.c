/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[1]; 
atomic_int atom_1_r0_2; 
atomic_int atom_1_r0_1; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);

  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r0 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  atomic_store_explicit(&vars[0], 3, memory_order_seq_cst);
  int v12 = (v2_r0 == 2);
  atomic_store_explicit(&atom_1_r0_2, v12, memory_order_seq_cst);
  int v13 = (v2_r0 == 1);
  atomic_store_explicit(&atom_1_r0_1, v13, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  atomic_init(&vars[0], 0);
  atomic_init(&atom_1_r0_2, 0);
  atomic_init(&atom_1_r0_1, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v3 = atomic_load_explicit(&atom_1_r0_2, memory_order_seq_cst);
  int v4 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v5 = (v4 == 2);
  int v6 = atomic_load_explicit(&atom_1_r0_1, memory_order_seq_cst);
  int v7 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v8 = (v7 == 1);
  int v9_conj = v6 & v8;
  int v10_disj = v5 | v9_conj;
  int v11_conj = v3 & v10_disj;
  if (v11_conj == 1) assert(0);
  return 0;
}
