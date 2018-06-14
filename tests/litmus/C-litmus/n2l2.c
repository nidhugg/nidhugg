// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/n2l2.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r1_1; 
volatile int atom_0_r2_0; 
volatile int atom_0_r1_2; 
volatile int atom_1_r1_2; 
volatile int atom_1_r2_0; 
volatile int atom_1_r1_1; 

void *t0(void *arg){
label_1:;
  int v2_r1 = vars[0];
  int v3_r9 = v2_r1 ^ v2_r1;
  int v6_r2 = vars[1+v3_r9];
  int v28 = (v2_r1 == 1);
  atom_0_r1_1 = v28;
  int v29 = (v6_r2 == 0);
  atom_0_r2_0 = v29;
  int v30 = (v2_r1 == 2);
  atom_0_r1_2 = v30;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v8_r1 = vars[0];
  int v9_r9 = v8_r1 ^ v8_r1;
  int v12_r2 = vars[1+v9_r9];
  int v31 = (v8_r1 == 2);
  atom_1_r1_2 = v31;
  int v32 = (v12_r2 == 0);
  atom_1_r2_0 = v32;
  int v33 = (v8_r1 == 1);
  atom_1_r1_1 = v33;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[1] = 2;

  vars[0] = 2;
  return NULL;
}

void *t3(void *arg){
label_4:;
  vars[1] = 1;

  vars[0] = 1;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  vars[1] = 0;
  vars[0] = 0;
  atom_0_r1_1 = 0;
  atom_0_r2_0 = 0;
  atom_0_r1_2 = 0;
  atom_1_r1_2 = 0;
  atom_1_r2_0 = 0;
  atom_1_r1_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v13 = atom_0_r1_1;
  int v14 = atom_0_r2_0;
  int v15_conj = v13 & v14;
  int v16 = atom_0_r1_2;
  int v17 = atom_0_r2_0;
  int v18_conj = v16 & v17;
  int v19 = atom_1_r1_2;
  int v20 = atom_1_r2_0;
  int v21_conj = v19 & v20;
  int v22 = atom_1_r1_1;
  int v23 = atom_1_r2_0;
  int v24_conj = v22 & v23;
  int v25_disj = v21_conj | v24_conj;
  int v26_disj = v18_conj | v25_disj;
  int v27_disj = v15_conj | v26_disj;
  if (v27_disj == 1) assert(0);
  return 0;
}