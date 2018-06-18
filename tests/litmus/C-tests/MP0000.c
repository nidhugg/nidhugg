// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MP0000.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_1_r1_1; 
volatile int atom_1_r4_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  int v2_r3 = vars[0];
  int v3_r4 = v2_r3 ^ v2_r3;
  int v6_r5 = vars[1+v3_r4];
  int v7_r7 = v6_r5 ^ v6_r5;
  vars[2+v7_r7] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v9_r1 = vars[2];
  int v10_r3 = v9_r1 ^ v9_r1;
  int v13_r4 = vars[0+v10_r3];
  int v17 = (v9_r1 == 1);
  atom_1_r1_1 = v17;
  int v18 = (v13_r4 == 0);
  atom_1_r4_0 = v18;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[2] = 0;
  vars[1] = 0;
  vars[0] = 0;
  atom_1_r1_1 = 0;
  atom_1_r4_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v14 = atom_1_r1_1;
  int v15 = atom_1_r4_0;
  int v16_conj = v14 & v15;
  if (v16_conj == 1) assert(0);
  return 0;
}
