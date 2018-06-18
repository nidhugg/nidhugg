// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/CO-SBI.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[1]; 
volatile int atom_0_r2_2; 
volatile int atom_0_r1_2; 
volatile int atom_1_r2_1; 
volatile int atom_1_r1_1; 
volatile int atom_0_r2_1; 
volatile int atom_1_r2_2; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  int v2_r1 = vars[0];
  int v4_r2 = vars[0];
  int v24 = (v4_r2 == 2);
  atom_0_r2_2 = v24;
  int v25 = (v2_r1 == 2);
  atom_0_r1_2 = v25;
  int v28 = (v4_r2 == 1);
  atom_0_r2_1 = v28;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[0] = 2;
  int v6_r1 = vars[0];
  int v8_r2 = vars[0];
  int v26 = (v8_r2 == 1);
  atom_1_r2_1 = v26;
  int v27 = (v6_r1 == 1);
  atom_1_r1_1 = v27;
  int v29 = (v8_r2 == 2);
  atom_1_r2_2 = v29;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 0;
  atom_0_r2_2 = 0;
  atom_0_r1_2 = 0;
  atom_1_r2_1 = 0;
  atom_1_r1_1 = 0;
  atom_0_r2_1 = 0;
  atom_1_r2_2 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v9 = atom_0_r2_2;
  int v10 = atom_0_r1_2;
  int v11_disj = v9 | v10;
  int v12 = atom_1_r2_1;
  int v13 = atom_1_r1_1;
  int v14_disj = v12 | v13;
  int v15_conj = v11_disj & v14_disj;
  int v16 = atom_0_r1_2;
  int v17 = atom_0_r2_1;
  int v18_conj = v16 & v17;
  int v19 = atom_1_r1_1;
  int v20 = atom_1_r2_2;
  int v21_conj = v19 & v20;
  int v22_disj = v18_conj | v21_conj;
  int v23_disj = v15_conj | v22_disj;
  if (v23_disj == 1) assert(0);
  return 0;
}
