// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MOREDETOUR0326.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r6_2; 
volatile int atom_1_r1_2; 
volatile int atom_2_r1_4; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;
  vars[1] = 1;
  vars[1] = 2;
  int v2_r6 = vars[1];
  vars[1] = 4;
  int v20 = (v2_r6 == 2);
  atom_0_r6_2 = v20;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v4_r1 = vars[1];
  vars[1] = 3;
  int v21 = (v4_r1 == 2);
  atom_1_r1_2 = v21;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v6_r1 = vars[1];
  int v7_r3 = v6_r1 ^ v6_r1;
  int v8_r3 = v7_r3 + 1;
  vars[0] = v8_r3;
  int v22 = (v6_r1 == 4);
  atom_2_r1_4 = v22;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[0] = 0;
  vars[1] = 0;
  atom_0_r6_2 = 0;
  atom_1_r1_2 = 0;
  atom_2_r1_4 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v9 = atom_0_r6_2;
  int v10 = vars[1];
  int v11 = (v10 == 4);
  int v12 = atom_1_r1_2;
  int v13 = vars[0];
  int v14 = (v13 == 2);
  int v15 = atom_2_r1_4;
  int v16_conj = v14 & v15;
  int v17_conj = v12 & v16_conj;
  int v18_conj = v11 & v17_conj;
  int v19_conj = v9 & v18_conj;
  if (v19_conj == 1) assert(0);
  return 0;
}