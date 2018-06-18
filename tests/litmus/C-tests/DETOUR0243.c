// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/DETOUR0243.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r1_1; 
volatile int atom_1_r1_1; 
volatile int atom_1_r5_3; 

void *t0(void *arg){
label_1:;
  int v2_r1 = vars[0];
  int v3_r3 = v2_r1 ^ v2_r1;
  int v4_r3 = v3_r3 + 1;
  vars[1] = v4_r3;
  int v16 = (v2_r1 == 1);
  atom_0_r1_1 = v16;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v6_r1 = vars[1];
  vars[1] = 2;
  vars[1] = 3;
  int v8_r5 = vars[1];
  int v9_r6 = v8_r5 ^ v8_r5;
  int v10_r6 = v9_r6 + 1;
  vars[0] = v10_r6;
  int v17 = (v6_r1 == 1);
  atom_1_r1_1 = v17;
  int v18 = (v8_r5 == 3);
  atom_1_r5_3 = v18;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 0;
  vars[1] = 0;
  atom_0_r1_1 = 0;
  atom_1_r1_1 = 0;
  atom_1_r5_3 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v11 = atom_0_r1_1;
  int v12 = atom_1_r1_1;
  int v13 = atom_1_r5_3;
  int v14_conj = v12 & v13;
  int v15_conj = v11 & v14_conj;
  if (v15_conj == 1) assert(0);
  return 0;
}
