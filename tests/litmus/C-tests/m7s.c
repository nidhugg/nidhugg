/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[2]; 
atomic_int atom_1_r1_1; 
atomic_int atom_1_r2_0; 
atomic_int atom_2_r3_0; 
atomic_int atom_1_r2_2; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);

  int v4_r2 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v24 = (v2_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v24, memory_order_seq_cst);
  int v25 = (v4_r2 == 0);
  atomic_store_explicit(&atom_1_r2_0, v25, memory_order_seq_cst);
  int v27 = (v4_r2 == 2);
  atomic_store_explicit(&atom_1_r2_2, v27, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);

  int v6_r3 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v26 = (v6_r3 == 0);
  atomic_store_explicit(&atom_2_r3_0, v26, memory_order_seq_cst);
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
  atomic_init(&atom_1_r1_1, 0);
  atomic_init(&atom_1_r2_0, 0);
  atomic_init(&atom_2_r3_0, 0);
  atomic_init(&atom_1_r2_2, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v7 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v8 = atomic_load_explicit(&atom_1_r2_0, memory_order_seq_cst);
  int v9 = atomic_load_explicit(&atom_2_r3_0, memory_order_seq_cst);
  int v10_conj = v8 & v9;
  int v11_conj = v7 & v10_conj;
  int v12 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v13 = atomic_load_explicit(&atom_1_r2_2, memory_order_seq_cst);
  int v14 = atomic_load_explicit(&atom_2_r3_0, memory_order_seq_cst);
  int v15 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v16 = (v15 == 1);
  int v17 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v18 = (v17 == 1);
  int v19_conj = v16 & v18;
  int v20_conj = v14 & v19_conj;
  int v21_conj = v13 & v20_conj;
  int v22_conj = v12 & v21_conj;
  int v23_disj = v11_conj | v22_conj;
  if (v23_disj == 1) assert(0);
  return 0;
}
