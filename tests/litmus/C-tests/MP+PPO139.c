// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MP+PPO139.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_1_r1_1; 
volatile int atom_1_r7_0; 

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
  vars[1] = 2;
  int v6_r5 = vars[1];
  int v7_r6 = v6_r5 ^ v6_r5;
  int v10_r7 = vars[0+v7_r6];
  int v17 = (v2_r1 == 1);
  atom_1_r1_1 = v17;
  int v18 = (v10_r7 == 0);
  atom_1_r7_0 = v18;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[1] = 0;
  vars[0] = 0;
  atom_1_r1_1 = 0;
  atom_1_r7_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v11 = vars[1];
  int v12 = (v11 == 2);
  int v13 = atom_1_r1_1;
  int v14 = atom_1_r7_0;
  int v15_conj = v13 & v14;
  int v16_conj = v12 & v15_conj;
  if (v16_conj == 1) assert(0);
  return 0;
}
