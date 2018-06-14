// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/iriwv4.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r1_0; 
volatile int atom_0_r2_0; 
volatile int atom_1_r1_0; 
volatile int atom_1_r2_2; 
volatile int atom_0_r2_2; 
volatile int atom_0_r1_1; 

void *t0(void *arg){
label_1:;
  int v2_r1 = vars[0];
  int v3_r3 = v2_r1 ^ v2_r1;
  int v6_r2 = vars[1+v3_r3];
  int v47 = (v2_r1 == 0);
  atom_0_r1_0 = v47;
  int v48 = (v6_r2 == 0);
  atom_0_r2_0 = v48;
  int v51 = (v6_r2 == 2);
  atom_0_r2_2 = v51;
  int v52 = (v2_r1 == 1);
  atom_0_r1_1 = v52;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v8_r2 = vars[1];
  int v9_r3 = v8_r2 ^ v8_r2;
  int v12_r1 = vars[0+v9_r3];
  int v49 = (v12_r1 == 0);
  atom_1_r1_0 = v49;
  int v50 = (v8_r2 == 2);
  atom_1_r2_2 = v50;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[0] = 1;
  int v14_r1 = vars[0];
  int v15_r3 = v14_r1 ^ v14_r1;
  vars[1+v15_r3] = 2;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[1] = 0;
  vars[0] = 0;
  atom_0_r1_0 = 0;
  atom_0_r2_0 = 0;
  atom_1_r1_0 = 0;
  atom_1_r2_2 = 0;
  atom_0_r2_2 = 0;
  atom_0_r1_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v16 = atom_0_r1_0;
  int v17 = atom_0_r2_0;
  int v18 = atom_1_r1_0;
  int v19 = atom_1_r2_2;
  int v20_conj = v18 & v19;
  int v21_conj = v17 & v20_conj;
  int v22_conj = v16 & v21_conj;
  int v23 = atom_0_r1_0;
  int v24 = atom_0_r2_2;
  int v25 = atom_1_r1_0;
  int v26 = atom_1_r2_2;
  int v27_conj = v25 & v26;
  int v28_conj = v24 & v27_conj;
  int v29_conj = v23 & v28_conj;
  int v30 = atom_0_r1_1;
  int v31 = atom_0_r2_0;
  int v32 = atom_1_r1_0;
  int v33 = atom_1_r2_2;
  int v34_conj = v32 & v33;
  int v35_conj = v31 & v34_conj;
  int v36_conj = v30 & v35_conj;
  int v37 = atom_0_r1_1;
  int v38 = atom_0_r2_2;
  int v39 = atom_1_r1_0;
  int v40 = atom_1_r2_2;
  int v41_conj = v39 & v40;
  int v42_conj = v38 & v41_conj;
  int v43_conj = v37 & v42_conj;
  int v44_disj = v36_conj | v43_conj;
  int v45_disj = v29_conj | v44_disj;
  int v46_disj = v22_conj | v45_disj;
  if (v46_disj == 1) assert(0);
  return 0;
}
