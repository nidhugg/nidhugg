/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[3]; 
atomic_int atom_1_r1_2; 
atomic_int atom_2_r5_2; 
atomic_int atom_2_r1_1; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);
  atomic_store_explicit(&vars[0], 4, memory_order_seq_cst);
  int v2_r4 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v3_r6 = v2_r4 ^ v2_r4;
  atomic_store_explicit(&vars[2+v3_r6], 1, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v5_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  atomic_store_explicit(&vars[0], 3, memory_order_seq_cst);
  int v20 = (v5_r1 == 2);
  atomic_store_explicit(&atom_1_r1_2, v20, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v7_r1 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v8_r3 = v7_r1 ^ v7_r1;
  int v9_r3 = v8_r3 + 1;
  atomic_store_explicit(&vars[0], v9_r3, memory_order_seq_cst);

  int v11_r5 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v21 = (v11_r5 == 2);
  atomic_store_explicit(&atom_2_r5_2, v21, memory_order_seq_cst);
  int v22 = (v7_r1 == 1);
  atomic_store_explicit(&atom_2_r1_1, v22, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  atomic_init(&vars[1], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&vars[2], 0);
  atomic_init(&atom_1_r1_2, 0);
  atomic_init(&atom_2_r5_2, 0);
  atomic_init(&atom_2_r1_1, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v12 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v13 = (v12 == 4);
  int v14 = atomic_load_explicit(&atom_1_r1_2, memory_order_seq_cst);
  int v15 = atomic_load_explicit(&atom_2_r5_2, memory_order_seq_cst);
  int v16 = atomic_load_explicit(&atom_2_r1_1, memory_order_seq_cst);
  int v17_conj = v15 & v16;
  int v18_conj = v14 & v17_conj;
  int v19_conj = v13 & v18_conj;
  if (v19_conj == 1) assert(0);
  return 0;
}
