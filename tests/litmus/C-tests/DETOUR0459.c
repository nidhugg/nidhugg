// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/DETOUR0459.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r3_2; 
volatile int atom_1_r1_2; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;
  int v2_r3 = vars[0];
  int v3_cmpeq = (v2_r3 == v2_r3);
  if (v3_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;
  vars[1] = 1;
  vars[1] = 2;
  int v14 = (v2_r3 == 2);
  atom_0_r3_2 = v14;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v5_r1 = vars[1];
  int v6_r3 = v5_r1 ^ v5_r1;
  int v7_r3 = v6_r3 + 1;
  vars[0] = v7_r3;
  int v15 = (v5_r1 == 2);
  atom_1_r1_2 = v15;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[1] = 0;
  vars[0] = 0;
  atom_0_r3_2 = 0;
  atom_1_r1_2 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v8 = atom_0_r3_2;
  int v9 = vars[0];
  int v10 = (v9 == 2);
  int v11 = atom_1_r1_2;
  int v12_conj = v10 & v11;
  int v13_conj = v8 & v12_conj;
  if (v13_conj == 1) assert(0);
  return 0;
}