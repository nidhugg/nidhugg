/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[1]; 
atomic_int atom_0_r1_2; 
atomic_int atom_0_r2_2; 
atomic_int atom_1_r1_2; 
atomic_int atom_1_r2_2; 
atomic_int atom_0_r1_1; 
atomic_int atom_0_r2_1; 
atomic_int atom_1_r2_1; 
atomic_int atom_1_r1_1; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  int v2_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v4_r2 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v36 = (v2_r1 == 2);
  atomic_store_explicit(&atom_0_r1_2, v36, memory_order_seq_cst);
  int v37 = (v4_r2 == 2);
  atomic_store_explicit(&atom_0_r2_2, v37, memory_order_seq_cst);
  int v40 = (v2_r1 == 1);
  atomic_store_explicit(&atom_0_r1_1, v40, memory_order_seq_cst);
  int v41 = (v4_r2 == 1);
  atomic_store_explicit(&atom_0_r2_1, v41, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);
  int v6_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v8_r2 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v38 = (v6_r1 == 2);
  atomic_store_explicit(&atom_1_r1_2, v38, memory_order_seq_cst);
  int v39 = (v8_r2 == 2);
  atomic_store_explicit(&atom_1_r2_2, v39, memory_order_seq_cst);
  int v42 = (v8_r2 == 1);
  atomic_store_explicit(&atom_1_r2_1, v42, memory_order_seq_cst);
  int v43 = (v6_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v43, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  atomic_init(&vars[0], 0);
  atomic_init(&atom_0_r1_2, 0);
  atomic_init(&atom_0_r2_2, 0);
  atomic_init(&atom_1_r1_2, 0);
  atomic_init(&atom_1_r2_2, 0);
  atomic_init(&atom_0_r1_1, 0);
  atomic_init(&atom_0_r2_1, 0);
  atomic_init(&atom_1_r2_1, 0);
  atomic_init(&atom_1_r1_1, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v9 = atomic_load_explicit(&atom_0_r1_2, memory_order_seq_cst);
  int v10 = atomic_load_explicit(&atom_0_r2_2, memory_order_seq_cst);
  int v11 = atomic_load_explicit(&atom_1_r1_2, memory_order_seq_cst);
  int v12 = atomic_load_explicit(&atom_1_r2_2, memory_order_seq_cst);
  int v13_conj = v11 & v12;
  int v14_conj = v10 & v13_conj;
  int v15_conj = v9 & v14_conj;
  int v16 = atomic_load_explicit(&atom_0_r1_1, memory_order_seq_cst);
  int v17 = atomic_load_explicit(&atom_0_r2_2, memory_order_seq_cst);
  int v18 = atomic_load_explicit(&atom_1_r1_2, memory_order_seq_cst);
  int v19 = atomic_load_explicit(&atom_1_r2_2, memory_order_seq_cst);
  int v20_conj = v18 & v19;
  int v21_conj = v17 & v20_conj;
  int v22 = atomic_load_explicit(&atom_0_r2_1, memory_order_seq_cst);
  int v23 = atomic_load_explicit(&atom_1_r1_2, memory_order_seq_cst);
  int v24 = atomic_load_explicit(&atom_1_r2_2, memory_order_seq_cst);
  int v25 = atomic_load_explicit(&atom_1_r2_1, memory_order_seq_cst);
  int v26_disj = v24 | v25;
  int v27_conj = v23 & v26_disj;
  int v28 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v29 = atomic_load_explicit(&atom_1_r2_1, memory_order_seq_cst);
  int v30_conj = v28 & v29;
  int v31_disj = v27_conj | v30_conj;
  int v32_conj = v22 & v31_disj;
  int v33_disj = v21_conj | v32_conj;
  int v34_conj = v16 & v33_disj;
  int v35_disj = v15_conj | v34_conj;
  if (v35_disj == 0) assert(0);
  return 0;
}
