// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/hh4.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_1_r2_2; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r2 = vars[1];
  int v3_r9 = v2_r2 ^ v2_r2;
  vars[0+v3_r9] = 1;
  int v11 = (v2_r2 == 2);
  atom_1_r2_2 = v11;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[1] = 2;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[0] = 0;
  vars[1] = 0;
  atom_1_r2_2 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v4 = atom_1_r2_2;
  int v5 = vars[0];
  int v6 = (v5 == 2);
  int v7 = vars[1];
  int v8 = (v7 == 2);
  int v9_conj = v6 & v8;
  int v10_conj = v4 & v9_conj;
  if (v10_conj == 1) assert(0);
  return 0;
}
