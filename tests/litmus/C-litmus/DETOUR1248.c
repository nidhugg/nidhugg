// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/DETOUR1248.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r5_2; 
volatile int atom_1_r5_4; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;

  vars[1] = 1;

  int v2_r5 = vars[1];
  int v13 = (v2_r5 == 2);
  atom_0_r5_2 = v13;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 2;
  vars[1] = 3;
  vars[1] = 4;
  int v4_r5 = vars[1];
  int v5_r6 = v4_r5 ^ v4_r5;
  int v6_r6 = v5_r6 + 1;
  vars[0] = v6_r6;
  int v14 = (v4_r5 == 4);
  atom_1_r5_4 = v14;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[1] = 0;
  vars[0] = 0;
  atom_0_r5_2 = 0;
  atom_1_r5_4 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v7 = atom_0_r5_2;
  int v8 = vars[0];
  int v9 = (v8 == 2);
  int v10 = atom_1_r5_4;
  int v11_conj = v9 & v10;
  int v12_conj = v7 & v11_conj;
  if (v12_conj == 1) assert(0);
  return 0;
}