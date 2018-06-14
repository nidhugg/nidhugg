// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MOREDETOUR0847.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_1_r1_1; 
volatile int atom_2_r4_4; 
volatile int atom_2_r1_2; 

void *t0(void *arg){
label_1:;
  vars[0] = 5;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[1];
  int v3_cmpeq = (v2_r1 == v2_r1);
  if (v3_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;
  vars[0] = 1;
  vars[0] = 2;
  vars[0] = 4;
  int v16 = (v2_r1 == 1);
  atom_1_r1_1 = v16;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v5_r1 = vars[0];
  vars[0] = 3;

  int v7_r4 = vars[0];
  int v17 = (v7_r4 == 4);
  atom_2_r4_4 = v17;
  int v18 = (v5_r1 == 2);
  atom_2_r1_2 = v18;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[0] = 0;
  vars[1] = 0;
  atom_1_r1_1 = 0;
  atom_2_r4_4 = 0;
  atom_2_r1_2 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v8 = vars[0];
  int v9 = (v8 == 5);
  int v10 = atom_1_r1_1;
  int v11 = atom_2_r4_4;
  int v12 = atom_2_r1_2;
  int v13_conj = v11 & v12;
  int v14_conj = v10 & v13_conj;
  int v15_conj = v9 & v14_conj;
  if (v15_conj == 1) assert(0);
  return 0;
}
