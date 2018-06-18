// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/CO-S+wsi+pos-fri.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[1]; 
volatile int atom_1_r3_2; 
volatile int atom_1_r1_2; 
volatile int atom_1_r1_1; 
volatile int atom_1_r1_0; 
volatile int atom_1_r3_1; 
volatile int atom_1_r3_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  vars[0] = 2;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[0];
  int v4_r3 = vars[0];
  vars[0] = 3;
  int v38 = (v4_r3 == 2);
  atom_1_r3_2 = v38;
  int v39 = (v2_r1 == 2);
  atom_1_r1_2 = v39;
  int v40 = (v2_r1 == 1);
  atom_1_r1_1 = v40;
  int v41 = (v2_r1 == 0);
  atom_1_r1_0 = v41;
  int v42 = (v4_r3 == 1);
  atom_1_r3_1 = v42;
  int v43 = (v4_r3 == 0);
  atom_1_r3_0 = v43;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 0;
  atom_1_r3_2 = 0;
  atom_1_r1_2 = 0;
  atom_1_r1_1 = 0;
  atom_1_r1_0 = 0;
  atom_1_r3_1 = 0;
  atom_1_r3_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v5 = vars[0];
  int v6 = (v5 == 3);
  int v7 = atom_1_r3_2;
  int v8 = atom_1_r1_2;
  int v9 = atom_1_r1_1;
  int v10 = atom_1_r1_0;
  int v11_disj = v9 | v10;
  int v12_disj = v8 | v11_disj;
  int v13_conj = v7 & v12_disj;
  int v14 = atom_1_r3_1;
  int v15 = atom_1_r1_1;
  int v16 = atom_1_r1_0;
  int v17_disj = v15 | v16;
  int v18_conj = v14 & v17_disj;
  int v19 = atom_1_r3_0;
  int v20 = atom_1_r1_0;
  int v21_conj = v19 & v20;
  int v22_disj = v18_conj | v21_conj;
  int v23_disj = v13_conj | v22_disj;
  int v24_conj = v6 & v23_disj;
  int v25 = vars[0];
  int v26 = (v25 == 2);
  int v27 = atom_1_r3_1;
  int v28 = atom_1_r1_1;
  int v29 = atom_1_r1_0;
  int v30_disj = v28 | v29;
  int v31_conj = v27 & v30_disj;
  int v32 = atom_1_r3_0;
  int v33 = atom_1_r1_0;
  int v34_conj = v32 & v33;
  int v35_disj = v31_conj | v34_conj;
  int v36_conj = v26 & v35_disj;
  int v37_disj = v24_conj | v36_conj;
  if (v37_disj == 0) assert(0);
  return 0;
}
