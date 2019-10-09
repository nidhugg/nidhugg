/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[2]; 
atomic_int atom_0_r1_1; 
atomic_int atom_0_r3_1; 
atomic_int atom_0_r5_0; 
atomic_int atom_2_r1_1; 
atomic_int atom_2_r3_1; 
atomic_int atom_2_r5_0; 

void *t0(void *arg){
label_1:;
  int v2_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);

  int v4_r3 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v5_r4 = v4_r3 ^ v4_r3;
  int v8_r5 = atomic_load_explicit(&vars[1+v5_r4], memory_order_seq_cst);
  int v28 = (v2_r1 == 1);
  atomic_store_explicit(&atom_0_r1_1, v28, memory_order_seq_cst);
  int v29 = (v4_r3 == 1);
  atomic_store_explicit(&atom_0_r3_1, v29, memory_order_seq_cst);
  int v30 = (v8_r5 == 0);
  atomic_store_explicit(&atom_0_r5_0, v30, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v10_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);

  int v12_r3 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v13_r4 = v12_r3 ^ v12_r3;
  int v16_r5 = atomic_load_explicit(&vars[0+v13_r4], memory_order_seq_cst);
  int v31 = (v10_r1 == 1);
  atomic_store_explicit(&atom_2_r1_1, v31, memory_order_seq_cst);
  int v32 = (v12_r3 == 1);
  atomic_store_explicit(&atom_2_r3_1, v32, memory_order_seq_cst);
  int v33 = (v16_r5 == 0);
  atomic_store_explicit(&atom_2_r5_0, v33, memory_order_seq_cst);
  return NULL;
}

void *t3(void *arg){
label_4:;
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
  atomic_init(&atom_0_r1_1, 0);
  atomic_init(&atom_0_r3_1, 0);
  atomic_init(&atom_0_r5_0, 0);
  atomic_init(&atom_2_r1_1, 0);
  atomic_init(&atom_2_r3_1, 0);
  atomic_init(&atom_2_r5_0, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v17 = atomic_load_explicit(&atom_0_r1_1, memory_order_seq_cst);
  int v18 = atomic_load_explicit(&atom_0_r3_1, memory_order_seq_cst);
  int v19 = atomic_load_explicit(&atom_0_r5_0, memory_order_seq_cst);
  int v20 = atomic_load_explicit(&atom_2_r1_1, memory_order_seq_cst);
  int v21 = atomic_load_explicit(&atom_2_r3_1, memory_order_seq_cst);
  int v22 = atomic_load_explicit(&atom_2_r5_0, memory_order_seq_cst);
  int v23_conj = v21 & v22;
  int v24_conj = v20 & v23_conj;
  int v25_conj = v19 & v24_conj;
  int v26_conj = v18 & v25_conj;
  int v27_conj = v17 & v26_conj;
  if (v27_conj == 1) assert(0);
  return 0;
}
