// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/R0068.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[4]; 
volatile int atom_1_r3_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  int v2_r3 = vars[1];
  int v3_r5 = v2_r3 ^ v2_r3;
  int v6_r6 = vars[2+v3_r5];
  int v7_r8 = v6_r6 ^ v6_r6;
  int v8_r8 = v7_r8 + 1;
  vars[3] = v8_r8;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[3] = 2;

  int v10_r3 = vars[0];
  int v15 = (v10_r3 == 0);
  atom_1_r3_0 = v15;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[2] = 0;
  vars[0] = 0;
  vars[1] = 0;
  vars[3] = 0;
  atom_1_r3_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v11 = vars[3];
  int v12 = (v11 == 2);
  int v13 = atom_1_r3_0;
  int v14_conj = v12 & v13;
  if (v14_conj == 1) assert(0);
  return 0;
}
