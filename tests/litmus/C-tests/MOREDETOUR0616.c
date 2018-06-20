// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MOREDETOUR0616.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r5_2; 
volatile int atom_1_r3_3; 
volatile int atom_1_r4_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  vars[1] = 1;

  int v2_r5 = vars[1];
  int v13 = (v2_r5 == 2);
  atom_0_r5_2 = v13;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 2;
  int v4_r3 = vars[1];
  int v5_cmpeq = (v4_r3 == v4_r3);
  if (v5_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;
  int v7_r4 = vars[0];
  int v14 = (v4_r3 == 3);
  atom_1_r3_3 = v14;
  int v15 = (v7_r4 == 0);
  atom_1_r4_0 = v15;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[1] = 3;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[1] = 0;
  vars[0] = 0;
  atom_0_r5_2 = 0;
  atom_1_r3_3 = 0;
  atom_1_r4_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v8 = atom_0_r5_2;
  int v9 = atom_1_r3_3;
  int v10 = atom_1_r4_0;
  int v11_conj = v9 & v10;
  int v12_conj = v8 & v11_conj;
  if (v12_conj == 1) assert(0);
  return 0;
}