/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[2]; 
atomic_int atom_1_r1_1; 
atomic_int atom_1_r2_0; 
atomic_int atom_2_r1_1; 
atomic_int atom_2_r2_0; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);

  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v3_r7 = v2_r1 ^ v2_r1;
  int v6_r2 = atomic_load_explicit(&vars[0+v3_r7], memory_order_seq_cst);
  int v20 = (v2_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v20, memory_order_seq_cst);
  int v21 = (v6_r2 == 0);
  atomic_store_explicit(&atom_1_r2_0, v21, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v8_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v9_r7 = v8_r1 ^ v8_r1;
  int v12_r2 = atomic_load_explicit(&vars[0+v9_r7], memory_order_seq_cst);
  int v22 = (v8_r1 == 1);
  atomic_store_explicit(&atom_2_r1_1, v22, memory_order_seq_cst);
  int v23 = (v12_r2 == 0);
  atomic_store_explicit(&atom_2_r2_0, v23, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  atomic_init(&vars[1], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&atom_1_r1_1, 0);
  atomic_init(&atom_1_r2_0, 0);
  atomic_init(&atom_2_r1_1, 0);
  atomic_init(&atom_2_r2_0, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v13 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v14 = atomic_load_explicit(&atom_1_r2_0, memory_order_seq_cst);
  int v15_conj = v13 & v14;
  int v16 = atomic_load_explicit(&atom_2_r1_1, memory_order_seq_cst);
  int v17 = atomic_load_explicit(&atom_2_r2_0, memory_order_seq_cst);
  int v18_conj = v16 & v17;
  int v19_disj = v15_conj | v18_conj;
  if (v19_disj == 1) assert(0);
  return 0;
}
