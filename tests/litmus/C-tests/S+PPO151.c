// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/S+PPO151.litmus

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
  int v3_r3 = v2_r1 ^ v2_r1;
  int v6_r4 = vars[2+v3_r3];
  vars[2] = 1;
  int v8_r7 = vars[2];
  int v9_cmpeq = (v8_r7 == v8_r7);
  if (v9_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;
  vars[3] = 1;
  int v11_r10 = vars[3];
  int v12_r11 = v11_r10 ^ v11_r10;
  vars[0+v12_r11] = 1;
  int v17 = (v2_r1 == 1);
  atom_1_r1_1 = v17;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[1] = 0;
  vars[3] = 0;
  vars[2] = 0;
  vars[0] = 0;
  atom_1_r1_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v13 = vars[0];
  int v14 = (v13 == 2);
  int v15 = atom_1_r1_1;
  int v16_conj = v14 & v15;
  if (v16_conj == 1) assert(0);
  return 0;
}
