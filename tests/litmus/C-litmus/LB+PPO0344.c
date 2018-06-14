// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/LB+PPO0344.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[4]; 
volatile int atom_0_r1_1; 
volatile int atom_1_r1_1; 

void *t0(void *arg){
label_1:;
  int v2_r1 = vars[0];
  int v3_r3 = v2_r1 ^ v2_r1;
  int v4_r3 = v3_r3 + 1;
  vars[1] = v4_r3;
  int v21 = (v2_r1 == 1);
  atom_0_r1_1 = v21;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v6_r1 = vars[1];
  int v7_cmpeq = (v6_r1 == v6_r1);
  if (v7_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;
  vars[2] = 1;
  vars[2] = 2;
  int v9_r6 = vars[2];
  int v10_r7 = v9_r6 ^ v9_r6;
  int v11_r7 = v10_r7 + 1;
  vars[3] = v11_r7;
  vars[3] = 2;
  vars[0] = 1;
  int v22 = (v6_r1 == 1);
  atom_1_r1_1 = v22;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[3] = 0;
  vars[1] = 0;
  vars[2] = 0;
  vars[0] = 0;
  atom_0_r1_1 = 0;
  atom_1_r1_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v12 = vars[2];
  int v13 = (v12 == 2);
  int v14 = vars[3];
  int v15 = (v14 == 2);
  int v16 = atom_0_r1_1;
  int v17 = atom_1_r1_1;
  int v18_conj = v16 & v17;
  int v19_conj = v15 & v18_conj;
  int v20_conj = v13 & v19_conj;
  if (v20_conj == 1) assert(0);
  return 0;
}