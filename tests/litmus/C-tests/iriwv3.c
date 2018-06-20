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
  int v3_r3 = v2_r1 ^ v2_r1;
  int v6_r2 = atomic_load_explicit(&vars[1+v3_r3], memory_order_seq_cst);
  int v44 = (v2_r1 == 0);
  atomic_store_explicit(&atom_0_r1_0, v44, memory_order_seq_cst);
  int v45 = (v6_r2 == 0);
  atomic_store_explicit(&atom_0_r2_0, v45, memory_order_seq_cst);
  int v48 = (v6_r2 == 2);
  atomic_store_explicit(&atom_0_r2_2, v48, memory_order_seq_cst);
  int v49 = (v2_r1 == 1);
  atomic_store_explicit(&atom_0_r1_1, v49, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v8_r2 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v9_r3 = v8_r2 ^ v8_r2;
  int v12_r1 = atomic_load_explicit(&vars[0+v9_r3], memory_order_seq_cst);
  int v46 = (v12_r1 == 0);
  atomic_store_explicit(&atom_1_r1_0, v46, memory_order_seq_cst);
  int v47 = (v8_r2 == 2);
  atomic_store_explicit(&atom_1_r2_2, v47, memory_order_seq_cst);
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

  atomic_init(&vars[0], 0);
  atomic_init(&vars[1], 0);
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

  int v13 = atomic_load_explicit(&atom_0_r1_0, memory_order_seq_cst);
  int v14 = atomic_load_explicit(&atom_0_r2_0, memory_order_seq_cst);
  int v15 = atomic_load_explicit(&atom_1_r1_0, memory_order_seq_cst);
  int v16 = atomic_load_explicit(&atom_1_r2_2, memory_order_seq_cst);
  int v17_conj = v15 & v16;
  int v18_conj = v14 & v17_conj;
  int v19_conj = v13 & v18_conj;
  int v20 = atomic_load_explicit(&atom_0_r1_0, memory_order_seq_cst);
  int v21 = atomic_load_explicit(&atom_0_r2_2, memory_order_seq_cst);
  int v22 = atomic_load_explicit(&atom_1_r1_0, memory_order_seq_cst);
  int v23 = atomic_load_explicit(&atom_1_r2_2, memory_order_seq_cst);
  int v24_conj = v22 & v23;
  int v25_conj = v21 & v24_conj;
  int v26_conj = v20 & v25_conj;
  int v27 = atomic_load_explicit(&atom_0_r1_1, memory_order_seq_cst);
  int v28 = atomic_load_explicit(&atom_0_r2_0, memory_order_seq_cst);
  int v29 = atomic_load_explicit(&atom_1_r1_0, memory_order_seq_cst);
  int v30 = atomic_load_explicit(&atom_1_r2_2, memory_order_seq_cst);
  int v31_conj = v29 & v30;
  int v32_conj = v28 & v31_conj;
  int v33_conj = v27 & v32_conj;
  int v34 = atomic_load_explicit(&atom_0_r1_1, memory_order_seq_cst);
  int v35 = atomic_load_explicit(&atom_0_r2_2, memory_order_seq_cst);
  int v36 = atomic_load_explicit(&atom_1_r1_0, memory_order_seq_cst);
  int v37 = atomic_load_explicit(&atom_1_r2_2, memory_order_seq_cst);
  int v38_conj = v36 & v37;
  int v39_conj = v35 & v38_conj;
  int v40_conj = v34 & v39_conj;
  int v41_disj = v33_conj | v40_conj;
  int v42_disj = v26_conj | v41_disj;
  int v43_disj = v19_conj | v42_disj;
  if (v43_disj == 1) assert(0);
  return 0;
}
