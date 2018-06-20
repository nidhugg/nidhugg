/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[4]; 
atomic_int atom_1_r1_1; 
atomic_int atom_1_r12_0; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);

  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v4_r3 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v5_r4 = v4_r3 ^ v4_r3;
  atomic_store_explicit(&vars[2+v5_r4], 1, memory_order_seq_cst);
  int v7_r7 = atomic_load_explicit(&vars[2], memory_order_seq_cst);
  int v8_r8 = v7_r7 ^ v7_r7;
  int v11_r9 = atomic_load_explicit(&vars[3+v8_r8], memory_order_seq_cst);
  int v13_r11 = atomic_load_explicit(&vars[3], memory_order_seq_cst);
  int v14_cmpeq = (v13_r11 == v13_r11);
  if (v14_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;

  int v16_r12 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v20 = (v2_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v20, memory_order_seq_cst);
  int v21 = (v16_r12 == 0);
  atomic_store_explicit(&atom_1_r12_0, v21, memory_order_seq_cst);
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  atomic_init(&vars[1], 0);
  atomic_init(&vars[3], 0);
  atomic_init(&vars[2], 0);
  atomic_init(&vars[0], 0);
  atomic_init(&atom_1_r1_1, 0);
  atomic_init(&atom_1_r12_0, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v17 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v18 = atomic_load_explicit(&atom_1_r12_0, memory_order_seq_cst);
  int v19_conj = v17 & v18;
  if (v19_conj == 1) assert(0);
  return 0;
}
