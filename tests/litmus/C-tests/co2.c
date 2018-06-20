/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[1]; 
atomic_int atom_2_r1_1; 
atomic_int atom_2_r2_2; 
atomic_int atom_3_r1_2; 
atomic_int atom_3_r2_1; 
atomic_int atom_2_r1_2; 
atomic_int atom_2_r2_1; 
atomic_int atom_3_r1_1; 
atomic_int atom_3_r2_2; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v2_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v4_r2 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v24 = (v2_r1 == 1);
  atomic_store_explicit(&atom_2_r1_1, v24, memory_order_seq_cst);
  int v25 = (v4_r2 == 2);
  atomic_store_explicit(&atom_2_r2_2, v25, memory_order_seq_cst);
  int v28 = (v2_r1 == 2);
  atomic_store_explicit(&atom_2_r1_2, v28, memory_order_seq_cst);
  int v29 = (v4_r2 == 1);
  atomic_store_explicit(&atom_2_r2_1, v29, memory_order_seq_cst);
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v6_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v8_r2 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v26 = (v6_r1 == 2);
  atomic_store_explicit(&atom_3_r1_2, v26, memory_order_seq_cst);
  int v27 = (v8_r2 == 1);
  atomic_store_explicit(&atom_3_r2_1, v27, memory_order_seq_cst);
  int v30 = (v6_r1 == 1);
  atomic_store_explicit(&atom_3_r1_1, v30, memory_order_seq_cst);
  int v31 = (v8_r2 == 2);
  atomic_store_explicit(&atom_3_r2_2, v31, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  atomic_init(&vars[0], 0);
  atomic_init(&atom_2_r1_1, 0);
  atomic_init(&atom_2_r2_2, 0);
  atomic_init(&atom_3_r1_2, 0);
  atomic_init(&atom_3_r2_1, 0);
  atomic_init(&atom_2_r1_2, 0);
  atomic_init(&atom_2_r2_1, 0);
  atomic_init(&atom_3_r1_1, 0);
  atomic_init(&atom_3_r2_2, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v9 = atomic_load_explicit(&atom_2_r1_1, memory_order_seq_cst);
  int v10 = atomic_load_explicit(&atom_2_r2_2, memory_order_seq_cst);
  int v11 = atomic_load_explicit(&atom_3_r1_2, memory_order_seq_cst);
  int v12 = atomic_load_explicit(&atom_3_r2_1, memory_order_seq_cst);
  int v13_conj = v11 & v12;
  int v14_conj = v10 & v13_conj;
  int v15_conj = v9 & v14_conj;
  int v16 = atomic_load_explicit(&atom_2_r1_2, memory_order_seq_cst);
  int v17 = atomic_load_explicit(&atom_2_r2_1, memory_order_seq_cst);
  int v18 = atomic_load_explicit(&atom_3_r1_1, memory_order_seq_cst);
  int v19 = atomic_load_explicit(&atom_3_r2_2, memory_order_seq_cst);
  int v20_conj = v18 & v19;
  int v21_conj = v17 & v20_conj;
  int v22_conj = v16 & v21_conj;
  int v23_disj = v15_conj | v22_conj;
  if (v23_disj == 1) assert(0);
  return 0;
}
