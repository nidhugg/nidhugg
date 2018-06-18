// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MP+PPO432.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[4]; 
volatile int atom_1_r1_1; 
volatile int atom_1_r12_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[1];
  int v4_r3 = vars[1];
  int v5_r4 = v4_r3 ^ v4_r3;
  vars[2+v5_r4] = 1;
  int v7_r7 = vars[2];
  int v8_r8 = v7_r7 ^ v7_r7;
  int v11_r9 = vars[3+v8_r8];
  int v13_r11 = vars[3];
  int v14_cmpeq = (v13_r11 == v13_r11);
  if (v14_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;

  int v16_r12 = vars[0];
  int v20 = (v2_r1 == 1);
  atom_1_r1_1 = v20;
  int v21 = (v16_r12 == 0);
  atom_1_r12_0 = v21;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[3] = 0;
  vars[2] = 0;
  vars[1] = 0;
  vars[0] = 0;
  atom_1_r1_1 = 0;
  atom_1_r12_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v17 = atom_1_r1_1;
  int v18 = atom_1_r12_0;
  int v19_conj = v17 & v18;
  if (v19_conj == 1) assert(0);
  return 0;
}
