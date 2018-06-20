/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[3]; 
atomic_int atom_0_r3_1; 
atomic_int atom_0_r5_0; 
atomic_int atom_1_r3_1; 
atomic_int atom_1_r5_0; 
atomic_int atom_2_r3_1; 
atomic_int atom_2_r5_0; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);

  int v2_r3 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v3_r4 = v2_r3 ^ v2_r3;
  int v6_r5 = atomic_load_explicit(&vars[1+v3_r4], memory_order_seq_cst);
  int v30 = (v2_r3 == 1);
  atomic_store_explicit(&atom_0_r3_1, v30, memory_order_seq_cst);
  int v31 = (v6_r5 == 0);
  atomic_store_explicit(&atom_0_r5_0, v31, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);

  int v8_r3 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v9_r4 = v8_r3 ^ v8_r3;
  int v12_r5 = atomic_load_explicit(&vars[2+v9_r4], memory_order_seq_cst);
  int v32 = (v8_r3 == 1);
  atomic_store_explicit(&atom_1_r3_1, v32, memory_order_seq_cst);
  int v33 = (v12_r5 == 0);
  atomic_store_explicit(&atom_1_r5_0, v33, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  atomic_store_explicit(&vars[2], 1, memory_order_seq_cst);

  int v14_r3 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v15_r4 = v14_r3 ^ v14_r3;
  int v18_r5 = atomic_load_explicit(&vars[0+v15_r4], memory_order_seq_cst);
  int v34 = (v14_r3 == 1);
  atomic_store_explicit(&atom_2_r3_1, v34, memory_order_seq_cst);
  int v35 = (v18_r5 == 0);
  atomic_store_explicit(&atom_2_r5_0, v35, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  atomic_init(&vars[2], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&atom_0_r3_1, 0);
  atomic_init(&atom_0_r5_0, 0);
  atomic_init(&atom_1_r3_1, 0);
  atomic_init(&atom_1_r5_0, 0);
  atomic_init(&atom_2_r3_1, 0);
  atomic_init(&atom_2_r5_0, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v19 = atomic_load_explicit(&atom_0_r3_1, memory_order_seq_cst);
  int v20 = atomic_load_explicit(&atom_0_r5_0, memory_order_seq_cst);
  int v21 = atomic_load_explicit(&atom_1_r3_1, memory_order_seq_cst);
  int v22 = atomic_load_explicit(&atom_1_r5_0, memory_order_seq_cst);
  int v23 = atomic_load_explicit(&atom_2_r3_1, memory_order_seq_cst);
  int v24 = atomic_load_explicit(&atom_2_r5_0, memory_order_seq_cst);
  int v25_conj = v23 & v24;
  int v26_conj = v22 & v25_conj;
  int v27_conj = v21 & v26_conj;
  int v28_conj = v20 & v27_conj;
  int v29_conj = v19 & v28_conj;
  if (v29_conj == 1) assert(0);
  return 0;
}
