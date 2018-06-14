// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/ISA2+rfi-data+data+addr-fri-rfi-addr.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[4]; 
volatile int atom_0_r3_1; 
volatile int atom_1_r1_1; 
volatile int atom_2_r1_1; 
volatile int atom_2_r4_0; 
volatile int atom_2_r7_1; 
volatile int atom_2_r9_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  int v2_r3 = vars[0];
  int v3_r4 = v2_r3 ^ v2_r3;
  int v4_r4 = v3_r4 + 1;
  vars[1] = v4_r4;
  int v32 = (v2_r3 == 1);
  atom_0_r3_1 = v32;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v6_r1 = vars[1];
  int v7_r3 = v6_r1 ^ v6_r1;
  int v8_r3 = v7_r3 + 1;
  vars[2] = v8_r3;
  int v33 = (v6_r1 == 1);
  atom_1_r1_1 = v33;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v10_r1 = vars[2];
  int v11_r3 = v10_r1 ^ v10_r1;
  int v14_r4 = vars[3+v11_r3];
  vars[3] = 1;
  int v16_r7 = vars[3];
  int v17_r8 = v16_r7 ^ v16_r7;
  int v20_r9 = vars[0+v17_r8];
  int v34 = (v10_r1 == 1);
  atom_2_r1_1 = v34;
  int v35 = (v14_r4 == 0);
  atom_2_r4_0 = v35;
  int v36 = (v16_r7 == 1);
  atom_2_r7_1 = v36;
  int v37 = (v20_r9 == 0);
  atom_2_r9_0 = v37;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[2] = 0;
  vars[0] = 0;
  vars[1] = 0;
  vars[3] = 0;
  atom_0_r3_1 = 0;
  atom_1_r1_1 = 0;
  atom_2_r1_1 = 0;
  atom_2_r4_0 = 0;
  atom_2_r7_1 = 0;
  atom_2_r9_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v21 = atom_0_r3_1;
  int v22 = atom_1_r1_1;
  int v23 = atom_2_r1_1;
  int v24 = atom_2_r4_0;
  int v25 = atom_2_r7_1;
  int v26 = atom_2_r9_0;
  int v27_conj = v25 & v26;
  int v28_conj = v24 & v27_conj;
  int v29_conj = v23 & v28_conj;
  int v30_conj = v22 & v29_conj;
  int v31_conj = v21 & v30_conj;
  if (v31_conj == 1) assert(0);
  return 0;
}
