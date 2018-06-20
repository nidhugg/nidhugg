/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[5]; 
atomic_int atom_1_r1_1; 
atomic_int atom_1_r14_0; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);

  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v3_r3 = v2_r1 ^ v2_r1;
  atomic_store_explicit(&vars[2+v3_r3], 1, memory_order_seq_cst);
  int v5_r6 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v6_r7 = v5_r6 ^ v5_r6;
  int v9_r8 = atomic_load_explicit(&vars[3+v6_r7], memory_order_seq_cst);
  int v10_r10 = v9_r8 ^ v9_r8;
  int v11_r10 = v10_r10 + 1;
  atomic_store_explicit(&vars[4], v11_r10, memory_order_seq_cst);
  int v13_r12 = atomic_load_explicit(&vars[4], memory_order_seq_cst);
  int v14_r13 = v13_r12 ^ v13_r12;
  int v17_r14 = atomic_load_explicit(&vars[0+v14_r13], memory_order_seq_cst);
  int v21 = (v2_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v21, memory_order_seq_cst);
  int v22 = (v17_r14 == 0);
  atomic_store_explicit(&atom_1_r14_0, v22, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  atomic_init(&vars[1], 0);
  atomic_init(&vars[3], 0);
  atomic_init(&vars[2], 0);
  atomic_init(&vars[4], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&atom_1_r1_1, 0);
  atomic_init(&atom_1_r14_0, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v18 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v19 = atomic_load_explicit(&atom_1_r14_0, memory_order_seq_cst);
  int v20_conj = v18 & v19;
  if (v20_conj == 1) assert(0);
  return 0;
}
