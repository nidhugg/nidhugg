// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MP+PPO164.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[4]; 
volatile int atom_1_r1_1; 
volatile int atom_1_r13_0; 

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
  int v8_r5 = vars[2+v5_r4];
  int v10_r7 = vars[2];
  int v11_r8 = v10_r7 ^ v10_r7;
  int v14_r9 = vars[3+v11_r8];
  int v16_r11 = vars[3];
  int v17_r12 = v16_r11 ^ v16_r11;
  int v20_r13 = vars[0+v17_r12];
  int v24 = (v2_r1 == 1);
  atom_1_r1_1 = v24;
  int v25 = (v20_r13 == 0);
  atom_1_r13_0 = v25;
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
  atom_1_r13_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v21 = atom_1_r1_1;
  int v22 = atom_1_r13_0;
  int v23_conj = v21 & v22;
  if (v23_conj == 1) assert(0);
  return 0;
}
