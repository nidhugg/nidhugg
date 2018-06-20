/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[2]; 
atomic_int atom_1_r1_1; 
atomic_int atom_1_r4_0; 
atomic_int atom_1_r7_1; 
atomic_int atom_2_r4_3; 
atomic_int atom_2_r1_1; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 4, memory_order_seq_cst);

  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v3_r3 = v2_r1 ^ v2_r1;
  int v6_r4 = atomic_load_explicit(&vars[0+v3_r3], memory_order_seq_cst);
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  int v8_r7 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  atomic_store_explicit(&vars[0], 3, memory_order_seq_cst);
  int v25 = (v2_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v25, memory_order_seq_cst);
  int v26 = (v6_r4 == 0);
  atomic_store_explicit(&atom_1_r4_0, v26, memory_order_seq_cst);
  int v27 = (v8_r7 == 1);
  atomic_store_explicit(&atom_1_r7_1, v27, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v10_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);

  int v12_r4 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v28 = (v12_r4 == 3);
  atomic_store_explicit(&atom_2_r4_3, v28, memory_order_seq_cst);
  int v29 = (v10_r1 == 1);
  atomic_store_explicit(&atom_2_r1_1, v29, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  atomic_init(&vars[0], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&atom_1_r1_1, 0);
  atomic_init(&atom_1_r4_0, 0);
  atomic_init(&atom_1_r7_1, 0);
  atomic_init(&atom_2_r4_3, 0);
  atomic_init(&atom_2_r1_1, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v13 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v14 = (v13 == 4);
  int v15 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v16 = atomic_load_explicit(&atom_1_r4_0, memory_order_seq_cst);
  int v17 = atomic_load_explicit(&atom_1_r7_1, memory_order_seq_cst);
  int v18 = atomic_load_explicit(&atom_2_r4_3, memory_order_seq_cst);
  int v19 = atomic_load_explicit(&atom_2_r1_1, memory_order_seq_cst);
  int v20_conj = v18 & v19;
  int v21_conj = v17 & v20_conj;
  int v22_conj = v16 & v21_conj;
  int v23_conj = v15 & v22_conj;
  int v24_conj = v14 & v23_conj;
  if (v24_conj == 1) assert(0);
  return 0;
}
