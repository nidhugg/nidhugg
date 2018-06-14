// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/CoWW2.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[1]; 
volatile int atom_0_r3_3; 
volatile int atom_1_r3_3; 
volatile int atom_1_r3_2; 
volatile int atom_0_r3_1; 
volatile int atom_1_r3_1; 
volatile int atom_0_r3_4; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  int v2_r3 = vars[0];
  vars[0] = 2;
  int v38 = (v2_r3 == 3);
  atom_0_r3_3 = v38;
  int v41 = (v2_r3 == 1);
  atom_0_r3_1 = v41;
  int v43 = (v2_r3 == 4);
  atom_0_r3_4 = v43;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[0] = 3;
  int v4_r3 = vars[0];
  vars[0] = 4;
  int v39 = (v4_r3 == 3);
  atom_1_r3_3 = v39;
  int v40 = (v4_r3 == 2);
  atom_1_r3_2 = v40;
  int v42 = (v4_r3 == 1);
  atom_1_r3_1 = v42;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 0;
  atom_0_r3_3 = 0;
  atom_1_r3_3 = 0;
  atom_1_r3_2 = 0;
  atom_0_r3_1 = 0;
  atom_1_r3_1 = 0;
  atom_0_r3_4 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v5 = vars[0];
  int v6 = (v5 == 4);
  int v7 = atom_0_r3_3;
  int v8 = atom_1_r3_3;
  int v9 = atom_1_r3_2;
  int v10_disj = v8 | v9;
  int v11_conj = v7 & v10_disj;
  int v12 = atom_0_r3_1;
  int v13 = atom_1_r3_3;
  int v14 = atom_1_r3_2;
  int v15 = atom_1_r3_1;
  int v16_disj = v14 | v15;
  int v17_disj = v13 | v16_disj;
  int v18_conj = v12 & v17_disj;
  int v19_disj = v11_conj | v18_conj;
  int v20_conj = v6 & v19_disj;
  int v21 = vars[0];
  int v22 = (v21 == 2);
  int v23 = atom_1_r3_3;
  int v24 = atom_0_r3_4;
  int v25 = atom_0_r3_3;
  int v26 = atom_0_r3_1;
  int v27_disj = v25 | v26;
  int v28_disj = v24 | v27_disj;
  int v29_conj = v23 & v28_disj;
  int v30 = atom_1_r3_1;
  int v31 = atom_0_r3_4;
  int v32 = atom_0_r3_1;
  int v33_disj = v31 | v32;
  int v34_conj = v30 & v33_disj;
  int v35_disj = v29_conj | v34_conj;
  int v36_conj = v22 & v35_disj;
  int v37_disj = v20_conj | v36_conj;
  if (v37_disj == 0) assert(0);
  return 0;
}
