/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[3]; 
atomic_int atom_0_r1_1; 
atomic_int atom_1_r1_1; 
atomic_int atom_2_r1_1; 

void *t0(void *arg){
label_1:;
  int v2_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v3_r3 = v2_r1 ^ v2_r1;
  atomic_store_explicit(&vars[1+v3_r3], 1, memory_order_seq_cst);
  int v13 = (v2_r1 == 1);
  atomic_store_explicit(&atom_0_r1_1, v13, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v5_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);

  atomic_store_explicit(&vars[2], 1, memory_order_seq_cst);
  int v14 = (v5_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v14, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v7_r1 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  int v15 = (v7_r1 == 1);
  atomic_store_explicit(&atom_2_r1_1, v15, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  atomic_init(&vars[2], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&atom_0_r1_1, 0);
  atomic_init(&atom_1_r1_1, 0);
  atomic_init(&atom_2_r1_1, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v8 = atomic_load_explicit(&atom_0_r1_1, memory_order_seq_cst);
  int v9 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v10 = atomic_load_explicit(&atom_2_r1_1, memory_order_seq_cst);
  int v11_conj = v9 & v10;
  int v12_conj = v8 & v11_conj;
  if (v12_conj == 1) assert(0);
  return 0;
}
