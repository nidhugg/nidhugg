// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MOREDETOUR0221.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r1_3; 
volatile int atom_1_r1_1; 
volatile int atom_1_r3_0; 
volatile int atom_1_r5_1; 
volatile int atom_3_r1_1; 

void *t0(void *arg){
label_1:;
  int v2_r1 = vars[0];
  int v3_r3 = v2_r1 ^ v2_r1;
  int v4_r3 = v3_r3 + 1;
  vars[1] = v4_r3;
  int v26 = (v2_r1 == 3);
  atom_0_r1_3 = v26;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v6_r1 = vars[1];
  int v7_cmpeq = (v6_r1 == v6_r1);
  if (v7_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;
  int v9_r3 = vars[0];
  int v11_r5 = vars[0];
  vars[0] = 3;
  int v27 = (v6_r1 == 1);
  atom_1_r1_1 = v27;
  int v28 = (v9_r3 == 0);
  atom_1_r3_0 = v28;
  int v29 = (v11_r5 == 1);
  atom_1_r5_1 = v29;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[0] = 1;
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v13_r1 = vars[0];
  vars[0] = 2;
  int v30 = (v13_r1 == 1);
  atom_3_r1_1 = v30;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  vars[1] = 0;
  vars[0] = 0;
  atom_0_r1_3 = 0;
  atom_1_r1_1 = 0;
  atom_1_r3_0 = 0;
  atom_1_r5_1 = 0;
  atom_3_r1_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v14 = atom_0_r1_3;
  int v15 = atom_1_r1_1;
  int v16 = atom_1_r3_0;
  int v17 = atom_1_r5_1;
  int v18 = vars[0];
  int v19 = (v18 == 3);
  int v20 = atom_3_r1_1;
  int v21_conj = v19 & v20;
  int v22_conj = v17 & v21_conj;
  int v23_conj = v16 & v22_conj;
  int v24_conj = v15 & v23_conj;
  int v25_conj = v14 & v24_conj;
  if (v25_conj == 1) assert(0);
  return 0;
}
