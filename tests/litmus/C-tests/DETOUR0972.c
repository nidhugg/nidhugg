// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/DETOUR0972.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_0_r3_1; 
volatile int atom_1_r3_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  int v2_r3 = vars[0];
  int v4_r4 = vars[1];
  int v5_r6 = v4_r4 ^ v4_r4;
  vars[2+v5_r6] = 1;
  int v14 = (v2_r3 == 1);
  atom_0_r3_1 = v14;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[2] = 2;

  int v7_r3 = vars[0];
  int v15 = (v7_r3 == 0);
  atom_1_r3_0 = v15;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 0;
  vars[1] = 0;
  vars[2] = 0;
  atom_0_r3_1 = 0;
  atom_1_r3_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v8 = vars[2];
  int v9 = (v8 == 2);
  int v10 = atom_0_r3_1;
  int v11 = atom_1_r3_0;
  int v12_conj = v10 & v11;
  int v13_conj = v9 & v12_conj;
  if (v13_conj == 1) assert(0);
  return 0;
}
