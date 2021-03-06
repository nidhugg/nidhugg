/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[2]; 
atomic_int atom_0_r3_0; 
atomic_int atom_1_r5_4; 
atomic_int atom_1_r7_0; 
atomic_int atom_2_r4_3; 
atomic_int atom_2_r1_1; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);

  int v2_r3 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v22 = (v2_r3 == 0);
  atomic_store_explicit(&atom_0_r3_0, v22, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  atomic_store_explicit(&vars[1], 3, memory_order_seq_cst);
  atomic_store_explicit(&vars[1], 4, memory_order_seq_cst);
  int v4_r5 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v5_r6 = v4_r5 ^ v4_r5;
  int v8_r7 = atomic_load_explicit(&vars[0+v5_r6], memory_order_seq_cst);
  int v23 = (v4_r5 == 4);
  atomic_store_explicit(&atom_1_r5_4, v23, memory_order_seq_cst);
  int v24 = (v8_r7 == 0);
  atomic_store_explicit(&atom_1_r7_0, v24, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v10_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  atomic_store_explicit(&vars[1], 2, memory_order_seq_cst);

  int v12_r4 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v25 = (v12_r4 == 3);
  atomic_store_explicit(&atom_2_r4_3, v25, memory_order_seq_cst);
  int v26 = (v10_r1 == 1);
  atomic_store_explicit(&atom_2_r1_1, v26, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  atomic_init(&vars[0], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&atom_0_r3_0, 0);
  atomic_init(&atom_1_r5_4, 0);
  atomic_init(&atom_1_r7_0, 0);
  atomic_init(&atom_2_r4_3, 0);
  atomic_init(&atom_2_r1_1, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v13 = atomic_load_explicit(&atom_0_r3_0, memory_order_seq_cst);
  int v14 = atomic_load_explicit(&atom_1_r5_4, memory_order_seq_cst);
  int v15 = atomic_load_explicit(&atom_1_r7_0, memory_order_seq_cst);
  int v16 = atomic_load_explicit(&atom_2_r4_3, memory_order_seq_cst);
  int v17 = atomic_load_explicit(&atom_2_r1_1, memory_order_seq_cst);
  int v18_conj = v16 & v17;
  int v19_conj = v15 & v18_conj;
  int v20_conj = v14 & v19_conj;
  int v21_conj = v13 & v20_conj;
  if (v21_conj == 1) assert(0);
  return 0;
}
