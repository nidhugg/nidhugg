// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/LB+sync+ctrl.litmus

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
  int v9 = (v2_r1 == 1);
  atom_0_r1_1 = v9;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v4_r1 = vars[1];
  int v5_cmpeq = (v4_r1 == v4_r1);
  if (v5_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;
  vars[0] = 1;
  int v10 = (v4_r1 == 1);
  atom_1_r1_1 = v10;
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

  int v6 = atom_0_r1_1;
  int v7 = atom_1_r1_1;
  int v8_conj = v6 & v7;
  if (v8_conj == 1) assert(0);
  return 0;
}
