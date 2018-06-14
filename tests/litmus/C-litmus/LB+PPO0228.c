// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/LB+PPO0228.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[6]; 
volatile int atom_0_r1_1; 
volatile int atom_1_r1_1; 

void *t0(void *arg){
label_1:;
  int v2_r1 = vars[0];
  int v3_r3 = v2_r1 ^ v2_r1;
  int v4_r3 = v3_r3 + 1;
  vars[1] = v4_r3;
  int v22 = (v2_r1 == 1);
  atom_0_r1_1 = v22;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v6_r1 = vars[1];
  int v7_r3 = v6_r1 ^ v6_r1;
  int v10_r4 = vars[2+v7_r3];
  int v11_cmpeq = (v10_r4 == v10_r4);
  if (v11_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;
  vars[3] = 1;
  int v13_r8 = vars[3];
  int v14_r9 = v13_r8 ^ v13_r8;
  int v17_r10 = vars[4+v14_r9];
  int v18_cmpeq = (v17_r10 == v17_r10);
  if (v18_cmpeq)  goto lbl_LC01; else goto lbl_LC01;
lbl_LC01:;
  vars[5] = 1;
  vars[0] = 1;
  int v23 = (v6_r1 == 1);
  atom_1_r1_1 = v23;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[3] = 0;
  vars[4] = 0;
  vars[0] = 0;
  vars[2] = 0;
  vars[5] = 0;
  vars[1] = 0;
  atom_0_r1_1 = 0;
  atom_1_r1_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v19 = atom_0_r1_1;
  int v20 = atom_1_r1_1;
  int v21_conj = v19 & v20;
  if (v21_conj == 1) assert(0);
  return 0;
}
