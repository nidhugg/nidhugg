// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MP+PPO856.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[4]; 
volatile int atom_1_r1_1; 
volatile int atom_1_r14_0; 

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
  int v6_r4 = vars[2+v3_r3];
  int v8_r6 = vars[2];
  int v9_r7 = v8_r6 ^ v8_r6;
  int v12_r8 = vars[3+v9_r7];
  int v14_r10 = vars[3];
  int v15_r11 = v14_r10 ^ v14_r10;
  int v18_r12 = vars[0+v15_r11];
  int v20_r14 = vars[0];
  int v24 = (v2_r1 == 1);
  atom_1_r1_1 = v24;
  int v25 = (v20_r14 == 0);
  atom_1_r14_0 = v25;
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
  atom_1_r14_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v21 = atom_1_r1_1;
  int v22 = atom_1_r14_0;
  int v23_conj = v21 & v22;
  if (v23_conj == 1) assert(0);
  return 0;
}
