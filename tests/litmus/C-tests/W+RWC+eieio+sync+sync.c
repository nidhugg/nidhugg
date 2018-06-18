// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/W+RWC+eieio+sync+sync.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_1_r1_1; 
volatile int atom_1_r3_0; 
volatile int atom_2_r3_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[1];

  int v4_r3 = vars[2];
  int v12 = (v2_r1 == 1);
  atom_1_r1_1 = v12;
  int v13 = (v4_r3 == 0);
  atom_1_r3_0 = v13;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[2] = 1;

  int v6_r3 = vars[0];
  int v14 = (v6_r3 == 0);
  atom_2_r3_0 = v14;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[0] = 0;
  vars[2] = 0;
  vars[1] = 0;
  atom_1_r1_1 = 0;
  atom_1_r3_0 = 0;
  atom_2_r3_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v7 = atom_1_r1_1;
  int v8 = atom_1_r3_0;
  int v9 = atom_2_r3_0;
  int v10_conj = v8 & v9;
  int v11_conj = v7 & v10_conj;
  if (v11_conj == 1) assert(0);
  return 0;
}
