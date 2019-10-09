/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[1]; 
atomic_int atom_0_r3_3; 
atomic_int atom_1_r3_3; 
atomic_int atom_1_r3_2; 
atomic_int atom_0_r3_1; 
atomic_int atom_1_r3_1; 
atomic_int atom_0_r3_4; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  int v2_r3 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);
  int v38 = (v2_r3 == 3);
  atomic_store_explicit(&atom_0_r3_3, v38, memory_order_seq_cst);
  int v41 = (v2_r3 == 1);
  atomic_store_explicit(&atom_0_r3_1, v41, memory_order_seq_cst);
  int v43 = (v2_r3 == 4);
  atomic_store_explicit(&atom_0_r3_4, v43, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  atomic_store_explicit(&vars[0], 3, memory_order_seq_cst);
  int v4_r3 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  atomic_store_explicit(&vars[0], 4, memory_order_seq_cst);
  int v39 = (v4_r3 == 3);
  atomic_store_explicit(&atom_1_r3_3, v39, memory_order_seq_cst);
  int v40 = (v4_r3 == 2);
  atomic_store_explicit(&atom_1_r3_2, v40, memory_order_seq_cst);
  int v42 = (v4_r3 == 1);
  atomic_store_explicit(&atom_1_r3_1, v42, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  atomic_init(&vars[0], 0);
  atomic_init(&atom_0_r3_3, 0);
  atomic_init(&atom_1_r3_3, 0);
  atomic_init(&atom_1_r3_2, 0);
  atomic_init(&atom_0_r3_1, 0);
  atomic_init(&atom_1_r3_1, 0);
  atomic_init(&atom_0_r3_4, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v5 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v6 = (v5 == 4);
  int v7 = atomic_load_explicit(&atom_0_r3_3, memory_order_seq_cst);
  int v8 = atomic_load_explicit(&atom_1_r3_3, memory_order_seq_cst);
  int v9 = atomic_load_explicit(&atom_1_r3_2, memory_order_seq_cst);
  int v10_disj = v8 | v9;
  int v11_conj = v7 & v10_disj;
  int v12 = atomic_load_explicit(&atom_0_r3_1, memory_order_seq_cst);
  int v13 = atomic_load_explicit(&atom_1_r3_3, memory_order_seq_cst);
  int v14 = atomic_load_explicit(&atom_1_r3_2, memory_order_seq_cst);
  int v15 = atomic_load_explicit(&atom_1_r3_1, memory_order_seq_cst);
  int v16_disj = v14 | v15;
  int v17_disj = v13 | v16_disj;
  int v18_conj = v12 & v17_disj;
  int v19_disj = v11_conj | v18_conj;
  int v20_conj = v6 & v19_disj;
  int v21 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v22 = (v21 == 2);
  int v23 = atomic_load_explicit(&atom_1_r3_3, memory_order_seq_cst);
  int v24 = atomic_load_explicit(&atom_0_r3_4, memory_order_seq_cst);
  int v25 = atomic_load_explicit(&atom_0_r3_3, memory_order_seq_cst);
  int v26 = atomic_load_explicit(&atom_0_r3_1, memory_order_seq_cst);
  int v27_disj = v25 | v26;
  int v28_disj = v24 | v27_disj;
  int v29_conj = v23 & v28_disj;
  int v30 = atomic_load_explicit(&atom_1_r3_1, memory_order_seq_cst);
  int v31 = atomic_load_explicit(&atom_0_r3_4, memory_order_seq_cst);
  int v32 = atomic_load_explicit(&atom_0_r3_1, memory_order_seq_cst);
  int v33_disj = v31 | v32;
  int v34_conj = v30 & v33_disj;
  int v35_disj = v29_conj | v34_conj;
  int v36_conj = v22 & v35_disj;
  int v37_disj = v20_conj | v36_conj;
  if (v37_disj == 0) assert(0);
  return 0;
}
