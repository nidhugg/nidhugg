// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/R+isyncs.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_1_r3_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 2;

  int v2_r3 = vars[0];
  int v7 = (v2_r3 == 0);
  atom_1_r3_0 = v7;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 0;
  vars[1] = 0;
  atom_1_r3_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v3 = vars[1];
  int v4 = (v3 == 2);
  int v5 = atom_1_r3_0;
  int v6_conj = v4 & v5;
  if (v6_conj == 1) assert(0);
  return 0;
}
