// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/W+RWC+rfi-data+addr-fri-rfi-addr+rfi-addr.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[4]; 
volatile int atom_0_r3_1; 
volatile int atom_1_r1_1; 
volatile int atom_1_r4_0; 
volatile int atom_1_r7_1; 
volatile int atom_1_r9_0; 
volatile int atom_2_r3_1; 
volatile int atom_2_r5_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  int v2_r3 = vars[0];
  int v3_r4 = v2_r3 ^ v2_r3;
  int v4_r4 = v3_r4 + 1;
  vars[1] = v4_r4;
  int v36 = (v2_r3 == 1);
  atom_0_r3_1 = v36;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v6_r1 = vars[1];
  int v7_r3 = v6_r1 ^ v6_r1;
  int v10_r4 = vars[2+v7_r3];
  vars[2] = 1;
  int v12_r7 = vars[2];
  int v13_r8 = v12_r7 ^ v12_r7;
  int v16_r9 = vars[3+v13_r8];
  int v37 = (v6_r1 == 1);
  atom_1_r1_1 = v37;
  int v38 = (v10_r4 == 0);
  atom_1_r4_0 = v38;
  int v39 = (v12_r7 == 1);
  atom_1_r7_1 = v39;
  int v40 = (v16_r9 == 0);
  atom_1_r9_0 = v40;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[3] = 1;
  int v18_r3 = vars[3];
  int v19_r4 = v18_r3 ^ v18_r3;
  int v22_r5 = vars[0+v19_r4];
  int v41 = (v18_r3 == 1);
  atom_2_r3_1 = v41;
  int v42 = (v22_r5 == 0);
  atom_2_r5_0 = v42;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[1] = 0;
  vars[0] = 0;
  vars[3] = 0;
  vars[2] = 0;
  atom_0_r3_1 = 0;
  atom_1_r1_1 = 0;
  atom_1_r4_0 = 0;
  atom_1_r7_1 = 0;
  atom_1_r9_0 = 0;
  atom_2_r3_1 = 0;
  atom_2_r5_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v23 = atom_0_r3_1;
  int v24 = atom_1_r1_1;
  int v25 = atom_1_r4_0;
  int v26 = atom_1_r7_1;
  int v27 = atom_1_r9_0;
  int v28 = atom_2_r3_1;
  int v29 = atom_2_r5_0;
  int v30_conj = v28 & v29;
  int v31_conj = v27 & v30_conj;
  int v32_conj = v26 & v31_conj;
  int v33_conj = v25 & v32_conj;
  int v34_conj = v24 & v33_conj;
  int v35_conj = v23 & v34_conj;
  if (v35_conj == 1) assert(0);
  return 0;
}
