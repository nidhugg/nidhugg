/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[2]; 
atomic_int atom_0_r3_1; 
atomic_int atom_0_r5_0; 
atomic_int atom_1_r3_1; 
atomic_int atom_2_r3_2; 
atomic_int atom_2_r5_0; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);

  int v2_r3 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v3_r4 = v2_r3 ^ v2_r3;
  int v6_r5 = atomic_load_explicit(&vars[1+v3_r4], memory_order_seq_cst);
  int v27 = (v2_r3 == 1);
  atomic_store_explicit(&atom_0_r3_1, v27, memory_order_seq_cst);
  int v28 = (v6_r5 == 0);
  atomic_store_explicit(&atom_0_r5_0, v28, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);

  int v8_r3 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v29 = (v8_r3 == 1);
  atomic_store_explicit(&atom_1_r3_1, v29, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  atomic_store_explicit(&vars[1], 2, memory_order_seq_cst);

  int v10_r3 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v11_r4 = v10_r3 ^ v10_r3;
  int v14_r5 = atomic_load_explicit(&vars[0+v11_r4], memory_order_seq_cst);
  int v30 = (v10_r3 == 2);
  atomic_store_explicit(&atom_2_r3_2, v30, memory_order_seq_cst);
  int v31 = (v14_r5 == 0);
  atomic_store_explicit(&atom_2_r5_0, v31, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  atomic_init(&vars[1], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&atom_0_r3_1, 0);
  atomic_init(&atom_0_r5_0, 0);
  atomic_init(&atom_1_r3_1, 0);
  atomic_init(&atom_2_r3_2, 0);
  atomic_init(&atom_2_r5_0, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v15 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v16 = (v15 == 2);
  int v17 = atomic_load_explicit(&atom_0_r3_1, memory_order_seq_cst);
  int v18 = atomic_load_explicit(&atom_0_r5_0, memory_order_seq_cst);
  int v19 = atomic_load_explicit(&atom_1_r3_1, memory_order_seq_cst);
  int v20 = atomic_load_explicit(&atom_2_r3_2, memory_order_seq_cst);
  int v21 = atomic_load_explicit(&atom_2_r5_0, memory_order_seq_cst);
  int v22_conj = v20 & v21;
  int v23_conj = v19 & v22_conj;
  int v24_conj = v18 & v23_conj;
  int v25_conj = v17 & v24_conj;
  int v26_conj = v16 & v25_conj;
  if (v26_conj == 1) assert(0);
  return 0;
}
