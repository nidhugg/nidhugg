/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[1]; 
atomic_int atom_0_r1_0; 
atomic_int atom_1_r3_1; 
atomic_int atom_1_r1_1; 
atomic_int atom_1_r1_0; 
atomic_int atom_1_r3_0; 
atomic_int atom_0_r1_2; 

void *t0(void *arg){
label_1:;
  int v2_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  int v32 = (v2_r1 == 0);
  atomic_store_explicit(&atom_0_r1_0, v32, memory_order_seq_cst);
  int v37 = (v2_r1 == 2);
  atomic_store_explicit(&atom_0_r1_2, v37, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v4_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v6_r3 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);
  int v33 = (v6_r3 == 1);
  atomic_store_explicit(&atom_1_r3_1, v33, memory_order_seq_cst);
  int v34 = (v4_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v34, memory_order_seq_cst);
  int v35 = (v4_r1 == 0);
  atomic_store_explicit(&atom_1_r1_0, v35, memory_order_seq_cst);
  int v36 = (v6_r3 == 0);
  atomic_store_explicit(&atom_1_r3_0, v36, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  atomic_init(&vars[0], 0);
  atomic_init(&atom_0_r1_0, 0);
  atomic_init(&atom_1_r3_1, 0);
  atomic_init(&atom_1_r1_1, 0);
  atomic_init(&atom_1_r1_0, 0);
  atomic_init(&atom_1_r3_0, 0);
  atomic_init(&atom_0_r1_2, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v7 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v8 = (v7 == 2);
  int v9 = atomic_load_explicit(&atom_0_r1_0, memory_order_seq_cst);
  int v10 = atomic_load_explicit(&atom_1_r3_1, memory_order_seq_cst);
  int v11 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v12 = atomic_load_explicit(&atom_1_r1_0, memory_order_seq_cst);
  int v13_disj = v11 | v12;
  int v14_conj = v10 & v13_disj;
  int v15 = atomic_load_explicit(&atom_1_r3_0, memory_order_seq_cst);
  int v16 = atomic_load_explicit(&atom_1_r1_0, memory_order_seq_cst);
  int v17_conj = v15 & v16;
  int v18_disj = v14_conj | v17_conj;
  int v19_conj = v9 & v18_disj;
  int v20_conj = v8 & v19_conj;
  int v21 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v22 = (v21 == 1);
  int v23 = atomic_load_explicit(&atom_1_r3_0, memory_order_seq_cst);
  int v24 = atomic_load_explicit(&atom_1_r1_0, memory_order_seq_cst);
  int v25 = atomic_load_explicit(&atom_0_r1_2, memory_order_seq_cst);
  int v26 = atomic_load_explicit(&atom_0_r1_0, memory_order_seq_cst);
  int v27_disj = v25 | v26;
  int v28_conj = v24 & v27_disj;
  int v29_conj = v23 & v28_conj;
  int v30_conj = v22 & v29_conj;
  int v31_disj = v20_conj | v30_conj;
  if (v31_disj == 0) assert(0);
  return 0;
}
