// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MOREDETOUR0076.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r3_1; 
volatile int atom_1_r1_1; 
volatile int atom_2_r1_3; 
volatile int atom_2_r4_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  int v2_r3 = vars[0];
  int v3_r4 = v2_r3 ^ v2_r3;
  vars[1+v3_r4] = 1;
  vars[1] = 3;
  int v22 = (v2_r3 == 1);
  atom_0_r3_1 = v22;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v5_r1 = vars[1];
  vars[1] = 2;
  int v23 = (v5_r1 == 1);
  atom_1_r1_1 = v23;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v7_r1 = vars[1];
  int v8_r3 = v7_r1 ^ v7_r1;
  int v11_r4 = vars[0+v8_r3];
  int v24 = (v7_r1 == 3);
  atom_2_r1_3 = v24;
  int v25 = (v11_r4 == 0);
  atom_2_r4_0 = v25;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[0] = 0;
  vars[1] = 0;
  atom_0_r3_1 = 0;
  atom_1_r1_1 = 0;
  atom_2_r1_3 = 0;
  atom_2_r4_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v12 = atom_0_r3_1;
  int v13 = vars[1];
  int v14 = (v13 == 3);
  int v15 = atom_1_r1_1;
  int v16 = atom_2_r1_3;
  int v17 = atom_2_r4_0;
  int v18_conj = v16 & v17;
  int v19_conj = v15 & v18_conj;
  int v20_conj = v14 & v19_conj;
  int v21_conj = v12 & v20_conj;
  if (v21_conj == 1) assert(0);
  return 0;
}
