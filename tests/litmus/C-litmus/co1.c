// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/co1.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[1]; 
volatile int atom_2_r1_1; 
volatile int atom_2_r2_2; 
volatile int atom_2_r1_2; 
volatile int atom_2_r2_1; 
volatile int atom_2_r2_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[0] = 2;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v2_r1 = vars[0];
  int v4_r2 = vars[0];
  int v24 = (v2_r1 == 1);
  atom_2_r1_1 = v24;
  int v25 = (v4_r2 == 2);
  atom_2_r2_2 = v25;
  int v26 = (v2_r1 == 2);
  atom_2_r1_2 = v26;
  int v27 = (v4_r2 == 1);
  atom_2_r2_1 = v27;
  int v28 = (v4_r2 == 0);
  atom_2_r2_0 = v28;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[0] = 0;
  atom_2_r1_1 = 0;
  atom_2_r2_2 = 0;
  atom_2_r1_2 = 0;
  atom_2_r2_1 = 0;
  atom_2_r2_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v5 = atom_2_r1_1;
  int v6 = atom_2_r2_2;
  int v7 = vars[0];
  int v8 = (v7 == 1);
  int v9_conj = v6 & v8;
  int v10_conj = v5 & v9_conj;
  int v11 = atom_2_r1_2;
  int v12 = atom_2_r2_1;
  int v13 = vars[0];
  int v14 = (v13 == 2);
  int v15_conj = v12 & v14;
  int v16_conj = v11 & v15_conj;
  int v17 = atom_2_r1_1;
  int v18 = atom_2_r1_2;
  int v19_disj = v17 | v18;
  int v20 = atom_2_r2_0;
  int v21_conj = v19_disj & v20;
  int v22_disj = v16_conj | v21_conj;
  int v23_disj = v10_conj | v22_disj;
  if (v23_disj == 1) assert(0);
  return 0;
}
