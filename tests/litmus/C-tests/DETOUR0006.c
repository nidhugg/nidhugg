/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[3]; 
atomic_int atom_1_r1_2; 
atomic_int atom_1_r4_0; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  int v2_r3 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v3_r5 = v2_r3 ^ v2_r3;
  int v4_r5 = v3_r5 + 1;
  atomic_store_explicit(&vars[2], v4_r5, memory_order_seq_cst);
  atomic_store_explicit(&vars[2], 2, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v6_r1 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v7_r3 = v6_r1 ^ v6_r1;
  int v10_r4 = atomic_load_explicit(&vars[0+v7_r3], memory_order_seq_cst);
  int v14 = (v6_r1 == 2);
  atomic_store_explicit(&atom_1_r1_2, v14, memory_order_seq_cst);
  int v15 = (v10_r4 == 0);
  atomic_store_explicit(&atom_1_r4_0, v15, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  atomic_init(&vars[0], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&vars[2], 0);
  atomic_init(&atom_1_r1_2, 0);
  atomic_init(&atom_1_r4_0, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v11 = atomic_load_explicit(&atom_1_r1_2, memory_order_seq_cst);
  int v12 = atomic_load_explicit(&atom_1_r4_0, memory_order_seq_cst);
  int v13_conj = v11 & v12;
  if (v13_conj == 1) assert(0);
  return 0;
}
