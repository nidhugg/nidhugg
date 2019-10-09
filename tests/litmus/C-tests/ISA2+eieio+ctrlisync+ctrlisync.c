/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[3]; 
atomic_int atom_1_r1_1; 
atomic_int atom_2_r1_1; 
atomic_int atom_2_r3_0; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);

  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v3_cmpeq = (v2_r1 == v2_r1);
  if (v3_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;

  atomic_store_explicit(&vars[2], 1, memory_order_seq_cst);
  int v14 = (v2_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v14, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v5_r1 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v6_cmpeq = (v5_r1 == v5_r1);
  if (v6_cmpeq)  goto lbl_LC01; else goto lbl_LC01;
lbl_LC01:;

  int v8_r3 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v15 = (v5_r1 == 1);
  atomic_store_explicit(&atom_2_r1_1, v15, memory_order_seq_cst);
  int v16 = (v8_r3 == 0);
  atomic_store_explicit(&atom_2_r3_0, v16, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  atomic_init(&vars[2], 0);
  atomic_init(&vars[1], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&atom_1_r1_1, 0);
  atomic_init(&atom_2_r1_1, 0);
  atomic_init(&atom_2_r3_0, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v9 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v10 = atomic_load_explicit(&atom_2_r1_1, memory_order_seq_cst);
  int v11 = atomic_load_explicit(&atom_2_r3_0, memory_order_seq_cst);
  int v12_conj = v10 & v11;
  int v13_conj = v9 & v12_conj;
  if (v13_conj == 1) assert(0);
  return 0;
}
