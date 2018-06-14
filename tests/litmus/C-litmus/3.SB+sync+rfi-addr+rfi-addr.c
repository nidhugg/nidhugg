// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/3.SB+sync+rfi-addr+rfi-addr.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_0_r3_0; 
volatile int atom_1_r3_1; 
volatile int atom_1_r5_0; 
volatile int atom_2_r3_1; 
volatile int atom_2_r5_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  int v2_r3 = vars[1];
  int v24 = (v2_r3 == 0);
  atom_0_r3_0 = v24;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 1;
  int v4_r3 = vars[1];
  int v5_r4 = v4_r3 ^ v4_r3;
  int v8_r5 = vars[2+v5_r4];
  int v25 = (v4_r3 == 1);
  atom_1_r3_1 = v25;
  int v26 = (v8_r5 == 0);
  atom_1_r5_0 = v26;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[2] = 1;
  int v10_r3 = vars[2];
  int v11_r4 = v10_r3 ^ v10_r3;
  int v14_r5 = vars[0+v11_r4];
  int v27 = (v10_r3 == 1);
  atom_2_r3_1 = v27;
  int v28 = (v14_r5 == 0);
  atom_2_r5_0 = v28;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[1] = 0;
  vars[2] = 0;
  vars[0] = 0;
  atom_0_r3_0 = 0;
  atom_1_r3_1 = 0;
  atom_1_r5_0 = 0;
  atom_2_r3_1 = 0;
  atom_2_r5_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v15 = atom_0_r3_0;
  int v16 = atom_1_r3_1;
  int v17 = atom_1_r5_0;
  int v18 = atom_2_r3_1;
  int v19 = atom_2_r5_0;
  int v20_conj = v18 & v19;
  int v21_conj = v17 & v20_conj;
  int v22_conj = v16 & v21_conj;
  int v23_conj = v15 & v22_conj;
  if (v23_conj == 1) assert(0);
  return 0;
}