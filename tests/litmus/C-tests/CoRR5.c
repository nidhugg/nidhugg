// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/CoRR5.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[1]; 
volatile int atom_0_r1_2; 
volatile int atom_0_r2_2; 
volatile int atom_1_r1_2; 
volatile int atom_1_r2_2; 
volatile int atom_0_r1_1; 
volatile int atom_0_r2_1; 
volatile int atom_1_r2_1; 
volatile int atom_1_r1_1; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  int v2_r1 = vars[0];
  int v4_r2 = vars[0];
  int v36 = (v2_r1 == 2);
  atom_0_r1_2 = v36;
  int v37 = (v4_r2 == 2);
  atom_0_r2_2 = v37;
  int v40 = (v2_r1 == 1);
  atom_0_r1_1 = v40;
  int v41 = (v4_r2 == 1);
  atom_0_r2_1 = v41;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[0] = 2;
  int v6_r1 = vars[0];
  int v8_r2 = vars[0];
  int v38 = (v6_r1 == 2);
  atom_1_r1_2 = v38;
  int v39 = (v8_r2 == 2);
  atom_1_r2_2 = v39;
  int v42 = (v8_r2 == 1);
  atom_1_r2_1 = v42;
  int v43 = (v6_r1 == 1);
  atom_1_r1_1 = v43;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 0;
  atom_0_r1_2 = 0;
  atom_0_r2_2 = 0;
  atom_1_r1_2 = 0;
  atom_1_r2_2 = 0;
  atom_0_r1_1 = 0;
  atom_0_r2_1 = 0;
  atom_1_r2_1 = 0;
  atom_1_r1_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v9 = atom_0_r1_2;
  int v10 = atom_0_r2_2;
  int v11 = atom_1_r1_2;
  int v12 = atom_1_r2_2;
  int v13_conj = v11 & v12;
  int v14_conj = v10 & v13_conj;
  int v15_conj = v9 & v14_conj;
  int v16 = atom_0_r1_1;
  int v17 = atom_0_r2_2;
  int v18 = atom_1_r1_2;
  int v19 = atom_1_r2_2;
  int v20_conj = v18 & v19;
  int v21_conj = v17 & v20_conj;
  int v22 = atom_0_r2_1;
  int v23 = atom_1_r1_2;
  int v24 = atom_1_r2_2;
  int v25 = atom_1_r2_1;
  int v26_disj = v24 | v25;
  int v27_conj = v23 & v26_disj;
  int v28 = atom_1_r1_1;
  int v29 = atom_1_r2_1;
  int v30_conj = v28 & v29;
  int v31_disj = v27_conj | v30_conj;
  int v32_conj = v22 & v31_disj;
  int v33_disj = v21_conj | v32_conj;
  int v34_conj = v16 & v33_disj;
  int v35_disj = v15_conj | v34_conj;
  if (v35_disj == 0) assert(0);
  return 0;
}
