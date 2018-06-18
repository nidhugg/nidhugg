// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/IRIW+sync+ctrl.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_1_r1_1; 
volatile int atom_1_r3_0; 
volatile int atom_3_r1_1; 
volatile int atom_3_r3_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[0];

  int v4_r3 = vars[1];
  int v17 = (v2_r1 == 1);
  atom_1_r1_1 = v17;
  int v18 = (v4_r3 == 0);
  atom_1_r3_0 = v18;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[1] = 1;
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v6_r1 = vars[1];
  int v7_cmpeq = (v6_r1 == v6_r1);
  if (v7_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;
  int v9_r3 = vars[0];
  int v19 = (v6_r1 == 1);
  atom_3_r1_1 = v19;
  int v20 = (v9_r3 == 0);
  atom_3_r3_0 = v20;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  vars[0] = 0;
  vars[1] = 0;
  atom_1_r1_1 = 0;
  atom_1_r3_0 = 0;
  atom_3_r1_1 = 0;
  atom_3_r3_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v10 = atom_1_r1_1;
  int v11 = atom_1_r3_0;
  int v12 = atom_3_r1_1;
  int v13 = atom_3_r3_0;
  int v14_conj = v12 & v13;
  int v15_conj = v11 & v14_conj;
  int v16_conj = v10 & v15_conj;
  if (v16_conj == 1) assert(0);
  return 0;
}
