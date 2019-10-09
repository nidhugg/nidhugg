/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

atomic_int vars[2]; 
atomic_int atom_1_r1_1; 
atomic_int atom_1_r2_0; 
atomic_int atom_1_r2_2; 

void *t0(void *arg){
label_1:;
  atomic_store_explicit(&vars[0], 1, memory_order_seq_cst);
  atomic_store_explicit(&vars[1], 1, memory_order_seq_cst);
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v4_r2 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v64 = (v2_r1 == 1);
  atomic_store_explicit(&atom_1_r1_1, v64, memory_order_seq_cst);
  int v65 = (v4_r2 == 0);
  atomic_store_explicit(&atom_1_r2_0, v65, memory_order_seq_cst);
  int v66 = (v4_r2 == 2);
  atomic_store_explicit(&atom_1_r2_2, v66, memory_order_seq_cst);
  return NULL;
}

void *t2(void *arg){
label_3:;
  atomic_store_explicit(&vars[0], 2, memory_order_seq_cst);
  return NULL;
}

void *t3(void *arg){
label_4:;
  atomic_store_explicit(&vars[1], 2, memory_order_seq_cst);
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
  atomic_init(&atom_1_r2_0, 0);
  atomic_init(&atom_1_r2_2, 0);

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v5 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v6 = atomic_load_explicit(&atom_1_r2_0, memory_order_seq_cst);
  int v7 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v8 = (v7 == 1);
  int v9 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v10 = (v9 == 1);
  int v11_conj = v8 & v10;
  int v12_conj = v6 & v11_conj;
  int v13_conj = v5 & v12_conj;
  int v14 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v15 = atomic_load_explicit(&atom_1_r2_0, memory_order_seq_cst);
  int v16 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v17 = (v16 == 1);
  int v18 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v19 = (v18 == 2);
  int v20_conj = v17 & v19;
  int v21_conj = v15 & v20_conj;
  int v22_conj = v14 & v21_conj;
  int v23 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v24 = atomic_load_explicit(&atom_1_r2_0, memory_order_seq_cst);
  int v25 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v26 = (v25 == 2);
  int v27 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v28 = (v27 == 1);
  int v29_conj = v26 & v28;
  int v30_conj = v24 & v29_conj;
  int v31_conj = v23 & v30_conj;
  int v32 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v33 = atomic_load_explicit(&atom_1_r2_0, memory_order_seq_cst);
  int v34 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v35 = (v34 == 2);
  int v36 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v37 = (v36 == 2);
  int v38_conj = v35 & v37;
  int v39_conj = v33 & v38_conj;
  int v40_conj = v32 & v39_conj;
  int v41 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v42 = atomic_load_explicit(&atom_1_r2_2, memory_order_seq_cst);
  int v43 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v44 = (v43 == 1);
  int v45 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v46 = (v45 == 1);
  int v47_conj = v44 & v46;
  int v48_conj = v42 & v47_conj;
  int v49_conj = v41 & v48_conj;
  int v50 = atomic_load_explicit(&atom_1_r1_1, memory_order_seq_cst);
  int v51 = atomic_load_explicit(&atom_1_r2_2, memory_order_seq_cst);
  int v52 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v53 = (v52 == 1);
  int v54 = atomic_load_explicit(&vars[1], memory_order_seq_cst);
  int v55 = (v54 == 2);
  int v56_conj = v53 & v55;
  int v57_conj = v51 & v56_conj;
  int v58_conj = v50 & v57_conj;
  int v59_disj = v49_conj | v58_conj;
  int v60_disj = v40_conj | v59_disj;
  int v61_disj = v31_conj | v60_disj;
  int v62_disj = v22_conj | v61_disj;
  int v63_disj = v13_conj | v62_disj;
  if (v63_disj == 1) assert(0);
  return 0;
}
