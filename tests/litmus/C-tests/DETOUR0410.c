/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[2]; 
atomic_int atom_0_r3_3; 
atomic_int atom_0_r4_4; 
atomic_int atom_3_r5_2; 
atomic_int atom_3_r1_1; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);
  int v2_r3 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v4_r4 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v5_r5 = v4_r4 ^ v4_r4;
  atomic_store_explicit(&vars[1+v5_r5], 1, memory_order_seq_cst);
  int v19 = (v2_r3 == 3);
  atomic_store_explicit(&atom_0_r3_3, v19, memory_order_seq_cst);
  int v20 = (v4_r4 == 4);
  atomic_store_explicit(&atom_0_r4_4, v20, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  atomic_store_explicit(&vars[0], 3, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  atomic_store_explicit(&vars[0], 4, memory_order_seq_cst);
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v7_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v8_r3 = v7_r1 ^ v7_r1;
  int v9_r3 = v8_r3 + 1;
  atomic_store_explicit(&vars[0], v9_r3, memory_order_seq_cst);

  int v11_r5 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v21 = (v11_r5 == 2);
  atomic_store_explicit(&atom_3_r5_2, v21, memory_order_seq_cst);
  int v22 = (v7_r1 == 1);
  atomic_store_explicit(&atom_3_r1_1, v22, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  atomic_init(&vars[0], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&atom_0_r3_3, 0);
  atomic_init(&atom_0_r4_4, 0);
  atomic_init(&atom_3_r5_2, 0);
  atomic_init(&atom_3_r1_1, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v12 = atomic_load_explicit(&atom_0_r3_3, memory_order_seq_cst);
  int v13 = atomic_load_explicit(&atom_0_r4_4, memory_order_seq_cst);
  int v14 = atomic_load_explicit(&atom_3_r5_2, memory_order_seq_cst);
  int v15 = atomic_load_explicit(&atom_3_r1_1, memory_order_seq_cst);
  int v16_conj = v14 & v15;
  int v17_conj = v13 & v16_conj;
  int v18_conj = v12 & v17_conj;
  if (v18_conj == 1) assert(0);
  return 0;
}
