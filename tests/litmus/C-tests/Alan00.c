/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[5]; 
atomic_int atom_2_r6_2; 
atomic_int atom_2_r7_0; 
atomic_int atom_2_r8_1; 
atomic_int atom_3_r6_1; 
atomic_int atom_3_r7_0; 
atomic_int atom_3_r8_1; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);

  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);

  atomic_store_explicit(&vars[2], 1, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  atomic_store_explicit(&vars[3], 2, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v2_r6 = atomic_load_explicit(&vars[3], memory_order_seq_cst);
  int v3_r7 = v2_r6 ^ v2_r6;
  int v6_r7 = atomic_load_explicit(&vars[0+v3_r7], memory_order_seq_cst);
  int v8_r8 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v9_r9 = v8_r8 ^ v8_r8;
  atomic_store_explicit(&vars[4+v9_r9], 1, memory_order_seq_cst);
  int v33 = (v2_r6 == 2);
  atomic_store_explicit(&atom_2_r6_2, v33, memory_order_seq_cst);
  int v34 = (v6_r7 == 0);
  atomic_store_explicit(&atom_2_r7_0, v34, memory_order_seq_cst);
  int v35 = (v8_r8 == 1);
  atomic_store_explicit(&atom_2_r8_1, v35, memory_order_seq_cst);
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v11_r6 = atomic_load_explicit(&vars[4], memory_order_seq_cst);
  int v12_r7 = v11_r6 ^ v11_r6;
  int v15_r7 = atomic_load_explicit(&vars[1+v12_r7], memory_order_seq_cst);
  int v17_r8 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v18_r9 = v17_r8 ^ v17_r8;
  atomic_store_explicit(&vars[3+v18_r9], 1, memory_order_seq_cst);
  int v36 = (v11_r6 == 1);
  atomic_store_explicit(&atom_3_r6_1, v36, memory_order_seq_cst);
  int v37 = (v15_r7 == 0);
  atomic_store_explicit(&atom_3_r7_0, v37, memory_order_seq_cst);
  int v38 = (v17_r8 == 1);
  atomic_store_explicit(&atom_3_r8_1, v38, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  atomic_init(&vars[2], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&vars[3], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&vars[4], 0);
  atomic_init(&atom_2_r6_2, 0);
  atomic_init(&atom_2_r7_0, 0);
  atomic_init(&atom_2_r8_1, 0);
  atomic_init(&atom_3_r6_1, 0);
  atomic_init(&atom_3_r7_0, 0);
  atomic_init(&atom_3_r8_1, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v19 = atomic_load_explicit(&vars[3], memory_order_seq_cst);
  int v20 = (v19 == 2);
  int v21 = atomic_load_explicit(&atom_2_r6_2, memory_order_seq_cst);
  int v22 = atomic_load_explicit(&atom_2_r7_0, memory_order_seq_cst);
  int v23 = atomic_load_explicit(&atom_2_r8_1, memory_order_seq_cst);
  int v24 = atomic_load_explicit(&atom_3_r6_1, memory_order_seq_cst);
  int v25 = atomic_load_explicit(&atom_3_r7_0, memory_order_seq_cst);
  int v26 = atomic_load_explicit(&atom_3_r8_1, memory_order_seq_cst);
  int v27_conj = v25 & v26;
  int v28_conj = v24 & v27_conj;
  int v29_conj = v23 & v28_conj;
  int v30_conj = v22 & v29_conj;
  int v31_conj = v21 & v30_conj;
  int v32_conj = v20 & v31_conj;
  if (v32_conj == 1) assert(0);
  return 0;
}
