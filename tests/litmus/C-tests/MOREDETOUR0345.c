// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MOREDETOUR0345.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r4_0; 
volatile int atom_2_r1_2; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;
  int v2_r3 = vars[0];
  int v3_cmpeq = (v2_r3 == v2_r3);
  if (v3_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;
  int v5_r4 = vars[1];
  vars[1] = 2;
  int v19 = (v5_r4 == 0);
  atom_0_r4_0 = v19;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 1;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v7_r1 = vars[1];
  int v8_r3 = v7_r1 ^ v7_r1;
  int v9_r3 = v8_r3 + 1;
  vars[0] = v9_r3;
  int v20 = (v7_r1 == 2);
  atom_2_r1_2 = v20;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[0] = 0;
  vars[1] = 0;
  atom_0_r4_0 = 0;
  atom_2_r1_2 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v10 = atom_0_r4_0;
  int v11 = vars[1];
  int v12 = (v11 == 2);
  int v13 = vars[0];
  int v14 = (v13 == 2);
  int v15 = atom_2_r1_2;
  int v16_conj = v14 & v15;
  int v17_conj = v12 & v16_conj;
  int v18_conj = v10 & v17_conj;
  if (v18_conj == 1) assert(0);
  return 0;
}
