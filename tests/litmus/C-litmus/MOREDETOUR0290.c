// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MOREDETOUR0290.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r5_5; 
volatile int atom_1_r4_4; 
volatile int atom_1_r1_2; 
volatile int atom_2_r5_2; 
volatile int atom_2_r1_1; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;
  vars[0] = 4;
  vars[0] = 5;
  int v2_r5 = vars[0];
  int v3_r6 = v2_r5 ^ v2_r5;
  int v4_r6 = v3_r6 + 1;
  vars[1] = v4_r6;
  int v24 = (v2_r5 == 5);
  atom_0_r5_5 = v24;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v6_r1 = vars[0];
  vars[0] = 3;

  int v8_r4 = vars[0];
  int v25 = (v8_r4 == 4);
  atom_1_r4_4 = v25;
  int v26 = (v6_r1 == 2);
  atom_1_r1_2 = v26;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v10_r1 = vars[1];
  int v11_r3 = v10_r1 ^ v10_r1;
  int v12_r3 = v11_r3 + 1;
  vars[0] = v12_r3;

  int v14_r5 = vars[0];
  int v27 = (v14_r5 == 2);
  atom_2_r5_2 = v27;
  int v28 = (v10_r1 == 1);
  atom_2_r1_1 = v28;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[1] = 0;
  vars[0] = 0;
  atom_0_r5_5 = 0;
  atom_1_r4_4 = 0;
  atom_1_r1_2 = 0;
  atom_2_r5_2 = 0;
  atom_2_r1_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v15 = atom_0_r5_5;
  int v16 = atom_1_r4_4;
  int v17 = atom_1_r1_2;
  int v18 = atom_2_r5_2;
  int v19 = atom_2_r1_1;
  int v20_conj = v18 & v19;
  int v21_conj = v17 & v20_conj;
  int v22_conj = v16 & v21_conj;
  int v23_conj = v15 & v22_conj;
  if (v23_conj == 1) assert(0);
  return 0;
}
