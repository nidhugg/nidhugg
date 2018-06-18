// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/LB+PPO0150.litmus

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
  int v22 = (v2_r1 == 1);
  atom_0_r1_1 = v22;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v6_r1 = vars[1];
  int v7_r3 = v6_r1 ^ v6_r1;
  vars[2+v7_r3] = 1;
  vars[2] = 2;
  int v9_r7 = vars[2];
  int v10_r8 = v9_r7 ^ v9_r7;
  int v13_r9 = vars[3+v10_r8];
  int v14_r11 = v13_r9 ^ v13_r9;
  int v15_r11 = v14_r11 + 1;
  vars[4] = v15_r11;
  vars[0] = 1;
  int v23 = (v6_r1 == 1);
  atom_1_r1_1 = v23;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[4] = 0;
  vars[0] = 0;
  vars[1] = 0;
  vars[3] = 0;
  vars[2] = 0;
  atom_0_r1_1 = 0;
  atom_1_r1_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v16 = vars[2];
  int v17 = (v16 == 2);
  int v18 = atom_0_r1_1;
  int v19 = atom_1_r1_1;
  int v20_conj = v18 & v19;
  int v21_conj = v17 & v20_conj;
  if (v21_conj == 1) assert(0);
  return 0;
}
