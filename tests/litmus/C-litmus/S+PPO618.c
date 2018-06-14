// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/S+PPO618.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[4]; 
volatile int atom_1_r1_1; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[1];
  vars[1] = 2;
  int v4_r4 = vars[1];
  int v5_r5 = v4_r4 ^ v4_r4;
  int v8_r6 = vars[2+v5_r5];
  int v10_r8 = vars[2];
  int v11_r9 = v10_r8 ^ v10_r8;
  int v14_r10 = vars[3+v11_r9];
  int v15_cmpeq = (v14_r10 == v14_r10);
  if (v15_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;
  vars[0] = 1;
  int v23 = (v2_r1 == 1);
  atom_1_r1_1 = v23;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[3] = 0;
  vars[2] = 0;
  vars[0] = 0;
  vars[1] = 0;
  atom_1_r1_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v16 = vars[0];
  int v17 = (v16 == 2);
  int v18 = vars[1];
  int v19 = (v18 == 2);
  int v20 = atom_1_r1_1;
  int v21_conj = v19 & v20;
  int v22_conj = v17 & v21_conj;
  if (v22_conj == 1) assert(0);
  return 0;
}
