// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/CO-S+sync+po.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[1]; 
volatile int atom_1_r0_2; 
volatile int atom_1_r0_1; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  vars[0] = 2;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r0 = vars[0];
  vars[0] = 3;
  int v12 = (v2_r0 == 2);
  atom_1_r0_2 = v12;
  int v13 = (v2_r0 == 1);
  atom_1_r0_1 = v13;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 0;
  atom_1_r0_2 = 0;
  atom_1_r0_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v3 = atom_1_r0_2;
  int v4 = vars[0];
  int v5 = (v4 == 2);
  int v6 = atom_1_r0_1;
  int v7 = vars[0];
  int v8 = (v7 == 1);
  int v9_conj = v6 & v8;
  int v10_disj = v5 | v9_conj;
  int v11_conj = v3 & v10_disj;
  if (v11_conj == 1) assert(0);
  return 0;
}
