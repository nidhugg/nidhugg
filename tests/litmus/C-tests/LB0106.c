// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/LB0106.litmus

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
  int v14 = (v2_r1 == 1);
  atom_0_r1_1 = v14;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v6_r1 = vars[1];
  int v8_r3 = vars[1];
  int v9_r4 = v8_r3 ^ v8_r3;
  int v10_r4 = v9_r4 + 1;
  vars[2] = v10_r4;
  vars[0] = 1;
  int v15 = (v6_r1 == 1);
  atom_1_r1_1 = v15;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 0;
  vars[2] = 0;
  vars[1] = 0;
  atom_0_r1_1 = 0;
  atom_1_r1_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v11 = atom_0_r1_1;
  int v12 = atom_1_r1_1;
  int v13_conj = v11 & v12;
  if (v13_conj == 1) assert(0);
  return 0;
}
