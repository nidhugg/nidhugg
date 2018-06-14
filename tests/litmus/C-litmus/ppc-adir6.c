// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/ppc-adir6.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r1_1; 
volatile int atom_0_r2_2; 
volatile int atom_1_r3_2; 

void *t0(void *arg){
label_1:;
  int v2_r1 = vars[0];
  vars[1] = 1;
  int v17 = (1 == 1);
  atom_0_r1_1 = v17;
  int v18 = (v2_r1 == 2);
  atom_0_r2_2 = v18;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v4_r3 = vars[1];
  int v5_r3 = v4_r3 + 1;
  vars[0] = v5_r3;
  int v19 = (v5_r3 == 2);
  atom_1_r3_2 = v19;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[1] = 0;
  vars[0] = 0;
  atom_0_r1_1 = 0;
  atom_0_r2_2 = 0;
  atom_1_r3_2 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v6 = atom_0_r1_1;
  int v7 = atom_0_r2_2;
  int v8 = atom_1_r3_2;
  int v9 = vars[1];
  int v10 = (v9 == 1);
  int v11 = vars[0];
  int v12 = (v11 == 2);
  int v13_conj = v10 & v12;
  int v14_conj = v8 & v13_conj;
  int v15_conj = v7 & v14_conj;
  int v16_conj = v6 & v15_conj;
  if (v16_conj == 1) assert(0);
  return 0;
}
