// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MOREDETOUR0869.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r5_2; 
volatile int atom_2_r1_2; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;

  vars[1] = 1;

  int v2_r5 = vars[1];
  int v17 = (v2_r5 == 2);
  atom_0_r5_2 = v17;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 2;
  vars[1] = 4;
  int v4_r4 = vars[1];
  int v5_r5 = v4_r4 ^ v4_r4;
  vars[0+v5_r5] = 1;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v7_r1 = vars[1];
  vars[1] = 3;
  int v18 = (v7_r1 == 2);
  atom_2_r1_2 = v18;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[0] = 0;
  vars[1] = 0;
  atom_0_r5_2 = 0;
  atom_2_r1_2 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v8 = atom_0_r5_2;
  int v9 = vars[0];
  int v10 = (v9 == 2);
  int v11 = vars[1];
  int v12 = (v11 == 4);
  int v13 = atom_2_r1_2;
  int v14_conj = v12 & v13;
  int v15_conj = v10 & v14_conj;
  int v16_conj = v8 & v15_conj;
  if (v16_conj == 1) assert(0);
  return 0;
}
