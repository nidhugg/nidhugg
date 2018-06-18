// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/CO-MP.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[1]; 
volatile int atom_1_r0_2; 
volatile int atom_1_r1_1; 
volatile int atom_1_r1_0; 
volatile int atom_1_r0_1; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  vars[0] = 2;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r0 = vars[0];
  int v4_r1 = vars[0];
  int v16 = (v2_r0 == 2);
  atom_1_r0_2 = v16;
  int v17 = (v4_r1 == 1);
  atom_1_r1_1 = v17;
  int v18 = (v4_r1 == 0);
  atom_1_r1_0 = v18;
  int v19 = (v2_r0 == 1);
  atom_1_r0_1 = v19;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 0;
  atom_1_r0_2 = 0;
  atom_1_r1_1 = 0;
  atom_1_r1_0 = 0;
  atom_1_r0_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v5 = atom_1_r0_2;
  int v6 = atom_1_r1_1;
  int v7 = atom_1_r0_2;
  int v8 = atom_1_r1_0;
  int v9 = atom_1_r0_1;
  int v10 = atom_1_r1_0;
  int v11_conj = v9 & v10;
  int v12_disj = v8 | v11_conj;
  int v13_conj = v7 & v12_disj;
  int v14_disj = v6 | v13_conj;
  int v15_conj = v5 & v14_disj;
  if (v15_conj == 1) assert(0);
  return 0;
}
