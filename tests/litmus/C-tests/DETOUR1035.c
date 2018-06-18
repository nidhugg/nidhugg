// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/DETOUR1035.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r3_0; 
volatile int atom_0_r6_1; 
volatile int atom_1_r3_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  int v2_r3 = vars[1];
  vars[1] = 1;
  int v4_r6 = vars[1];
  vars[1] = 2;
  int v15 = (v2_r3 == 0);
  atom_0_r3_0 = v15;
  int v16 = (v4_r6 == 1);
  atom_0_r6_1 = v16;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 3;

  int v6_r3 = vars[0];
  int v17 = (v6_r3 == 0);
  atom_1_r3_0 = v17;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 0;
  vars[1] = 0;
  atom_0_r3_0 = 0;
  atom_0_r6_1 = 0;
  atom_1_r3_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v7 = vars[1];
  int v8 = (v7 == 3);
  int v9 = atom_0_r3_0;
  int v10 = atom_0_r6_1;
  int v11 = atom_1_r3_0;
  int v12_conj = v10 & v11;
  int v13_conj = v9 & v12_conj;
  int v14_conj = v8 & v13_conj;
  if (v14_conj == 1) assert(0);
  return 0;
}
