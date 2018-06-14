// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MOREDETOUR0694.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_1_r4_3; 
volatile int atom_1_r1_1; 
volatile int atom_2_r3_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  int v2_r3 = vars[1];
  int v3_r5 = v2_r3 ^ v2_r3;
  vars[2+v3_r5] = 1;
  vars[2] = 3;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v5_r1 = vars[2];
  vars[2] = 2;

  int v7_r4 = vars[2];
  int v18 = (v7_r4 == 3);
  atom_1_r4_3 = v18;
  int v19 = (v5_r1 == 1);
  atom_1_r1_1 = v19;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[2] = 4;

  int v9_r3 = vars[0];
  int v20 = (v9_r3 == 0);
  atom_2_r3_0 = v20;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[2] = 0;
  vars[1] = 0;
  vars[0] = 0;
  atom_1_r4_3 = 0;
  atom_1_r1_1 = 0;
  atom_2_r3_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v10 = vars[2];
  int v11 = (v10 == 4);
  int v12 = atom_1_r4_3;
  int v13 = atom_1_r1_1;
  int v14 = atom_2_r3_0;
  int v15_conj = v13 & v14;
  int v16_conj = v12 & v15_conj;
  int v17_conj = v11 & v16_conj;
  if (v17_conj == 1) assert(0);
  return 0;
}
