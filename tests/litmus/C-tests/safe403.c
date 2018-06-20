/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[2]; 
atomic_int atom_0_r1_2; 
atomic_int atom_0_r3_0; 
atomic_int atom_2_r1_2; 
atomic_int atom_2_r3_0; 

void *t0(void *arg){
label_1:;
  int v2_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);

  int v4_r3 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v22 = (v2_r1 == 2);
  atomic_store_explicit(&atom_0_r1_2, v22, memory_order_seq_cst);
  int v23 = (v4_r3 == 0);
  atomic_store_explicit(&atom_0_r3_0, v23, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);

  atomic_store_explicit(&vars[1], 2, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v6_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);

  int v8_r3 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v24 = (v6_r1 == 2);
  atomic_store_explicit(&atom_2_r1_2, v24, memory_order_seq_cst);
  int v25 = (v8_r3 == 0);
  atomic_store_explicit(&atom_2_r3_0, v25, memory_order_seq_cst);
  return NULL;
}

void *t3(void *arg){
label_4:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);

  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  atomic_init(&vars[1], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&atom_0_r1_2, 0);
  atomic_init(&atom_0_r3_0, 0);
  atomic_init(&atom_2_r1_2, 0);
  atomic_init(&atom_2_r3_0, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v9 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v10 = (v9 == 2);
  int v11 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v12 = (v11 == 2);
  int v13 = atomic_load_explicit(&atom_0_r1_2, memory_order_seq_cst);
  int v14 = atomic_load_explicit(&atom_0_r3_0, memory_order_seq_cst);
  int v15 = atomic_load_explicit(&atom_2_r1_2, memory_order_seq_cst);
  int v16 = atomic_load_explicit(&atom_2_r3_0, memory_order_seq_cst);
  int v17_conj = v15 & v16;
  int v18_conj = v14 & v17_conj;
  int v19_conj = v13 & v18_conj;
  int v20_conj = v12 & v19_conj;
  int v21_conj = v10 & v20_conj;
  if (v21_conj == 1) assert(0);
  return 0;
}
