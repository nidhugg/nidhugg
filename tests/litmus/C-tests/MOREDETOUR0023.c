// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MOREDETOUR0023.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r5_4; 
volatile int atom_1_r4_3; 
volatile int atom_1_r1_1; 
volatile int atom_2_r1_1; 
volatile int atom_2_r4_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  vars[0] = 3;
  vars[0] = 4;
  int v2_r5 = vars[0];
  int v3_cmpeq = (v2_r5 == v2_r5);
  if (v3_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;
  vars[1] = 1;
  int v23 = (v2_r5 == 4);
  atom_0_r5_4 = v23;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v5_r1 = vars[0];
  vars[0] = 2;

  int v7_r4 = vars[0];
  int v24 = (v7_r4 == 3);
  atom_1_r4_3 = v24;
  int v25 = (v5_r1 == 1);
  atom_1_r1_1 = v25;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v9_r1 = vars[1];
  int v10_r3 = v9_r1 ^ v9_r1;
  int v13_r4 = vars[0+v10_r3];
  int v26 = (v9_r1 == 1);
  atom_2_r1_1 = v26;
  int v27 = (v13_r4 == 0);
  atom_2_r4_0 = v27;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[0] = 0;
  vars[1] = 0;
  atom_0_r5_4 = 0;
  atom_1_r4_3 = 0;
  atom_1_r1_1 = 0;
  atom_2_r1_1 = 0;
  atom_2_r4_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v14 = atom_0_r5_4;
  int v15 = atom_1_r4_3;
  int v16 = atom_1_r1_1;
  int v17 = atom_2_r1_1;
  int v18 = atom_2_r4_0;
  int v19_conj = v17 & v18;
  int v20_conj = v16 & v19_conj;
  int v21_conj = v15 & v20_conj;
  int v22_conj = v14 & v21_conj;
  if (v22_conj == 1) assert(0);
  return 0;
}
