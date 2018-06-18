// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/S+PPO569.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[5]; 
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
  int v3_r3 = v2_r1 ^ v2_r1;
  int v6_r4 = vars[2+v3_r3];
  int v7_r6 = v6_r4 ^ v6_r4;
  vars[3+v7_r6] = 1;
  int v9_r9 = vars[3];
  int v10_r10 = v9_r9 ^ v9_r9;
  int v13_r11 = vars[4+v10_r10];
  int v14_cmpeq = (v13_r11 == v13_r11);
  if (v14_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;
  vars[0] = 1;
  int v19 = (v2_r1 == 1);
  atom_1_r1_1 = v19;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[3] = 0;
  vars[2] = 0;
  vars[0] = 0;
  vars[1] = 0;
  vars[4] = 0;
  atom_1_r1_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v15 = vars[0];
  int v16 = (v15 == 2);
  int v17 = atom_1_r1_1;
  int v18_conj = v16 & v17;
  if (v18_conj == 1) assert(0);
  return 0;
}
