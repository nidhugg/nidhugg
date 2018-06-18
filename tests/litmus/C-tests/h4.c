// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/h4.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_1_r2_1; 

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
  int v8 = (v2_r2 == 1);
  atom_1_r2_1 = v8;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[1] = 0;
  vars[0] = 0;
  atom_1_r2_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v4 = atom_1_r2_1;
  int v5 = vars[0];
  int v6 = (v5 == 2);
  int v7_conj = v4 & v6;
  if (v7_conj == 1) assert(0);
  return 0;
}
