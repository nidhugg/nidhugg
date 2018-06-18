// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/LB+isync+data.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r1_1; 
volatile int atom_1_r1_1; 

void *t0(void *arg){
label_1:;
  int v2_r1 = vars[0];

  vars[1] = 1;
  int v10 = (v2_r1 == 1);
  atom_0_r1_1 = v10;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v4_r1 = vars[1];
  int v5_r3 = v4_r1 ^ v4_r1;
  int v6_r3 = v5_r3 + 1;
  vars[0] = v6_r3;
  int v11 = (v4_r1 == 1);
  atom_1_r1_1 = v11;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 0;
  vars[1] = 0;
  atom_0_r1_1 = 0;
  atom_1_r1_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v7 = atom_0_r1_1;
  int v8 = atom_1_r1_1;
  int v9_conj = v7 & v8;
  if (v9_conj == 1) assert(0);
  return 0;
}
