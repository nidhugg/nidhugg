/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[1]; 
atomic_int atom_0_r2_2; 
atomic_int atom_0_r1_2; 
atomic_int atom_1_r2_1; 
atomic_int atom_1_r1_1; 
atomic_int atom_0_r2_1; 
atomic_int atom_1_r2_2; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  int v2_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v4_r2 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v24 = (v4_r2 == 2);
  atomic_store_explicit(&atom_0_r2_2, v24, memory_order_seq_cst);
  int v25 = (v2_r1 == 2);
  atomic_store_explicit(&atom_0_r1_2, v25, memory_order_seq_cst);
  int v28 = (v4_r2 == 1);
  atomic_store_explicit(&atom_0_r2_1, v28, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);
  int v6_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v8_r2 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v26 = (v8_r2 == 1);
  atomic_store_explicit(&atom_1_r2_1, v26, memory_order_seq_cst);
  int v27 = (v6_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v27, memory_order_seq_cst);
  int v29 = (v8_r2 == 2);
  atomic_store_explicit(&atom_1_r2_2, v29, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  atomic_init(&vars[0], 0);
  atomic_init(&atom_0_r2_2, 0);
  atomic_init(&atom_0_r1_2, 0);
  atomic_init(&atom_1_r2_1, 0);
  atomic_init(&atom_1_r1_1, 0);
  atomic_init(&atom_0_r2_1, 0);
  atomic_init(&atom_1_r2_2, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v9 = atomic_load_explicit(&atom_0_r2_2, memory_order_seq_cst);
  int v10 = atomic_load_explicit(&atom_0_r1_2, memory_order_seq_cst);
  int v11_disj = v9 | v10;
  int v12 = atomic_load_explicit(&atom_1_r2_1, memory_order_seq_cst);
  int v13 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v14_disj = v12 | v13;
  int v15_conj = v11_disj & v14_disj;
  int v16 = atomic_load_explicit(&atom_0_r1_2, memory_order_seq_cst);
  int v17 = atomic_load_explicit(&atom_0_r2_1, memory_order_seq_cst);
  int v18_conj = v16 & v17;
  int v19 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v20 = atomic_load_explicit(&atom_1_r2_2, memory_order_seq_cst);
  int v21_conj = v19 & v20;
  int v22_disj = v18_conj | v21_conj;
  int v23_disj = v15_conj | v22_disj;
  if (v23_disj == 1) assert(0);
  return 0;
}
