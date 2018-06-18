// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/safe001.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_1_r1_2; 
volatile int atom_2_r3_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  vars[0] = 2;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[0];
  int v3_r3 = v2_r1 ^ v2_r1;
  vars[1+v3_r3] = 1;
  int v15 = (v2_r1 == 2);
  atom_1_r1_2 = v15;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[1] = 2;

  int v5_r3 = vars[0];
  int v16 = (v5_r3 == 0);
  atom_2_r3_0 = v16;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[0] = 0;
  vars[1] = 0;
  atom_1_r1_2 = 0;
  atom_2_r3_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v6 = vars[0];
  int v7 = (v6 == 2);
  int v8 = vars[1];
  int v9 = (v8 == 2);
  int v10 = atom_1_r1_2;
  int v11 = atom_2_r3_0;
  int v12_conj = v10 & v11;
  int v13_conj = v9 & v12_conj;
  int v14_conj = v7 & v13_conj;
  if (v14_conj == 1) assert(0);
  return 0;
}
