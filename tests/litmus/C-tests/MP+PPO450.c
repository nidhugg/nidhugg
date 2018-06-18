// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MP+PPO450.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[4]; 
volatile int atom_1_r1_1; 
volatile int atom_1_r11_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[1];
  int v3_cmpeq = (v2_r1 == v2_r1);
  if (v3_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;
  vars[2] = 1;
  int v5_r5 = vars[2];
  int v7_r6 = vars[2];
  int v8_r7 = v7_r6 ^ v7_r6;
  int v11_r8 = vars[3+v8_r7];
  int v13_r10 = vars[3];
  int v14_cmpeq = (v13_r10 == v13_r10);
  if (v14_cmpeq)  goto lbl_LC01; else goto lbl_LC01;
lbl_LC01:;

  int v16_r11 = vars[0];
  int v20 = (v2_r1 == 1);
  atom_1_r1_1 = v20;
  int v21 = (v16_r11 == 0);
  atom_1_r11_0 = v21;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[1] = 0;
  vars[0] = 0;
  vars[2] = 0;
  vars[3] = 0;
  atom_1_r1_1 = 0;
  atom_1_r11_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v17 = atom_1_r1_1;
  int v18 = atom_1_r11_0;
  int v19_conj = v17 & v18;
  if (v19_conj == 1) assert(0);
  return 0;
}
