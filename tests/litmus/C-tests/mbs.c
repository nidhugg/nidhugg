/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[2]; 
atomic_int atom_1_r2_1; 
atomic_int atom_1_r2_2; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 3, memory_order_seq_cst);

  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r2 = atomic_load_explicit(&vars[1], memory_order_seq_cst);

  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  int v26 = (v2_r2 == 1);
  atomic_store_explicit(&atom_1_r2_1, v26, memory_order_seq_cst);
  int v27 = (v2_r2 == 2);
  atomic_store_explicit(&atom_1_r2_2, v27, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);
  return NULL;
}

void *t3(void *arg){
label_4:;
  atomic_store_explicit(&vars[1], 2, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  atomic_init(&vars[1], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&atom_1_r2_1, 0);
  atomic_init(&atom_1_r2_2, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v3 = atomic_load_explicit(&atom_1_r2_1, memory_order_seq_cst);
  int v4 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v5 = (v4 == 3);
  int v6 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v7 = (v6 == 1);
  int v8_conj = v5 & v7;
  int v9_conj = v3 & v8_conj;
  int v10 = atomic_load_explicit(&atom_1_r2_1, memory_order_seq_cst);
  int v11 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v12 = (v11 == 3);
  int v13 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v14 = (v13 == 2);
  int v15_conj = v12 & v14;
  int v16_conj = v10 & v15_conj;
  int v17 = atomic_load_explicit(&atom_1_r2_2, memory_order_seq_cst);
  int v18 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v19 = (v18 == 3);
  int v20 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v21 = (v20 == 2);
  int v22_conj = v19 & v21;
  int v23_conj = v17 & v22_conj;
  int v24_disj = v16_conj | v23_conj;
  int v25_disj = v9_conj | v24_disj;
  if (v25_disj == 1) assert(0);
  return 0;
}
