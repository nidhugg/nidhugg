// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MOREDETOUR0286.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r4_5; 
volatile int atom_1_r4_4; 
volatile int atom_1_r1_2; 
volatile int atom_3_r5_2; 
volatile int atom_3_r1_1; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;
  vars[0] = 4;
  int v2_r4 = vars[0];
  int v3_r5 = v2_r4 ^ v2_r4;
  vars[1+v3_r5] = 1;
  int v23 = (v2_r4 == 5);
  atom_0_r4_5 = v23;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v5_r1 = vars[0];
  vars[0] = 3;

  int v7_r4 = vars[0];
  int v24 = (v7_r4 == 4);
  atom_1_r4_4 = v24;
  int v25 = (v5_r1 == 2);
  atom_1_r1_2 = v25;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[0] = 5;
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v9_r1 = vars[1];
  int v10_r3 = v9_r1 ^ v9_r1;
  int v11_r3 = v10_r3 + 1;
  vars[0] = v11_r3;

  int v13_r5 = vars[0];
  int v26 = (v13_r5 == 2);
  atom_3_r5_2 = v26;
  int v27 = (v9_r1 == 1);
  atom_3_r1_1 = v27;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  vars[0] = 0;
  vars[1] = 0;
  atom_0_r4_5 = 0;
  atom_1_r4_4 = 0;
  atom_1_r1_2 = 0;
  atom_3_r5_2 = 0;
  atom_3_r1_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v14 = atom_0_r4_5;
  int v15 = atom_1_r4_4;
  int v16 = atom_1_r1_2;
  int v17 = atom_3_r5_2;
  int v18 = atom_3_r1_1;
  int v19_conj = v17 & v18;
  int v20_conj = v16 & v19_conj;
  int v21_conj = v15 & v20_conj;
  int v22_conj = v14 & v21_conj;
  if (v22_conj == 1) assert(0);
  return 0;
}
