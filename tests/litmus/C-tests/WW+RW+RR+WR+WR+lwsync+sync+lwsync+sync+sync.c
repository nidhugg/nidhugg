/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[5]; 
atomic_int atom_1_r1_1; 
atomic_int atom_2_r1_1; 
atomic_int atom_2_r3_0; 
atomic_int atom_3_r3_0; 
atomic_int atom_4_r3_0; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);

  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);

  atomic_store_explicit(&vars[2], 1, memory_order_seq_cst);
  int v20 = (v2_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v20, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v4_r1 = atomic_load_explicit(&vars[2], memory_order_seq_cst);

  int v6_r3 = atomic_load_explicit(&vars[3], memory_order_seq_cst);
  int v21 = (v4_r1 == 1);
  atomic_store_explicit(&atom_2_r1_1, v21, memory_order_seq_cst);
  int v22 = (v6_r3 == 0);
  atomic_store_explicit(&atom_2_r3_0, v22, memory_order_seq_cst);
  return NULL;
}

void *t3(void *arg){
label_4:;
  atomic_store_explicit(&vars[3], 1, memory_order_seq_cst);

  int v8_r3 = atomic_load_explicit(&vars[4], memory_order_seq_cst);
  int v23 = (v8_r3 == 0);
  atomic_store_explicit(&atom_3_r3_0, v23, memory_order_seq_cst);
  return NULL;
}

void *t4(void *arg){
label_5:;
  atomic_store_explicit(&vars[4], 1, memory_order_seq_cst);

  int v10_r3 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v24 = (v10_r3 == 0);
  atomic_store_explicit(&atom_4_r3_0, v24, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 
  pthread_t thr4; 

  atomic_init(&vars[1], 0);
  atomic_init(&vars[2], 0);
  atomic_init(&vars[3], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&vars[4], 0);
  atomic_init(&atom_1_r1_1, 0);
  atomic_init(&atom_2_r1_1, 0);
  atomic_init(&atom_2_r3_0, 0);
  atomic_init(&atom_3_r3_0, 0);
  atomic_init(&atom_4_r3_0, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);
  pthread_create(&thr4, NULL, t4, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);
  pthread_join(thr4, NULL);

  int v11 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v12 = atomic_load_explicit(&atom_2_r1_1, memory_order_seq_cst);
  int v13 = atomic_load_explicit(&atom_2_r3_0, memory_order_seq_cst);
  int v14 = atomic_load_explicit(&atom_3_r3_0, memory_order_seq_cst);
  int v15 = atomic_load_explicit(&atom_4_r3_0, memory_order_seq_cst);
  int v16_conj = v14 & v15;
  int v17_conj = v13 & v16_conj;
  int v18_conj = v12 & v17_conj;
  int v19_conj = v11 & v18_conj;
  if (v19_conj == 1) assert(0);
  return 0;
}
