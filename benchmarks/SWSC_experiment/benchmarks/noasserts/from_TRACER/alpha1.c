/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */

/* There are N^2+N+1 weak traces */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#ifndef N
#  warning "N was not defined"
#  define N 5
#endif

atomic_int vars[1];
atomic_int atom_1_r1_2;
atomic_int atom_1_r2_1;


void *writer(void *arg){
  int tid = *((int *)arg);
  atomic_store_explicit(&vars[0], tid, memory_order_seq_cst);
  
  return NULL;
}

void *reader(void *arg){
  int v2_r1 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v4_r2 = atomic_load_explicit(&vars[0], memory_order_seq_cst);
  int v8 = (v2_r1 == 2);
  atomic_store_explicit(&atom_1_r1_2, v8, memory_order_seq_cst);
  int v9 = (v4_r2 == 1);
  atomic_store_explicit(&atom_1_r2_1, v9, memory_order_seq_cst);
  
  return NULL;
}

int arg[N];
int main(int argc, char **argv){
  pthread_t ws[N];
 	pthread_t read;
  
  atomic_init(&vars[0], 1);
  
  for (int i=0; i<N; i++) {
    arg[i]=i;
    pthread_create(&ws[i], NULL, writer, &arg[i]);
  }
  
  pthread_create(&read, NULL, reader, NULL);
  
  for (int i=0; i<N; i++)
    pthread_join(ws[i], NULL);
  
  pthread_join(read, NULL);
  
  int v5 = atomic_load_explicit(&atom_1_r1_2, memory_order_seq_cst);
  int v6 = atomic_load_explicit(&atom_1_r2_1, memory_order_seq_cst);
//  int v7_conj = v5 & v6;
//  if (v7_conj == 1) assert(0);
  return 0;
  
}
