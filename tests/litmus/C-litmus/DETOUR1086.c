// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/DETOUR1086.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_1_r1_1; 
volatile int atom_1_r5_3; 

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
  vars[1] = 3;
  int v4_r5 = vars[1];
  int v5_r6 = v4_r5 ^ v4_r5;
  vars[0+v5_r6] = 1;
  int v12 = (v2_r1 == 1);
  atom_1_r1_1 = v12;
  int v13 = (v4_r5 == 3);
  atom_1_r5_3 = v13;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 0;
  vars[1] = 0;
  atom_1_r1_1 = 0;
  atom_1_r5_3 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v6 = vars[0];
  int v7 = (v6 == 2);
  int v8 = atom_1_r1_1;
  int v9 = atom_1_r5_3;
  int v10_conj = v8 & v9;
  int v11_conj = v7 & v10_conj;
  if (v11_conj == 1) assert(0);
  return 0;
}
