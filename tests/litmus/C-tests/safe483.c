/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[3]; 
atomic_int atom_0_r1_1; 
atomic_int atom_1_r1_1; 
atomic_int atom_1_r4_0; 
atomic_int atom_2_r1_1; 
atomic_int atom_2_r3_1; 

void *t0(void *arg){
label_1:;
  int v2_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);

  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  int v25 = (v2_r1 == 1);
  atomic_store_explicit(&atom_0_r1_1, v25, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v4_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v5_r3 = v4_r1 ^ v4_r1;
  int v8_r4 = atomic_load_explicit(&vars[2+v5_r3], memory_order_seq_cst);
  atomic_store_explicit(&vars[2], 1, memory_order_seq_cst);
  int v26 = (v4_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v26, memory_order_seq_cst);
  int v27 = (v8_r4 == 0);
  atomic_store_explicit(&atom_1_r4_0, v27, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v10_r1 = atomic_load_explicit(&vars[2], memory_order_seq_cst);

  int v12_r3 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v28 = (v10_r1 == 1);
  atomic_store_explicit(&atom_2_r1_1, v28, memory_order_seq_cst);
  int v29 = (v12_r3 == 1);
  atomic_store_explicit(&atom_2_r3_1, v29, memory_order_seq_cst);
  return NULL;
}

void *t3(void *arg){
label_4:;
  atomic_store_explicit(&vars[2], 2, memory_order_seq_cst);

  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  atomic_init(&vars[0], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&vars[2], 0);
  atomic_init(&atom_0_r1_1, 0);
  atomic_init(&atom_1_r1_1, 0);
  atomic_init(&atom_1_r4_0, 0);
  atomic_init(&atom_2_r1_1, 0);
  atomic_init(&atom_2_r3_1, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v13 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v14 = (v13 == 2);
  int v15 = atomic_load_explicit(&atom_0_r1_1, memory_order_seq_cst);
  int v16 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v17 = atomic_load_explicit(&atom_1_r4_0, memory_order_seq_cst);
  int v18 = atomic_load_explicit(&atom_2_r1_1, memory_order_seq_cst);
  int v19 = atomic_load_explicit(&atom_2_r3_1, memory_order_seq_cst);
  int v20_conj = v18 & v19;
  int v21_conj = v17 & v20_conj;
  int v22_conj = v16 & v21_conj;
  int v23_conj = v15 & v22_conj;
  int v24_conj = v14 & v23_conj;
  if (v24_conj == 1) assert(0);
  return 0;
}
