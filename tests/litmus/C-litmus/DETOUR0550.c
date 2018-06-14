// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/DETOUR0550.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r3_0; 
volatile int atom_1_r3_1; 
volatile int atom_1_r7_1; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;

  int v2_r3 = vars[1];
  int v16 = (v2_r3 == 0);
  atom_0_r3_0 = v16;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 1;
  int v4_r3 = vars[1];
  int v5_r4 = v4_r3 ^ v4_r3;
  vars[0+v5_r4] = 1;
  int v7_r7 = vars[0];
  int v17 = (v4_r3 == 1);
  atom_1_r3_1 = v17;
  int v18 = (v7_r7 == 1);
  atom_1_r7_1 = v18;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[1] = 0;
  vars[0] = 0;
  atom_0_r3_0 = 0;
  atom_1_r3_1 = 0;
  atom_1_r7_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v8 = atom_0_r3_0;
  int v9 = vars[0];
  int v10 = (v9 == 2);
  int v11 = atom_1_r3_1;
  int v12 = atom_1_r7_1;
  int v13_conj = v11 & v12;
  int v14_conj = v10 & v13_conj;
  int v15_conj = v8 & v14_conj;
  if (v15_conj == 1) assert(0);
  return 0;
}
