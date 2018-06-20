/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[4]; 
atomic_int atom_2_r1_1; 
atomic_int atom_2_r3_0; 
atomic_int atom_2_r4_1; 
atomic_int atom_3_r1_1; 
atomic_int atom_3_r3_0; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);

  atomic_store_explicit(&vars[2], 1, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v2_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v3_r2 = v2_r1 ^ v2_r1;
  int v6_r3 = atomic_load_explicit(&vars[1+v3_r2], memory_order_seq_cst);
  int v8_r4 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v9_r5 = v8_r4 ^ v8_r4;
  int v10_r5 = v9_r5 + 1;
  atomic_store_explicit(&vars[3], v10_r5, memory_order_seq_cst);
  int v26 = (v2_r1 == 1);
  atomic_store_explicit(&atom_2_r1_1, v26, memory_order_seq_cst);
  int v27 = (v6_r3 == 0);
  atomic_store_explicit(&atom_2_r3_0, v27, memory_order_seq_cst);
  int v28 = (v8_r4 == 1);
  atomic_store_explicit(&atom_2_r4_1, v28, memory_order_seq_cst);
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v12_r1 = atomic_load_explicit(&vars[3], memory_order_seq_cst);
  int v13_r2 = v12_r1 ^ v12_r1;
  int v16_r3 = atomic_load_explicit(&vars[0+v13_r2], memory_order_seq_cst);
  int v29 = (v12_r1 == 1);
  atomic_store_explicit(&atom_3_r1_1, v29, memory_order_seq_cst);
  int v30 = (v16_r3 == 0);
  atomic_store_explicit(&atom_3_r3_0, v30, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  atomic_init(&vars[3], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&vars[2], 0);
  atomic_init(&atom_2_r1_1, 0);
  atomic_init(&atom_2_r3_0, 0);
  atomic_init(&atom_2_r4_1, 0);
  atomic_init(&atom_3_r1_1, 0);
  atomic_init(&atom_3_r3_0, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v17 = atomic_load_explicit(&atom_2_r1_1, memory_order_seq_cst);
  int v18 = atomic_load_explicit(&atom_2_r3_0, memory_order_seq_cst);
  int v19 = atomic_load_explicit(&atom_2_r4_1, memory_order_seq_cst);
  int v20 = atomic_load_explicit(&atom_3_r1_1, memory_order_seq_cst);
  int v21 = atomic_load_explicit(&atom_3_r3_0, memory_order_seq_cst);
  int v22_conj = v20 & v21;
  int v23_conj = v19 & v22_conj;
  int v24_conj = v18 & v23_conj;
  int v25_conj = v17 & v24_conj;
  if (v25_conj == 1) assert(0);
  return 0;
}
