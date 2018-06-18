// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/iriwv6.litmus

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
  int v4_r2 = vars[1];
  int v40 = (v2_r1 == 0);
  atom_0_r1_0 = v40;
  int v41 = (v4_r2 == 0);
  atom_0_r2_0 = v41;
  int v44 = (v4_r2 == 2);
  atom_0_r2_2 = v44;
  int v45 = (v2_r1 == 1);
  atom_0_r1_1 = v45;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v6_r2 = vars[1];
  int v8_r1 = vars[0];
  int v42 = (v8_r1 == 0);
  atom_1_r1_0 = v42;
  int v43 = (v6_r2 == 2);
  atom_1_r2_2 = v43;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[0] = 1;
  vars[1] = 2;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[0] = 0;
  vars[1] = 0;
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

  int v9 = atom_0_r1_0;
  int v10 = atom_0_r2_0;
  int v11 = atom_1_r1_0;
  int v12 = atom_1_r2_2;
  int v13_conj = v11 & v12;
  int v14_conj = v10 & v13_conj;
  int v15_conj = v9 & v14_conj;
  int v16 = atom_0_r1_0;
  int v17 = atom_0_r2_2;
  int v18 = atom_1_r1_0;
  int v19 = atom_1_r2_2;
  int v20_conj = v18 & v19;
  int v21_conj = v17 & v20_conj;
  int v22_conj = v16 & v21_conj;
  int v23 = atom_0_r1_1;
  int v24 = atom_0_r2_0;
  int v25 = atom_1_r1_0;
  int v26 = atom_1_r2_2;
  int v27_conj = v25 & v26;
  int v28_conj = v24 & v27_conj;
  int v29_conj = v23 & v28_conj;
  int v30 = atom_0_r1_1;
  int v31 = atom_0_r2_2;
  int v32 = atom_1_r1_0;
  int v33 = atom_1_r2_2;
  int v34_conj = v32 & v33;
  int v35_conj = v31 & v34_conj;
  int v36_conj = v30 & v35_conj;
  int v37_disj = v29_conj | v36_conj;
  int v38_disj = v22_conj | v37_disj;
  int v39_disj = v15_conj | v38_disj;
  if (v39_disj == 1) assert(0);
  return 0;
}
