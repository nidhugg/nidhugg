// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MOREDETOUR0340.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_0_r5_0; 
volatile int atom_2_r1_2; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;
  int v2_r3 = vars[1];
  int v4_r5 = vars[2];
  vars[2] = 2;
  int v18 = (v4_r5 == 0);
  atom_0_r5_0 = v18;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[2] = 1;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v6_r1 = vars[2];
  int v7_r3 = v6_r1 ^ v6_r1;
  int v8_r3 = v7_r3 + 1;
  vars[0] = v8_r3;
  int v19 = (v6_r1 == 2);
  atom_2_r1_2 = v19;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[2] = 0;
  vars[0] = 0;
  vars[1] = 0;
  atom_0_r5_0 = 0;
  atom_2_r1_2 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v9 = atom_0_r5_0;
  int v10 = vars[2];
  int v11 = (v10 == 2);
  int v12 = vars[0];
  int v13 = (v12 == 2);
  int v14 = atom_2_r1_2;
  int v15_conj = v13 & v14;
  int v16_conj = v11 & v15_conj;
  int v17_conj = v9 & v16_conj;
  if (v17_conj == 1) assert(0);
  return 0;
}