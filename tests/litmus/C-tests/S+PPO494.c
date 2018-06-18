// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/S+PPO494.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
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
  vars[2+v3_r3] = 1;
  int v5_r6 = vars[2];
  int v7_r7 = vars[2];
  vars[2] = 2;
  int v9_r9 = vars[2];
  int v10_r10 = v9_r9 ^ v9_r9;
  int v11_r10 = v10_r10 + 1;
  vars[0] = v11_r10;
  int v19 = (v2_r1 == 1);
  atom_1_r1_1 = v19;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[1] = 0;
  vars[0] = 0;
  vars[2] = 0;
  atom_1_r1_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v12 = vars[0];
  int v13 = (v12 == 2);
  int v14 = vars[2];
  int v15 = (v14 == 2);
  int v16 = atom_1_r1_1;
  int v17_conj = v15 & v16;
  int v18_conj = v13 & v17_conj;
  if (v18_conj == 1) assert(0);
  return 0;
}
