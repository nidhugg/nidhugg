/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[2]; 
atomic_int atom_0_r1_0; 
atomic_int atom_0_r2_0; 
atomic_int atom_1_r1_0; 
atomic_int atom_1_r2_2; 
atomic_int atom_0_r2_2; 
atomic_int atom_0_r1_1; 

void *t0(void *arg){
label_1:;
  int v2_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);

  int v4_r2 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v40 = (v2_r1 == 0);
  atomic_store_explicit(&atom_0_r1_0, v40, memory_order_seq_cst);
  int v41 = (v4_r2 == 0);
  atomic_store_explicit(&atom_0_r2_0, v41, memory_order_seq_cst);
  int v44 = (v4_r2 == 2);
  atomic_store_explicit(&atom_0_r2_2, v44, memory_order_seq_cst);
  int v45 = (v2_r1 == 1);
  atomic_store_explicit(&atom_0_r1_1, v45, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v6_r2 = atomic_load_explicit(&vars[1], memory_order_seq_cst);

  int v8_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v42 = (v8_r1 == 0);
  atomic_store_explicit(&atom_1_r1_0, v42, memory_order_seq_cst);
  int v43 = (v6_r2 == 2);
  atomic_store_explicit(&atom_1_r2_2, v43, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);

  atomic_store_explicit(&vars[1], 2, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  atomic_init(&vars[1], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&atom_0_r1_0, 0);
  atomic_init(&atom_0_r2_0, 0);
  atomic_init(&atom_1_r1_0, 0);
  atomic_init(&atom_1_r2_2, 0);
  atomic_init(&atom_0_r2_2, 0);
  atomic_init(&atom_0_r1_1, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v9 = atomic_load_explicit(&atom_0_r1_0, memory_order_seq_cst);
  int v10 = atomic_load_explicit(&atom_0_r2_0, memory_order_seq_cst);
  int v11 = atomic_load_explicit(&atom_1_r1_0, memory_order_seq_cst);
  int v12 = atomic_load_explicit(&atom_1_r2_2, memory_order_seq_cst);
  int v13_conj = v11 & v12;
  int v14_conj = v10 & v13_conj;
  int v15_conj = v9 & v14_conj;
  int v16 = atomic_load_explicit(&atom_0_r1_0, memory_order_seq_cst);
  int v17 = atomic_load_explicit(&atom_0_r2_2, memory_order_seq_cst);
  int v18 = atomic_load_explicit(&atom_1_r1_0, memory_order_seq_cst);
  int v19 = atomic_load_explicit(&atom_1_r2_2, memory_order_seq_cst);
  int v20_conj = v18 & v19;
  int v21_conj = v17 & v20_conj;
  int v22_conj = v16 & v21_conj;
  int v23 = atomic_load_explicit(&atom_0_r1_1, memory_order_seq_cst);
  int v24 = atomic_load_explicit(&atom_0_r2_0, memory_order_seq_cst);
  int v25 = atomic_load_explicit(&atom_1_r1_0, memory_order_seq_cst);
  int v26 = atomic_load_explicit(&atom_1_r2_2, memory_order_seq_cst);
  int v27_conj = v25 & v26;
  int v28_conj = v24 & v27_conj;
  int v29_conj = v23 & v28_conj;
  int v30 = atomic_load_explicit(&atom_0_r1_1, memory_order_seq_cst);
  int v31 = atomic_load_explicit(&atom_0_r2_2, memory_order_seq_cst);
  int v32 = atomic_load_explicit(&atom_1_r1_0, memory_order_seq_cst);
  int v33 = atomic_load_explicit(&atom_1_r2_2, memory_order_seq_cst);
  int v34_conj = v32 & v33;
  int v35_conj = v31 & v34_conj;
  int v36_conj = v30 & v35_conj;
  int v37_disj = v29_conj | v36_conj;
  int v38_disj = v22_conj | v37_disj;
  int v39_disj = v15_conj | v38_disj;
  if (v39_disj == 1) assert(0);
  return 0;
}
