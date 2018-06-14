// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/alpha2.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[1]; 
volatile int atom_1_r1_2; 
volatile int atom_1_r2_3; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[0] = 3;
  int v2_r1 = vars[0];
  int v4_r2 = vars[0];
  int v8 = (v2_r1 == 2);
  atom_1_r1_2 = v8;
  int v9 = (v4_r2 == 3);
  atom_1_r2_3 = v9;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 1;
  atom_1_r1_2 = 0;
  atom_1_r2_3 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v5 = atom_1_r1_2;
  int v6 = atom_1_r2_3;
  int v7_conj = v5 & v6;
  if (v7_conj == 1) assert(0);
  return 0;
}
