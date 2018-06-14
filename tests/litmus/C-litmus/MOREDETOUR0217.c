// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MOREDETOUR0217.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r1_4; 
volatile int atom_1_r1_1; 
volatile int atom_1_r5_2; 
volatile int atom_3_r1_2; 

void *t0(void *arg){
label_1:;
  int v2_r1 = vars[0];
  int v3_r3 = v2_r1 ^ v2_r1;
  int v4_r3 = v3_r3 + 1;
  vars[1] = v4_r3;
  int v22 = (v2_r1 == 4);
  atom_0_r1_4 = v22;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v6_r1 = vars[1];
  int v7_cmpeq = (v6_r1 == v6_r1);
  if (v7_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;
  vars[0] = 1;
  int v9_r5 = vars[0];
  vars[0] = 4;
  int v23 = (v6_r1 == 1);
  atom_1_r1_1 = v23;
  int v24 = (v9_r5 == 2);
  atom_1_r5_2 = v24;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[0] = 2;
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v11_r1 = vars[0];
  vars[0] = 3;
  int v25 = (v11_r1 == 2);
  atom_3_r1_2 = v25;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  vars[0] = 0;
  vars[1] = 0;
  atom_0_r1_4 = 0;
  atom_1_r1_1 = 0;
  atom_1_r5_2 = 0;
  atom_3_r1_2 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v12 = atom_0_r1_4;
  int v13 = atom_1_r1_1;
  int v14 = atom_1_r5_2;
  int v15 = vars[0];
  int v16 = (v15 == 4);
  int v17 = atom_3_r1_2;
  int v18_conj = v16 & v17;
  int v19_conj = v14 & v18_conj;
  int v20_conj = v13 & v19_conj;
  int v21_conj = v12 & v20_conj;
  if (v21_conj == 1) assert(0);
  return 0;
}
