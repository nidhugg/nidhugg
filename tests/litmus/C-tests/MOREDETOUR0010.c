// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MOREDETOUR0010.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_1_r1_1; 
volatile int atom_2_r1_1; 
volatile int atom_2_r4_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  vars[0] = 3;
  int v2_r4 = vars[0];
  int v3_r5 = v2_r4 ^ v2_r4;
  vars[1+v3_r5] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v5_r1 = vars[0];
  vars[0] = 2;
  int v20 = (v5_r1 == 1);
  atom_1_r1_1 = v20;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v7_r1 = vars[1];
  int v8_r3 = v7_r1 ^ v7_r1;
  int v11_r4 = vars[0+v8_r3];
  int v21 = (v7_r1 == 1);
  atom_2_r1_1 = v21;
  int v22 = (v11_r4 == 0);
  atom_2_r4_0 = v22;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[0] = 0;
  vars[1] = 0;
  atom_1_r1_1 = 0;
  atom_2_r1_1 = 0;
  atom_2_r4_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v12 = vars[0];
  int v13 = (v12 == 3);
  int v14 = atom_1_r1_1;
  int v15 = atom_2_r1_1;
  int v16 = atom_2_r4_0;
  int v17_conj = v15 & v16;
  int v18_conj = v14 & v17_conj;
  int v19_conj = v13 & v18_conj;
  if (v19_conj == 1) assert(0);
  return 0;
}
