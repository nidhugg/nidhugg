// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MP+PPO301.litmus

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
  int v3_r3 = v2_r1 ^ v2_r1;
  vars[2+v3_r3] = 1;
  vars[2] = 2;
  int v5_r7 = vars[2];
  int v6_r8 = v5_r7 ^ v5_r7;
  vars[3+v6_r8] = 1;
  int v8_r11 = vars[3];
  int v9_cmpeq = (v8_r11 == v8_r11);
  if (v9_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;

  int v11_r12 = vars[0];
  int v18 = (v2_r1 == 1);
  atom_1_r1_1 = v18;
  int v19 = (v11_r12 == 0);
  atom_1_r12_0 = v19;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 0;
  vars[2] = 0;
  vars[1] = 0;
  vars[3] = 0;
  atom_1_r1_1 = 0;
  atom_1_r12_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v12 = vars[2];
  int v13 = (v12 == 2);
  int v14 = atom_1_r1_1;
  int v15 = atom_1_r12_0;
  int v16_conj = v14 & v15;
  int v17_conj = v13 & v16_conj;
  if (v17_conj == 1) assert(0);
  return 0;
}
