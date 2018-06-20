/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[2]; 
atomic_int atom_0_r1_1; 
atomic_int atom_1_r1_1; 
atomic_int atom_1_r4_3; 
atomic_int atom_2_r1_1; 

void *t0(void *arg){
label_1:;
  int v2_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v3_r3 = v2_r1 ^ v2_r1;
  int v4_r3 = v3_r3 + 1;
  atomic_store_explicit(&vars[1], v4_r3, memory_order_seq_cst);
  int v22 = (v2_r1 == 1);
  atomic_store_explicit(&atom_0_r1_1, v22, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v6_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  atomic_store_explicit(&vars[1], 3, memory_order_seq_cst);
  int v8_r4 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v9_cmpeq = (v8_r4 == v8_r4);
  if (v9_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  int v23 = (v6_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v23, memory_order_seq_cst);
  int v24 = (v8_r4 == 3);
  atomic_store_explicit(&atom_1_r4_3, v24, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v11_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  atomic_store_explicit(&vars[1], 2, memory_order_seq_cst);
  int v25 = (v11_r1 == 1);
  atomic_store_explicit(&atom_2_r1_1, v25, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  atomic_init(&vars[1], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&atom_0_r1_1, 0);
  atomic_init(&atom_1_r1_1, 0);
  atomic_init(&atom_1_r4_3, 0);
  atomic_init(&atom_2_r1_1, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v12 = atomic_load_explicit(&atom_0_r1_1, memory_order_seq_cst);
  int v13 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v14 = atomic_load_explicit(&atom_1_r4_3, memory_order_seq_cst);
  int v15 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v16 = (v15 == 3);
  int v17 = atomic_load_explicit(&atom_2_r1_1, memory_order_seq_cst);
  int v18_conj = v16 & v17;
  int v19_conj = v14 & v18_conj;
  int v20_conj = v13 & v19_conj;
  int v21_conj = v12 & v20_conj;
  if (v21_conj == 1) assert(0);
  return 0;
}
