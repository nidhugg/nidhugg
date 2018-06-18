// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/R0007.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[4]; 
volatile int atom_1_r8_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 2;
  int v2_r3 = vars[2];
  int v4_r5 = vars[3];
  int v5_r7 = v4_r5 ^ v4_r5;
  int v8_r8 = vars[0+v5_r7];
  int v13 = (v8_r8 == 0);
  atom_1_r8_0 = v13;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[2] = 0;
  vars[0] = 0;
  vars[1] = 0;
  vars[3] = 0;
  atom_1_r8_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v9 = vars[1];
  int v10 = (v9 == 2);
  int v11 = atom_1_r8_0;
  int v12_conj = v10 & v11;
  if (v12_conj == 1) assert(0);
  return 0;
}
