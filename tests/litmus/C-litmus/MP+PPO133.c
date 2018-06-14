// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MP+PPO133.litmus

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
  int v6_r4 = vars[2+v3_r3];
  int v7_r6 = v6_r4 ^ v6_r4;
  int v8_r6 = v7_r6 + 1;
  vars[3] = v8_r6;
  int v10_r8 = vars[3];
  vars[3] = 2;
  int v12_r10 = vars[3];
  int v13_r11 = v12_r10 ^ v12_r10;
  int v16_r12 = vars[0+v13_r11];
  int v23 = (v2_r1 == 1);
  atom_1_r1_1 = v23;
  int v24 = (v16_r12 == 0);
  atom_1_r12_0 = v24;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 0;
  vars[3] = 0;
  vars[2] = 0;
  vars[1] = 0;
  atom_1_r1_1 = 0;
  atom_1_r12_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v17 = vars[3];
  int v18 = (v17 == 2);
  int v19 = atom_1_r1_1;
  int v20 = atom_1_r12_0;
  int v21_conj = v19 & v20;
  int v22_conj = v18 & v21_conj;
  if (v22_conj == 1) assert(0);
  return 0;
}
