// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/CO-LB+fri+pos-fri.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[1]; 
volatile int atom_0_r1_0; 
volatile int atom_1_r3_1; 
volatile int atom_1_r1_1; 
volatile int atom_1_r1_0; 
volatile int atom_1_r3_0; 
volatile int atom_0_r1_2; 

void *t0(void *arg){
label_1:;
  int v2_r1 = vars[0];
  vars[0] = 1;
  int v32 = (v2_r1 == 0);
  atom_0_r1_0 = v32;
  int v37 = (v2_r1 == 2);
  atom_0_r1_2 = v37;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v4_r1 = vars[0];
  int v6_r3 = vars[0];
  vars[0] = 2;
  int v33 = (v6_r3 == 1);
  atom_1_r3_1 = v33;
  int v34 = (v4_r1 == 1);
  atom_1_r1_1 = v34;
  int v35 = (v4_r1 == 0);
  atom_1_r1_0 = v35;
  int v36 = (v6_r3 == 0);
  atom_1_r3_0 = v36;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 0;
  atom_0_r1_0 = 0;
  atom_1_r3_1 = 0;
  atom_1_r1_1 = 0;
  atom_1_r1_0 = 0;
  atom_1_r3_0 = 0;
  atom_0_r1_2 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v7 = vars[0];
  int v8 = (v7 == 2);
  int v9 = atom_0_r1_0;
  int v10 = atom_1_r3_1;
  int v11 = atom_1_r1_1;
  int v12 = atom_1_r1_0;
  int v13_disj = v11 | v12;
  int v14_conj = v10 & v13_disj;
  int v15 = atom_1_r3_0;
  int v16 = atom_1_r1_0;
  int v17_conj = v15 & v16;
  int v18_disj = v14_conj | v17_conj;
  int v19_conj = v9 & v18_disj;
  int v20_conj = v8 & v19_conj;
  int v21 = vars[0];
  int v22 = (v21 == 1);
  int v23 = atom_1_r3_0;
  int v24 = atom_1_r1_0;
  int v25 = atom_0_r1_2;
  int v26 = atom_0_r1_0;
  int v27_disj = v25 | v26;
  int v28_conj = v24 & v27_disj;
  int v29_conj = v23 & v28_conj;
  int v30_conj = v22 & v29_conj;
  int v31_disj = v20_conj | v30_conj;
  if (v31_disj == 0) assert(0);
  return 0;
}
