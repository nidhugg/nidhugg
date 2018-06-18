// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/ppo9.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r1_2; 
volatile int atom_0_r2_0; 

void *t0(void *arg){
label_1:;
  int v2_r1 = vars[0];
  vars[0] = 3;
  vars[0] = 4;
  int v4_r4 = vars[0];
  int v5_r10 = v4_r4 ^ v4_r4;
  int v8_r2 = vars[1+v5_r10];
  int v12 = (v2_r1 == 2);
  atom_0_r1_2 = v12;
  int v13 = (v8_r2 == 0);
  atom_0_r2_0 = v13;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 1;

  vars[0] = 2;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 0;
  vars[1] = 0;
  atom_0_r1_2 = 0;
  atom_0_r2_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v9 = atom_0_r1_2;
  int v10 = atom_0_r2_0;
  int v11_conj = v9 & v10;
  if (v11_conj == 1) assert(0);
  return 0;
}
