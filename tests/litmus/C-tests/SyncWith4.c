// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/SyncWith4.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[6]; 
volatile int atom_0_r1_1; 
volatile int atom_0_r4_0; 
volatile int atom_0_r6_1; 
volatile int atom_2_r1_1; 
volatile int atom_2_r4_0; 
volatile int atom_2_r6_1; 

void *t0(void *arg){
label_1:;
  int v2_r6 = vars[2];
  int v3_cmpeq = (v2_r6 == v2_r6);
  if (v3_cmpeq)  goto lbl_L0; else goto lbl_L0;
lbl_L0:;

  int v5_r1 = vars[0];
  int v7_r4 = vars[1];
  int v26 = (v5_r1 == 1);
  atom_0_r1_1 = v26;
  int v27 = (v7_r4 == 0);
  atom_0_r4_0 = v27;
  int v28 = (v2_r6 == 1);
  atom_0_r6_1 = v28;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 1;
  vars[3] = 1;

  vars[4] = 1;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v9_r6 = vars[4];
  int v10_cmpeq = (v9_r6 == v9_r6);
  if (v10_cmpeq)  goto lbl_L2; else goto lbl_L2;
lbl_L2:;

  int v12_r1 = vars[3];
  int v14_r4 = vars[5];
  int v29 = (v12_r1 == 1);
  atom_2_r1_1 = v29;
  int v30 = (v14_r4 == 0);
  atom_2_r4_0 = v30;
  int v31 = (v9_r6 == 1);
  atom_2_r6_1 = v31;
  return NULL;
}

void *t3(void *arg){
label_4:;
  vars[5] = 1;
  vars[0] = 1;

  vars[2] = 1;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  vars[0] = 0;
  vars[2] = 0;
  vars[3] = 0;
  vars[1] = 0;
  vars[4] = 0;
  vars[5] = 0;
  atom_0_r1_1 = 0;
  atom_0_r4_0 = 0;
  atom_0_r6_1 = 0;
  atom_2_r1_1 = 0;
  atom_2_r4_0 = 0;
  atom_2_r6_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v15 = atom_0_r1_1;
  int v16 = atom_0_r4_0;
  int v17 = atom_0_r6_1;
  int v18 = atom_2_r1_1;
  int v19 = atom_2_r4_0;
  int v20 = atom_2_r6_1;
  int v21_conj = v19 & v20;
  int v22_conj = v18 & v21_conj;
  int v23_conj = v17 & v22_conj;
  int v24_conj = v16 & v23_conj;
  int v25_conj = v15 & v24_conj;
  if (v25_conj == 1) assert(0);
  return 0;
}
