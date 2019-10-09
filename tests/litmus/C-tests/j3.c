/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[2]; 
atomic_int atom_0_r2_2; 
atomic_int atom_0_r4_2; 
atomic_int atom_1_r1_1; 
atomic_int atom_2_r1_3; 
atomic_int atom_2_r3_0; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  int v2_r2 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v3_r3 = v2_r2 ^ v2_r2;
  int v6_r4 = atomic_load_explicit(&vars[1+v3_r3], memory_order_seq_cst);
  atomic_store_explicit(&vars[1], 3, memory_order_seq_cst);
  int v30 = (v2_r2 == 2);
  atomic_store_explicit(&atom_0_r2_2, v30, memory_order_seq_cst);
  int v31 = (v6_r4 == 2);
  atomic_store_explicit(&atom_0_r4_2, v31, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v8_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  atomic_store_explicit(&vars[1], 2, memory_order_seq_cst);

  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);
  int v32 = (v8_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v32, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v10_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v11_r2 = v10_r1 ^ v10_r1;
  int v14_r3 = atomic_load_explicit(&vars[0+v11_r2], memory_order_seq_cst);
  int v33 = (v10_r1 == 3);
  atomic_store_explicit(&atom_2_r1_3, v33, memory_order_seq_cst);
  int v34 = (v14_r3 == 0);
  atomic_store_explicit(&atom_2_r3_0, v34, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  atomic_init(&vars[1], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&atom_0_r2_2, 0);
  atomic_init(&atom_0_r4_2, 0);
  atomic_init(&atom_1_r1_1, 0);
  atomic_init(&atom_2_r1_3, 0);
  atomic_init(&atom_2_r3_0, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v15 = atomic_load_explicit(&atom_0_r2_2, memory_order_seq_cst);
  int v16 = atomic_load_explicit(&atom_0_r4_2, memory_order_seq_cst);
  int v17 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v18 = atomic_load_explicit(&atom_2_r1_3, memory_order_seq_cst);
  int v19 = atomic_load_explicit(&atom_2_r3_0, memory_order_seq_cst);
  int v20 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v21 = (v20 == 2);
  int v22 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v23 = (v22 == 3);
  int v24_conj = v21 & v23;
  int v25_conj = v19 & v24_conj;
  int v26_conj = v18 & v25_conj;
  int v27_conj = v17 & v26_conj;
  int v28_conj = v16 & v27_conj;
  int v29_conj = v15 & v28_conj;
  if (v29_conj == 1) assert(0);
  return 0;
}
