// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/iriwv3.litmus

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
  int v44 = (v2_r1 == 0);
  atom_0_r1_0 = v44;
  int v45 = (v6_r2 == 0);
  atom_0_r2_0 = v45;
  int v48 = (v6_r2 == 2);
  atom_0_r2_2 = v48;
  int v49 = (v2_r1 == 1);
  atom_0_r1_1 = v49;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v8_r2 = vars[1];
  int v9_r3 = v8_r2 ^ v8_r2;
  int v12_r1 = vars[0+v9_r3];
  int v46 = (v12_r1 == 0);
  atom_1_r1_0 = v46;
  int v47 = (v8_r2 == 2);
  atom_1_r2_2 = v47;
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

  int v13 = atom_0_r1_0;
  int v14 = atom_0_r2_0;
  int v15 = atom_1_r1_0;
  int v16 = atom_1_r2_2;
  int v17_conj = v15 & v16;
  int v18_conj = v14 & v17_conj;
  int v19_conj = v13 & v18_conj;
  int v20 = atom_0_r1_0;
  int v21 = atom_0_r2_2;
  int v22 = atom_1_r1_0;
  int v23 = atom_1_r2_2;
  int v24_conj = v22 & v23;
  int v25_conj = v21 & v24_conj;
  int v26_conj = v20 & v25_conj;
  int v27 = atom_0_r1_1;
  int v28 = atom_0_r2_0;
  int v29 = atom_1_r1_0;
  int v30 = atom_1_r2_2;
  int v31_conj = v29 & v30;
  int v32_conj = v28 & v31_conj;
  int v33_conj = v27 & v32_conj;
  int v34 = atom_0_r1_1;
  int v35 = atom_0_r2_2;
  int v36 = atom_1_r1_0;
  int v37 = atom_1_r2_2;
  int v38_conj = v36 & v37;
  int v39_conj = v35 & v38_conj;
  int v40_conj = v34 & v39_conj;
  int v41_disj = v33_conj | v40_conj;
  int v42_disj = v26_conj | v41_disj;
  int v43_disj = v19_conj | v42_disj;
  if (v43_disj == 1) assert(0);
  return 0;
}
