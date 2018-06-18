// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/LB+PPO0571.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_0_r1_1; 
volatile int atom_1_r1_1; 

void *t0(void *arg){
label_1:;
  int v2_r1 = vars[0];
  int v3_r3 = v2_r1 ^ v2_r1;
  int v4_r3 = v3_r3 + 1;
  vars[1] = v4_r3;
  int v18 = (v2_r1 == 1);
  atom_0_r1_1 = v18;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v6_r1 = vars[1];
  int v8_r3 = vars[1];
  int v9_r4 = v8_r3 ^ v8_r3;
  int v10_r4 = v9_r4 + 1;
  vars[2] = v10_r4;
  int v12_r6 = vars[2];
  int v14_r7 = vars[2];
  vars[0] = 1;
  int v19 = (v6_r1 == 1);
  atom_1_r1_1 = v19;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[1] = 0;
  vars[0] = 0;
  vars[2] = 0;
  atom_0_r1_1 = 0;
  atom_1_r1_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v15 = atom_0_r1_1;
  int v16 = atom_1_r1_1;
  int v17_conj = v15 & v16;
  if (v17_conj == 1) assert(0);
  return 0;
}
