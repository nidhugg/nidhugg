// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/rfi001.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r3_2; 
volatile int atom_0_r5_0; 
volatile int atom_1_r3_1; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;
  int v2_r3 = vars[0];
  int v3_r4 = v2_r3 ^ v2_r3;
  int v6_r5 = vars[1+v3_r4];
  int v18 = (v2_r3 == 2);
  atom_0_r3_2 = v18;
  int v19 = (v6_r5 == 0);
  atom_0_r5_0 = v19;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 1;
  int v8_r3 = vars[1];
  int v9_r4 = v8_r3 ^ v8_r3;
  vars[0+v9_r4] = 1;
  int v20 = (v8_r3 == 1);
  atom_1_r3_1 = v20;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[1] = 0;
  vars[0] = 0;
  atom_0_r3_2 = 0;
  atom_0_r5_0 = 0;
  atom_1_r3_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v10 = vars[0];
  int v11 = (v10 == 2);
  int v12 = atom_0_r3_2;
  int v13 = atom_0_r5_0;
  int v14 = atom_1_r3_1;
  int v15_conj = v13 & v14;
  int v16_conj = v12 & v15_conj;
  int v17_conj = v11 & v16_conj;
  if (v17_conj == 1) assert(0);
  return 0;
}
