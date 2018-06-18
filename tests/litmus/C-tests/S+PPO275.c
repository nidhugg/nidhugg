// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/S+PPO275.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
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
  vars[1] = 2;
  int v4_r4 = vars[1];
  int v6_r5 = vars[1];
  int v7_r6 = v6_r5 ^ v6_r5;
  vars[0+v7_r6] = 1;
  int v15 = (v2_r1 == 1);
  atom_1_r1_1 = v15;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[1] = 0;
  vars[0] = 0;
  atom_1_r1_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v8 = vars[0];
  int v9 = (v8 == 2);
  int v10 = vars[1];
  int v11 = (v10 == 2);
  int v12 = atom_1_r1_1;
  int v13_conj = v11 & v12;
  int v14_conj = v9 & v13_conj;
  if (v14_conj == 1) assert(0);
  return 0;
}
