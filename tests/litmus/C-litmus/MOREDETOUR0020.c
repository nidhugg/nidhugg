// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MOREDETOUR0020.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_0_r3_1; 
volatile int atom_1_r1_1; 
volatile int atom_1_r4_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  int v2_r3 = vars[0];
  int v3_cmpeq = (v2_r3 == v2_r3);
  if (v3_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;
  int v5_r4 = vars[1];
  int v6_r6 = v5_r4 ^ v5_r4;
  int v7_r6 = v6_r6 + 1;
  vars[2] = v7_r6;
  int v19 = (v2_r3 == 1);
  atom_0_r3_1 = v19;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v9_r1 = vars[2];
  int v10_r3 = v9_r1 ^ v9_r1;
  int v13_r4 = vars[0+v10_r3];
  int v20 = (v9_r1 == 1);
  atom_1_r1_1 = v20;
  int v21 = (v13_r4 == 0);
  atom_1_r4_0 = v21;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[1] = 0;
  vars[0] = 0;
  vars[2] = 0;
  atom_0_r3_1 = 0;
  atom_1_r1_1 = 0;
  atom_1_r4_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v14 = atom_0_r3_1;
  int v15 = atom_1_r1_1;
  int v16 = atom_1_r4_0;
  int v17_conj = v15 & v16;
  int v18_conj = v14 & v17_conj;
  if (v18_conj == 1) assert(0);
  return 0;
}
