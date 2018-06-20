/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[2]; 
atomic_int atom_0_r3_2; 
atomic_int atom_0_r5_0; 
atomic_int atom_1_r3_1; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);
  int v2_r3 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v3_r4 = v2_r3 ^ v2_r3;
  int v6_r5 = atomic_load_explicit(&vars[1+v3_r4], memory_order_seq_cst);
  int v18 = (v2_r3 == 2);
  atomic_store_explicit(&atom_0_r3_2, v18, memory_order_seq_cst);
  int v19 = (v6_r5 == 0);
  atomic_store_explicit(&atom_0_r5_0, v19, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  int v8_r3 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v9_r4 = v8_r3 ^ v8_r3;
  atomic_store_explicit(&vars[0+v9_r4], 1, memory_order_seq_cst);
  int v20 = (v8_r3 == 1);
  atomic_store_explicit(&atom_1_r3_1, v20, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  atomic_init(&vars[1], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&atom_0_r3_2, 0);
  atomic_init(&atom_0_r5_0, 0);
  atomic_init(&atom_1_r3_1, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v10 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v11 = (v10 == 2);
  int v12 = atomic_load_explicit(&atom_0_r3_2, memory_order_seq_cst);
  int v13 = atomic_load_explicit(&atom_0_r5_0, memory_order_seq_cst);
  int v14 = atomic_load_explicit(&atom_1_r3_1, memory_order_seq_cst);
  int v15_conj = v13 & v14;
  int v16_conj = v12 & v15_conj;
  int v17_conj = v11 & v16_conj;
  if (v17_conj == 1) assert(0);
  return 0;
}
