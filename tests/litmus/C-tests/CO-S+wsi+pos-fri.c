/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[1]; 
atomic_int atom_1_r3_2; 
atomic_int atom_1_r1_2; 
atomic_int atom_1_r1_1; 
atomic_int atom_1_r1_0; 
atomic_int atom_1_r3_1; 
atomic_int atom_1_r3_0; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v4_r3 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  atomic_store_explicit(&vars[0], 3, memory_order_seq_cst);
  int v38 = (v4_r3 == 2);
  atomic_store_explicit(&atom_1_r3_2, v38, memory_order_seq_cst);
  int v39 = (v2_r1 == 2);
  atomic_store_explicit(&atom_1_r1_2, v39, memory_order_seq_cst);
  int v40 = (v2_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v40, memory_order_seq_cst);
  int v41 = (v2_r1 == 0);
  atomic_store_explicit(&atom_1_r1_0, v41, memory_order_seq_cst);
  int v42 = (v4_r3 == 1);
  atomic_store_explicit(&atom_1_r3_1, v42, memory_order_seq_cst);
  int v43 = (v4_r3 == 0);
  atomic_store_explicit(&atom_1_r3_0, v43, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  atomic_init(&vars[0], 0);
  atomic_init(&atom_1_r3_2, 0);
  atomic_init(&atom_1_r1_2, 0);
  atomic_init(&atom_1_r1_1, 0);
  atomic_init(&atom_1_r1_0, 0);
  atomic_init(&atom_1_r3_1, 0);
  atomic_init(&atom_1_r3_0, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v5 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v6 = (v5 == 3);
  int v7 = atomic_load_explicit(&atom_1_r3_2, memory_order_seq_cst);
  int v8 = atomic_load_explicit(&atom_1_r1_2, memory_order_seq_cst);
  int v9 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v10 = atomic_load_explicit(&atom_1_r1_0, memory_order_seq_cst);
  int v11_disj = v9 | v10;
  int v12_disj = v8 | v11_disj;
  int v13_conj = v7 & v12_disj;
  int v14 = atomic_load_explicit(&atom_1_r3_1, memory_order_seq_cst);
  int v15 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v16 = atomic_load_explicit(&atom_1_r1_0, memory_order_seq_cst);
  int v17_disj = v15 | v16;
  int v18_conj = v14 & v17_disj;
  int v19 = atomic_load_explicit(&atom_1_r3_0, memory_order_seq_cst);
  int v20 = atomic_load_explicit(&atom_1_r1_0, memory_order_seq_cst);
  int v21_conj = v19 & v20;
  int v22_disj = v18_conj | v21_conj;
  int v23_disj = v13_conj | v22_disj;
  int v24_conj = v6 & v23_disj;
  int v25 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v26 = (v25 == 2);
  int v27 = atomic_load_explicit(&atom_1_r3_1, memory_order_seq_cst);
  int v28 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v29 = atomic_load_explicit(&atom_1_r1_0, memory_order_seq_cst);
  int v30_disj = v28 | v29;
  int v31_conj = v27 & v30_disj;
  int v32 = atomic_load_explicit(&atom_1_r3_0, memory_order_seq_cst);
  int v33 = atomic_load_explicit(&atom_1_r1_0, memory_order_seq_cst);
  int v34_conj = v32 & v33;
  int v35_disj = v31_conj | v34_conj;
  int v36_conj = v26 & v35_disj;
  int v37_disj = v24_conj | v36_conj;
  if (v37_disj == 0) assert(0);
  return 0;
}
