/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[2]; 
atomic_int atom_1_r1_1; 
atomic_int atom_1_r3_0; 
atomic_int atom_3_r1_1; 
atomic_int atom_3_r3_0; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);

  int v4_r3 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v17 = (v2_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v17, memory_order_seq_cst);
  int v18 = (v4_r3 == 0);
  atomic_store_explicit(&atom_1_r3_0, v18, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v6_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v7_cmpeq = (v6_r1 == v6_r1);
  if (v7_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;

  int v9_r3 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v19 = (v6_r1 == 1);
  atomic_store_explicit(&atom_3_r1_1, v19, memory_order_seq_cst);
  int v20 = (v9_r3 == 0);
  atomic_store_explicit(&atom_3_r3_0, v20, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  atomic_init(&vars[0], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&atom_1_r1_1, 0);
  atomic_init(&atom_1_r3_0, 0);
  atomic_init(&atom_3_r1_1, 0);
  atomic_init(&atom_3_r3_0, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v10 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v11 = atomic_load_explicit(&atom_1_r3_0, memory_order_seq_cst);
  int v12 = atomic_load_explicit(&atom_3_r1_1, memory_order_seq_cst);
  int v13 = atomic_load_explicit(&atom_3_r3_0, memory_order_seq_cst);
  int v14_conj = v12 & v13;
  int v15_conj = v11 & v14_conj;
  int v16_conj = v10 & v15_conj;
  if (v16_conj == 1) assert(0);
  return 0;
}
