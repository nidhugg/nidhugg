// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/DETOUR0370.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_1_r5_2; 
volatile int atom_1_r1_2; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;
  vars[0] = 3;
  vars[1] = 1;
  vars[1] = 2;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[1];
  int v3_r3 = v2_r1 ^ v2_r1;
  int v4_r3 = v3_r3 + 1;
  vars[0] = v4_r3;

  int v6_r5 = vars[0];
  int v10 = (v6_r5 == 2);
  atom_1_r5_2 = v10;
  int v11 = (v2_r1 == 2);
  atom_1_r1_2 = v11;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[1] = 0;
  vars[0] = 0;
  atom_1_r5_2 = 0;
  atom_1_r1_2 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v7 = atom_1_r5_2;
  int v8 = atom_1_r1_2;
  int v9_conj = v7 & v8;
  if (v9_conj == 1) assert(0);
  return 0;
}
