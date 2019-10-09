/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[4]; 
atomic_int atom_0_r1_1; 
atomic_int atom_0_r2_0; 
atomic_int atom_1_r1_1; 
atomic_int atom_1_r2_0; 
atomic_int atom_2_r1_1; 
atomic_int atom_2_r2_0; 
atomic_int atom_3_r1_1; 
atomic_int atom_3_r2_0; 

void *t0(void *arg){
label_1:;
  int v2_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v3_r9 = v2_r1 ^ v2_r1;
  int v6_r2 = atomic_load_explicit(&vars[1+v3_r9], memory_order_seq_cst);
  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  int v40 = (v2_r1 == 1);
  atomic_store_explicit(&atom_0_r1_1, v40, memory_order_seq_cst);
  int v41 = (v6_r2 == 0);
  atomic_store_explicit(&atom_0_r2_0, v41, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v8_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v9_r9 = v8_r1 ^ v8_r1;
  int v12_r2 = atomic_load_explicit(&vars[2+v9_r9], memory_order_seq_cst);
  atomic_store_explicit(&vars[2], 1, memory_order_seq_cst);
  int v42 = (v8_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v42, memory_order_seq_cst);
  int v43 = (v12_r2 == 0);
  atomic_store_explicit(&atom_1_r2_0, v43, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v14_r1 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v15_r9 = v14_r1 ^ v14_r1;
  int v18_r2 = atomic_load_explicit(&vars[3+v15_r9], memory_order_seq_cst);
  atomic_store_explicit(&vars[3], 1, memory_order_seq_cst);
  int v44 = (v14_r1 == 1);
  atomic_store_explicit(&atom_2_r1_1, v44, memory_order_seq_cst);
  int v45 = (v18_r2 == 0);
  atomic_store_explicit(&atom_2_r2_0, v45, memory_order_seq_cst);
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v20_r1 = atomic_load_explicit(&vars[3], memory_order_seq_cst);
  int v21_r9 = v20_r1 ^ v20_r1;
  int v24_r2 = atomic_load_explicit(&vars[0+v21_r9], memory_order_seq_cst);
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  int v46 = (v20_r1 == 1);
  atomic_store_explicit(&atom_3_r1_1, v46, memory_order_seq_cst);
  int v47 = (v24_r2 == 0);
  atomic_store_explicit(&atom_3_r2_0, v47, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  atomic_init(&vars[3], 0);
  atomic_init(&vars[2], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&atom_0_r1_1, 0);
  atomic_init(&atom_0_r2_0, 0);
  atomic_init(&atom_1_r1_1, 0);
  atomic_init(&atom_1_r2_0, 0);
  atomic_init(&atom_2_r1_1, 0);
  atomic_init(&atom_2_r2_0, 0);
  atomic_init(&atom_3_r1_1, 0);
  atomic_init(&atom_3_r2_0, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v25 = atomic_load_explicit(&atom_0_r1_1, memory_order_seq_cst);
  int v26 = atomic_load_explicit(&atom_0_r2_0, memory_order_seq_cst);
  int v27 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v28 = atomic_load_explicit(&atom_1_r2_0, memory_order_seq_cst);
  int v29 = atomic_load_explicit(&atom_2_r1_1, memory_order_seq_cst);
  int v30 = atomic_load_explicit(&atom_2_r2_0, memory_order_seq_cst);
  int v31 = atomic_load_explicit(&atom_3_r1_1, memory_order_seq_cst);
  int v32 = atomic_load_explicit(&atom_3_r2_0, memory_order_seq_cst);
  int v33_conj = v31 & v32;
  int v34_conj = v30 & v33_conj;
  int v35_conj = v29 & v34_conj;
  int v36_conj = v28 & v35_conj;
  int v37_conj = v27 & v36_conj;
  int v38_conj = v26 & v37_conj;
  int v39_conj = v25 & v38_conj;
  if (v39_conj == 1) assert(0);
  return 0;
}
