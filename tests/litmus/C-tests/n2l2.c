/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[2]; 
atomic_int atom_0_r1_1; 
atomic_int atom_0_r2_0; 
atomic_int atom_0_r1_2; 
atomic_int atom_1_r1_2; 
atomic_int atom_1_r2_0; 
atomic_int atom_1_r1_1; 

void *t0(void *arg){
label_1:;
  int v2_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v3_r9 = v2_r1 ^ v2_r1;
  int v6_r2 = atomic_load_explicit(&vars[1+v3_r9], memory_order_seq_cst);
  int v28 = (v2_r1 == 1);
  atomic_store_explicit(&atom_0_r1_1, v28, memory_order_seq_cst);
  int v29 = (v6_r2 == 0);
  atomic_store_explicit(&atom_0_r2_0, v29, memory_order_seq_cst);
  int v30 = (v2_r1 == 2);
  atomic_store_explicit(&atom_0_r1_2, v30, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v8_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v9_r9 = v8_r1 ^ v8_r1;
  int v12_r2 = atomic_load_explicit(&vars[1+v9_r9], memory_order_seq_cst);
  int v31 = (v8_r1 == 2);
  atomic_store_explicit(&atom_1_r1_2, v31, memory_order_seq_cst);
  int v32 = (v12_r2 == 0);
  atomic_store_explicit(&atom_1_r2_0, v32, memory_order_seq_cst);
  int v33 = (v8_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v33, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  atomic_store_explicit(&vars[1], 2, memory_order_seq_cst);

  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);
  return NULL;
}

void *t3(void *arg){
label_4:;
  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);

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
  atomic_init(&atom_0_r2_0, 0);
  atomic_init(&atom_0_r1_2, 0);
  atomic_init(&atom_1_r1_2, 0);
  atomic_init(&atom_1_r2_0, 0);
  atomic_init(&atom_1_r1_1, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v13 = atomic_load_explicit(&atom_0_r1_1, memory_order_seq_cst);
  int v14 = atomic_load_explicit(&atom_0_r2_0, memory_order_seq_cst);
  int v15_conj = v13 & v14;
  int v16 = atomic_load_explicit(&atom_0_r1_2, memory_order_seq_cst);
  int v17 = atomic_load_explicit(&atom_0_r2_0, memory_order_seq_cst);
  int v18_conj = v16 & v17;
  int v19 = atomic_load_explicit(&atom_1_r1_2, memory_order_seq_cst);
  int v20 = atomic_load_explicit(&atom_1_r2_0, memory_order_seq_cst);
  int v21_conj = v19 & v20;
  int v22 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v23 = atomic_load_explicit(&atom_1_r2_0, memory_order_seq_cst);
  int v24_conj = v22 & v23;
  int v25_disj = v21_conj | v24_conj;
  int v26_disj = v18_conj | v25_disj;
  int v27_disj = v15_conj | v26_disj;
  if (v27_disj == 1) assert(0);
  return 0;
}
