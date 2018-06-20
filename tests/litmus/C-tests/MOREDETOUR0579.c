/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[2]; 
atomic_int atom_0_r5_2; 
atomic_int atom_1_r3_3; 
atomic_int atom_1_r5_0; 
atomic_int atom_3_r1_3; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);

  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);

  int v2_r5 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v19 = (v2_r5 == 2);
  atomic_store_explicit(&atom_0_r5_2, v19, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  atomic_store_explicit(&vars[1], 2, memory_order_seq_cst);
  int v4_r3 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  atomic_store_explicit(&vars[1], 5, memory_order_seq_cst);
  int v6_r5 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v20 = (v4_r3 == 3);
  atomic_store_explicit(&atom_1_r3_3, v20, memory_order_seq_cst);
  int v21 = (v6_r5 == 0);
  atomic_store_explicit(&atom_1_r5_0, v21, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  atomic_store_explicit(&vars[1], 3, memory_order_seq_cst);
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v8_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  atomic_store_explicit(&vars[1], 4, memory_order_seq_cst);
  int v22 = (v8_r1 == 3);
  atomic_store_explicit(&atom_3_r1_3, v22, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  atomic_init(&vars[1], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&atom_0_r5_2, 0);
  atomic_init(&atom_1_r3_3, 0);
  atomic_init(&atom_1_r5_0, 0);
  atomic_init(&atom_3_r1_3, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v9 = atomic_load_explicit(&atom_0_r5_2, memory_order_seq_cst);
  int v10 = atomic_load_explicit(&atom_1_r3_3, memory_order_seq_cst);
  int v11 = atomic_load_explicit(&atom_1_r5_0, memory_order_seq_cst);
  int v12 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v13 = (v12 == 5);
  int v14 = atomic_load_explicit(&atom_3_r1_3, memory_order_seq_cst);
  int v15_conj = v13 & v14;
  int v16_conj = v11 & v15_conj;
  int v17_conj = v10 & v16_conj;
  int v18_conj = v9 & v17_conj;
  if (v18_conj == 1) assert(0);
  return 0;
}
