// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/S+PPO038.litmus

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
  vars[2] = 2;
  int v8_r8 = vars[2];
  int v9_r9 = v8_r8 ^ v8_r8;
  int v12_r10 = vars[3+v9_r9];
  int v13_r12 = v12_r10 ^ v12_r10;
  vars[0+v13_r12] = 1;
  int v21 = (v2_r1 == 1);
  atom_1_r1_1 = v21;
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

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v14 = vars[0];
  int v15 = (v14 == 2);
  int v16 = vars[2];
  int v17 = (v16 == 2);
  int v18 = atom_1_r1_1;
  int v19_conj = v17 & v18;
  int v20_conj = v15 & v19_conj;
  if (v20_conj == 1) assert(0);
  return 0;
}