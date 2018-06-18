// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MOREDETOUR0814.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_1_r1_1; 
volatile int atom_1_r3_0; 
volatile int atom_1_r5_1; 
volatile int atom_3_r4_3; 
volatile int atom_3_r1_1; 

void *t0(void *arg){
label_1:;
  vars[0] = 4;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[1];
  int v3_cmpeq = (v2_r1 == v2_r1);
  if (v3_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;
  int v5_r3 = vars[0];
  int v7_r5 = vars[0];
  vars[0] = 3;
  int v24 = (v2_r1 == 1);
  atom_1_r1_1 = v24;
  int v25 = (v5_r3 == 0);
  atom_1_r3_0 = v25;
  int v26 = (v7_r5 == 1);
  atom_1_r5_1 = v26;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[0] = 1;
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v9_r1 = vars[0];
  vars[0] = 2;

  int v11_r4 = vars[0];
  int v27 = (v11_r4 == 3);
  atom_3_r4_3 = v27;
  int v28 = (v9_r1 == 1);
  atom_3_r1_1 = v28;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  vars[1] = 0;
  vars[0] = 0;
  atom_1_r1_1 = 0;
  atom_1_r3_0 = 0;
  atom_1_r5_1 = 0;
  atom_3_r4_3 = 0;
  atom_3_r1_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v12 = vars[0];
  int v13 = (v12 == 4);
  int v14 = atom_1_r1_1;
  int v15 = atom_1_r3_0;
  int v16 = atom_1_r5_1;
  int v17 = atom_3_r4_3;
  int v18 = atom_3_r1_1;
  int v19_conj = v17 & v18;
  int v20_conj = v16 & v19_conj;
  int v21_conj = v15 & v20_conj;
  int v22_conj = v14 & v21_conj;
  int v23_conj = v13 & v22_conj;
  if (v23_conj == 1) assert(0);
  return 0;
}
