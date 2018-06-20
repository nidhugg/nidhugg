/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[4]; 
atomic_int atom_0_r3_1; 
atomic_int atom_1_r1_1; 
atomic_int atom_2_r1_1; 
atomic_int atom_2_r4_0; 
atomic_int atom_2_r7_1; 
atomic_int atom_2_r9_0; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  int v2_r3 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v3_r4 = v2_r3 ^ v2_r3;
  int v4_r4 = v3_r4 + 1;
  atomic_store_explicit(&vars[1], v4_r4, memory_order_seq_cst);
  int v31 = (v2_r3 == 1);
  atomic_store_explicit(&atom_0_r3_1, v31, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v6_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v7_r3 = v6_r1 ^ v6_r1;
  atomic_store_explicit(&vars[2+v7_r3], 1, memory_order_seq_cst);
  int v32 = (v6_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v32, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v9_r1 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v10_r3 = v9_r1 ^ v9_r1;
  int v13_r4 = atomic_load_explicit(&vars[3+v10_r3], memory_order_seq_cst);
  atomic_store_explicit(&vars[3], 1, memory_order_seq_cst);
  int v15_r7 = atomic_load_explicit(&vars[3], memory_order_seq_cst);
  int v16_r8 = v15_r7 ^ v15_r7;
  int v19_r9 = atomic_load_explicit(&vars[0+v16_r8], memory_order_seq_cst);
  int v33 = (v9_r1 == 1);
  atomic_store_explicit(&atom_2_r1_1, v33, memory_order_seq_cst);
  int v34 = (v13_r4 == 0);
  atomic_store_explicit(&atom_2_r4_0, v34, memory_order_seq_cst);
  int v35 = (v15_r7 == 1);
  atomic_store_explicit(&atom_2_r7_1, v35, memory_order_seq_cst);
  int v36 = (v19_r9 == 0);
  atomic_store_explicit(&atom_2_r9_0, v36, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  atomic_init(&vars[2], 0);
  atomic_init(&vars[3], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&atom_0_r3_1, 0);
  atomic_init(&atom_1_r1_1, 0);
  atomic_init(&atom_2_r1_1, 0);
  atomic_init(&atom_2_r4_0, 0);
  atomic_init(&atom_2_r7_1, 0);
  atomic_init(&atom_2_r9_0, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v20 = atomic_load_explicit(&atom_0_r3_1, memory_order_seq_cst);
  int v21 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v22 = atomic_load_explicit(&atom_2_r1_1, memory_order_seq_cst);
  int v23 = atomic_load_explicit(&atom_2_r4_0, memory_order_seq_cst);
  int v24 = atomic_load_explicit(&atom_2_r7_1, memory_order_seq_cst);
  int v25 = atomic_load_explicit(&atom_2_r9_0, memory_order_seq_cst);
  int v26_conj = v24 & v25;
  int v27_conj = v23 & v26_conj;
  int v28_conj = v22 & v27_conj;
  int v29_conj = v21 & v28_conj;
  int v30_conj = v20 & v29_conj;
  if (v30_conj == 1) assert(0);
  return 0;
}
