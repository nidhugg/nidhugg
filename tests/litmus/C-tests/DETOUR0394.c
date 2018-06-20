// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/DETOUR0394.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_0_r6_2; 
volatile int atom_1_r1_1; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;
  vars[1] = 1;
  vars[1] = 2;
  int v2_r6 = vars[1];
  int v3_r7 = v2_r6 ^ v2_r6;
  vars[2+v3_r7] = 1;
  int v14 = (v2_r6 == 2);
  atom_0_r6_2 = v14;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v5_r1 = vars[2];
  int v6_r3 = v5_r1 ^ v5_r1;
  int v7_r3 = v6_r3 + 1;
  vars[0] = v7_r3;
  int v15 = (v5_r1 == 1);
  atom_1_r1_1 = v15;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 0;
  vars[2] = 0;
  vars[1] = 0;
  atom_0_r6_2 = 0;
  atom_1_r1_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v8 = atom_0_r6_2;
  int v9 = vars[0];
  int v10 = (v9 == 2);
  int v11 = atom_1_r1_1;
  int v12_conj = v10 & v11;
  int v13_conj = v8 & v12_conj;
  if (v13_conj == 1) assert(0);
  return 0;
}