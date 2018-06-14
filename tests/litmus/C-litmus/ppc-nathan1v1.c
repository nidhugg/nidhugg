// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/ppc-nathan1v1.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_0_r8_1; 
volatile int atom_0_r9_0; 
volatile int atom_1_r8_1; 
volatile int atom_1_r9_0; 
volatile int atom_3_r8_1; 

void *t0(void *arg){
label_1:;
  int v2_r8 = vars[0];
  int v3_r12 = v2_r8 ^ v2_r8;
  int v6_r9 = vars[1+v3_r12];
  int v24 = (v2_r8 == 1);
  atom_0_r8_1 = v24;
  int v25 = (v6_r9 == 0);
  atom_0_r9_0 = v25;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 1;
  int v8_r8 = vars[1];
  int v9_r12 = v8_r8 ^ v8_r8;
  int v12_r9 = vars[2+v9_r12];
  int v26 = (v8_r8 == 1);
  atom_1_r8_1 = v26;
  int v27 = (v12_r9 == 0);
  atom_1_r9_0 = v27;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[2] = 1;
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v14_r8 = vars[2];
  vars[0] = 1;
  int v28 = (v14_r8 == 1);
  atom_3_r8_1 = v28;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  vars[1] = 0;
  vars[2] = 0;
  vars[0] = 0;
  atom_0_r8_1 = 0;
  atom_0_r9_0 = 0;
  atom_1_r8_1 = 0;
  atom_1_r9_0 = 0;
  atom_3_r8_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v15 = atom_0_r8_1;
  int v16 = atom_0_r9_0;
  int v17 = atom_1_r8_1;
  int v18 = atom_1_r9_0;
  int v19 = atom_3_r8_1;
  int v20_conj = v18 & v19;
  int v21_conj = v17 & v20_conj;
  int v22_conj = v16 & v21_conj;
  int v23_conj = v15 & v22_conj;
  if (v23_conj == 1) assert(0);
  return 0;
}
