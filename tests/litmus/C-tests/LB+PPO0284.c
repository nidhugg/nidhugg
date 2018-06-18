// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/LB+PPO0284.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[5]; 
volatile int atom_0_r1_1; 
volatile int atom_1_r1_1; 

void *t0(void *arg){
label_1:;
  int v2_r1 = vars[0];
  int v3_r3 = v2_r1 ^ v2_r1;
  int v4_r3 = v3_r3 + 1;
  vars[1] = v4_r3;
  int v23 = (v2_r1 == 1);
  atom_0_r1_1 = v23;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v6_r1 = vars[1];
  int v7_r3 = v6_r1 ^ v6_r1;
  int v10_r4 = vars[2+v7_r3];
  int v12_r6 = vars[2];
  int v13_r7 = v12_r6 ^ v12_r6;
  int v16_r8 = vars[3+v13_r7];
  int v18_r10 = vars[3];
  int v19_cmpeq = (v18_r10 == v18_r10);
  if (v19_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;
  vars[4] = 1;
  vars[0] = 1;
  int v24 = (v6_r1 == 1);
  atom_1_r1_1 = v24;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[2] = 0;
  vars[1] = 0;
  vars[4] = 0;
  vars[3] = 0;
  vars[0] = 0;
  atom_0_r1_1 = 0;
  atom_1_r1_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v20 = atom_0_r1_1;
  int v21 = atom_1_r1_1;
  int v22_conj = v20 & v21;
  if (v22_conj == 1) assert(0);
  return 0;
}
