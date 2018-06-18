// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/co4.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[1]; 
volatile int atom_0_r2_2; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  int v2_r2 = vars[0];
  int v7 = (v2_r2 == 2);
  atom_0_r2_2 = v7;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[0] = 2;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 0;
  atom_0_r2_2 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v3 = vars[0];
  int v4 = (v3 == 1);
  int v5 = atom_0_r2_2;
  int v6_conj = v4 & v5;
  if (v6_conj == 1) assert(0);
  return 0;
}
