// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/IRIW+addrs-twice.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[6]; 
volatile int atom_1_r1_1; 
volatile int atom_1_r4_0; 
volatile int atom_3_r1_1; 
volatile int atom_3_r4_0; 
volatile int atom_2_r8_1; 
volatile int atom_2_r10_0; 
volatile int atom_3_r8_1; 
volatile int atom_3_r10_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  vars[4] = 1;
  vars[2] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[0];
  int v3_r3 = v2_r1 ^ v2_r1;
  int v6_r4 = vars[1+v3_r3];
  vars[3] = 1;
  int v40 = (v2_r1 == 1);
  atom_1_r1_1 = v40;
  int v41 = (v6_r4 == 0);
  atom_1_r4_0 = v41;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[1] = 1;
  vars[5] = 1;
  int v8_r8 = vars[2];
  int v9_r9 = v8_r8 ^ v8_r8;
  int v12_r10 = vars[3+v9_r9];
  int v44 = (v8_r8 == 1);
  atom_2_r8_1 = v44;
  int v45 = (v12_r10 == 0);
  atom_2_r10_0 = v45;
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v14_r1 = vars[1];
  int v15_r3 = v14_r1 ^ v14_r1;
  int v18_r4 = vars[0+v15_r3];
  int v20_r8 = vars[3];
  int v21_r9 = v20_r8 ^ v20_r8;
  int v24_r10 = vars[2+v21_r9];
  int v42 = (v14_r1 == 1);
  atom_3_r1_1 = v42;
  int v43 = (v18_r4 == 0);
  atom_3_r4_0 = v43;
  int v46 = (v20_r8 == 1);
  atom_3_r8_1 = v46;
  int v47 = (v24_r10 == 0);
  atom_3_r10_0 = v47;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  vars[5] = 0;
  vars[2] = 0;
  vars[3] = 0;
  vars[0] = 0;
  vars[1] = 0;
  vars[4] = 0;
  atom_1_r1_1 = 0;
  atom_1_r4_0 = 0;
  atom_3_r1_1 = 0;
  atom_3_r4_0 = 0;
  atom_2_r8_1 = 0;
  atom_2_r10_0 = 0;
  atom_3_r8_1 = 0;
  atom_3_r10_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v25 = atom_1_r1_1;
  int v26 = atom_1_r4_0;
  int v27 = atom_3_r1_1;
  int v28 = atom_3_r4_0;
  int v29 = atom_2_r8_1;
  int v30 = atom_2_r10_0;
  int v31 = atom_3_r8_1;
  int v32 = atom_3_r10_0;
  int v33_conj = v31 & v32;
  int v34_conj = v30 & v33_conj;
  int v35_conj = v29 & v34_conj;
  int v36_conj = v28 & v35_conj;
  int v37_conj = v27 & v36_conj;
  int v38_conj = v26 & v37_conj;
  int v39_conj = v25 & v38_conj;
  if (v39_conj == 1) assert(0);
  return 0;
}
