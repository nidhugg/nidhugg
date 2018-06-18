// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MP0021.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[4]; 
volatile int atom_1_r1_1; 
volatile int atom_1_r4_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  vars[1] = 1;
  int v2_r5 = vars[2];
  int v3_r7 = v2_r5 ^ v2_r5;
  int v4_r7 = v3_r7 + 1;
  vars[3] = v4_r7;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v6_r1 = vars[3];
  int v7_r3 = v6_r1 ^ v6_r1;
  int v10_r4 = vars[0+v7_r3];
  int v14 = (v6_r1 == 1);
  atom_1_r1_1 = v14;
  int v15 = (v10_r4 == 0);
  atom_1_r4_0 = v15;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[1] = 0;
  vars[3] = 0;
  vars[2] = 0;
  vars[0] = 0;
  atom_1_r1_1 = 0;
  atom_1_r4_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v11 = atom_1_r1_1;
  int v12 = atom_1_r4_0;
  int v13_conj = v11 & v12;
  if (v13_conj == 1) assert(0);
  return 0;
}
