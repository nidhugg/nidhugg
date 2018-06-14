// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MOREDETOUR0331.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r3_3; 
volatile int atom_0_r5_0; 
volatile int atom_3_r5_2; 
volatile int atom_3_r1_2; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;
  int v2_r3 = vars[0];
  int v3_r4 = v2_r3 ^ v2_r3;
  int v6_r5 = vars[1+v3_r4];
  vars[1] = 2;
  int v23 = (v2_r3 == 3);
  atom_0_r3_3 = v23;
  int v24 = (v6_r5 == 0);
  atom_0_r5_0 = v24;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[0] = 3;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[1] = 1;
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v8_r1 = vars[1];
  int v9_r3 = v8_r1 ^ v8_r1;
  int v10_r3 = v9_r3 + 1;
  vars[0] = v10_r3;

  int v12_r5 = vars[0];
  int v25 = (v12_r5 == 2);
  atom_3_r5_2 = v25;
  int v26 = (v8_r1 == 2);
  atom_3_r1_2 = v26;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  vars[1] = 0;
  vars[0] = 0;
  atom_0_r3_3 = 0;
  atom_0_r5_0 = 0;
  atom_3_r5_2 = 0;
  atom_3_r1_2 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v13 = atom_0_r3_3;
  int v14 = atom_0_r5_0;
  int v15 = vars[1];
  int v16 = (v15 == 2);
  int v17 = atom_3_r5_2;
  int v18 = atom_3_r1_2;
  int v19_conj = v17 & v18;
  int v20_conj = v16 & v19_conj;
  int v21_conj = v14 & v20_conj;
  int v22_conj = v13 & v21_conj;
  if (v22_conj == 1) assert(0);
  return 0;
}