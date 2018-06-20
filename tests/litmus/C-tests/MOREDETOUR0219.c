/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[2]; 
atomic_int atom_0_r1_3; 
atomic_int atom_1_r1_1; 
atomic_int atom_1_r4_0; 
atomic_int atom_1_r6_1; 
atomic_int atom_3_r1_1; 

void *t0(void *arg){
label_1:;
  int v2_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v3_r3 = v2_r1 ^ v2_r1;
  int v4_r3 = v3_r3 + 1;
  atomic_store_explicit(&vars[1], v4_r3, memory_order_seq_cst);
  int v27 = (v2_r1 == 3);
  atomic_store_explicit(&atom_0_r1_3, v27, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v6_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v7_r3 = v6_r1 ^ v6_r1;
  int v10_r4 = atomic_load_explicit(&vars[0+v7_r3], memory_order_seq_cst);
  int v12_r6 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  atomic_store_explicit(&vars[0], 3, memory_order_seq_cst);
  int v28 = (v6_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v28, memory_order_seq_cst);
  int v29 = (v10_r4 == 0);
  atomic_store_explicit(&atom_1_r4_0, v29, memory_order_seq_cst);
  int v30 = (v12_r6 == 1);
  atomic_store_explicit(&atom_1_r6_1, v30, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v14_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);
  int v31 = (v14_r1 == 1);
  atomic_store_explicit(&atom_3_r1_1, v31, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  atomic_init(&vars[0], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&atom_0_r1_3, 0);
  atomic_init(&atom_1_r1_1, 0);
  atomic_init(&atom_1_r4_0, 0);
  atomic_init(&atom_1_r6_1, 0);
  atomic_init(&atom_3_r1_1, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v15 = atomic_load_explicit(&atom_0_r1_3, memory_order_seq_cst);
  int v16 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v17 = atomic_load_explicit(&atom_1_r4_0, memory_order_seq_cst);
  int v18 = atomic_load_explicit(&atom_1_r6_1, memory_order_seq_cst);
  int v19 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v20 = (v19 == 3);
  int v21 = atomic_load_explicit(&atom_3_r1_1, memory_order_seq_cst);
  int v22_conj = v20 & v21;
  int v23_conj = v18 & v22_conj;
  int v24_conj = v17 & v23_conj;
  int v25_conj = v16 & v24_conj;
  int v26_conj = v15 & v25_conj;
  if (v26_conj == 1) assert(0);
  return 0;
}
