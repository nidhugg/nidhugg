// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/LB+PPO0032.litmus

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
  int v24 = (v2_r1 == 1);
  atom_0_r1_1 = v24;
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
  vars[3] = 2;
  int v13_r9 = vars[3];
  int v14_r10 = v13_r9 ^ v13_r9;
  int v17_r11 = vars[4+v14_r10];
  vars[0] = 1;
  int v25 = (v6_r1 == 1);
  atom_1_r1_1 = v25;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[2] = 0;
  vars[0] = 0;
  vars[1] = 0;
  vars[3] = 0;
  vars[4] = 0;
  atom_0_r1_1 = 0;
  atom_1_r1_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v18 = vars[3];
  int v19 = (v18 == 2);
  int v20 = atom_0_r1_1;
  int v21 = atom_1_r1_1;
  int v22_conj = v20 & v21;
  int v23_conj = v19 & v22_conj;
  if (v23_conj == 1) assert(0);
  return 0;
}