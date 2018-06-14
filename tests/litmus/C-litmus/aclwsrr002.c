// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/aclwsrr002.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r1_1; 
volatile int atom_0_r3_1; 
volatile int atom_0_r5_0; 
volatile int atom_2_r1_1; 
volatile int atom_2_r3_1; 
volatile int atom_2_r5_0; 

void *t0(void *arg){
label_1:;
  int v2_r1 = vars[0];

  int v4_r3 = vars[0];
  int v5_r4 = v4_r3 ^ v4_r3;
  int v8_r5 = vars[1+v5_r4];
  int v28 = (v2_r1 == 1);
  atom_0_r1_1 = v28;
  int v29 = (v4_r3 == 1);
  atom_0_r3_1 = v29;
  int v30 = (v8_r5 == 0);
  atom_0_r5_0 = v30;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 1;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v10_r1 = vars[1];

  int v12_r3 = vars[1];
  int v13_r4 = v12_r3 ^ v12_r3;
  int v16_r5 = vars[0+v13_r4];
  int v31 = (v10_r1 == 1);
  atom_2_r1_1 = v31;
  int v32 = (v12_r3 == 1);
  atom_2_r3_1 = v32;
  int v33 = (v16_r5 == 0);
  atom_2_r5_0 = v33;
  return NULL;
}

void *t3(void *arg){
label_4:;
  vars[0] = 1;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  vars[0] = 0;
  vars[1] = 0;
  atom_0_r1_1 = 0;
  atom_0_r3_1 = 0;
  atom_0_r5_0 = 0;
  atom_2_r1_1 = 0;
  atom_2_r3_1 = 0;
  atom_2_r5_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v17 = atom_0_r1_1;
  int v18 = atom_0_r3_1;
  int v19 = atom_0_r5_0;
  int v20 = atom_2_r1_1;
  int v21 = atom_2_r3_1;
  int v22 = atom_2_r5_0;
  int v23_conj = v21 & v22;
  int v24_conj = v20 & v23_conj;
  int v25_conj = v19 & v24_conj;
  int v26_conj = v18 & v25_conj;
  int v27_conj = v17 & v26_conj;
  if (v27_conj == 1) assert(0);
  return 0;
}
