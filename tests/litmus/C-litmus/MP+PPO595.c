// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MP+PPO595.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[5]; 
volatile int atom_1_r1_1; 
volatile int atom_1_r14_1; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[1];
  int v3_r3 = v2_r1 ^ v2_r1;
  int v6_r4 = vars[2+v3_r3];
  int v7_r6 = v6_r4 ^ v6_r4;
  int v8_r6 = v7_r6 + 1;
  vars[3] = v8_r6;
  int v10_r8 = vars[3];
  int v11_r9 = v10_r8 ^ v10_r8;
  int v14_r10 = vars[4+v11_r9];
  int v15_r12 = v14_r10 ^ v14_r10;
  int v16_r12 = v15_r12 + 1;
  vars[0] = v16_r12;
  int v18_r14 = vars[0];
  int v25 = (v2_r1 == 1);
  atom_1_r1_1 = v25;
  int v26 = (v18_r14 == 1);
  atom_1_r14_1 = v26;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[3] = 0;
  vars[4] = 0;
  vars[0] = 0;
  vars[2] = 0;
  vars[1] = 0;
  atom_1_r1_1 = 0;
  atom_1_r14_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v19 = vars[0];
  int v20 = (v19 == 2);
  int v21 = atom_1_r1_1;
  int v22 = atom_1_r14_1;
  int v23_conj = v21 & v22;
  int v24_conj = v20 & v23_conj;
  if (v24_conj == 1) assert(0);
  return 0;
}
