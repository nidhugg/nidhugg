// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MOREDETOUR0882.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r5_2; 
volatile int atom_1_r5_5; 
volatile int atom_2_r4_4; 
volatile int atom_2_r1_2; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;

  vars[1] = 1;

  int v2_r5 = vars[1];
  int v20 = (v2_r5 == 2);
  atom_0_r5_2 = v20;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 2;
  vars[1] = 4;
  vars[1] = 5;
  int v4_r5 = vars[1];
  int v5_cmpeq = (v4_r5 == v4_r5);
  if (v5_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;
  vars[0] = 1;
  int v21 = (v4_r5 == 5);
  atom_1_r5_5 = v21;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v7_r1 = vars[1];
  vars[1] = 3;

  int v9_r4 = vars[1];
  int v22 = (v9_r4 == 4);
  atom_2_r4_4 = v22;
  int v23 = (v7_r1 == 2);
  atom_2_r1_2 = v23;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[1] = 0;
  vars[0] = 0;
  atom_0_r5_2 = 0;
  atom_1_r5_5 = 0;
  atom_2_r4_4 = 0;
  atom_2_r1_2 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v10 = atom_0_r5_2;
  int v11 = vars[0];
  int v12 = (v11 == 2);
  int v13 = atom_1_r5_5;
  int v14 = atom_2_r4_4;
  int v15 = atom_2_r1_2;
  int v16_conj = v14 & v15;
  int v17_conj = v13 & v16_conj;
  int v18_conj = v12 & v17_conj;
  int v19_conj = v10 & v18_conj;
  if (v19_conj == 1) assert(0);
  return 0;
}
