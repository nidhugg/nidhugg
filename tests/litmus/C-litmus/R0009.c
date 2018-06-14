// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/R0009.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[4]; 
volatile int atom_1_r8_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 2;
  int v2_r3 = vars[2];
  int v3_r5 = v2_r3 ^ v2_r3;
  int v6_r6 = vars[3+v3_r5];
  int v7_cmpeq = (v6_r6 == v6_r6);
  if (v7_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;

  int v9_r8 = vars[0];
  int v14 = (v9_r8 == 0);
  atom_1_r8_0 = v14;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 0;
  vars[2] = 0;
  vars[1] = 0;
  vars[3] = 0;
  atom_1_r8_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v10 = vars[1];
  int v11 = (v10 == 2);
  int v12 = atom_1_r8_0;
  int v13_conj = v11 & v12;
  if (v13_conj == 1) assert(0);
  return 0;
}
