/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[1]; 
atomic_int atom_2_r1_1; 
atomic_int atom_2_r2_2; 
atomic_int atom_2_r1_2; 
atomic_int atom_2_r2_1; 
atomic_int atom_2_r2_0; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
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

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  atomic_init(&vars[0], 0);
  atomic_init(&atom_2_r1_1, 0);
  atomic_init(&atom_2_r2_2, 0);
  atomic_init(&atom_2_r1_2, 0);
  atomic_init(&atom_2_r2_1, 0);
  atomic_init(&atom_2_r2_0, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v5 = atomic_load_explicit(&atom_2_r1_1, memory_order_seq_cst);
  int v6 = atomic_load_explicit(&atom_2_r2_2, memory_order_seq_cst);
  int v7 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v8 = (v7 == 1);
  int v9_conj = v6 & v8;
  int v10_conj = v5 & v9_conj;
  int v11 = atomic_load_explicit(&atom_2_r1_2, memory_order_seq_cst);
  int v12 = atomic_load_explicit(&atom_2_r2_1, memory_order_seq_cst);
  int v13 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v14 = (v13 == 2);
  int v15_conj = v12 & v14;
  int v16_conj = v11 & v15_conj;
  int v17 = atomic_load_explicit(&atom_2_r1_1, memory_order_seq_cst);
  int v18 = atomic_load_explicit(&atom_2_r1_2, memory_order_seq_cst);
  int v19_disj = v17 | v18;
  int v20 = atomic_load_explicit(&atom_2_r2_0, memory_order_seq_cst);
  int v21_conj = v19_disj & v20;
  int v22_disj = v16_conj | v21_conj;
  int v23_disj = v10_conj | v22_disj;
  if (v23_disj == 1) assert(0);
  return 0;
}
