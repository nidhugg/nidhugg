// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/mas.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r2_0; 
volatile int atom_1_r2_2; 
volatile int atom_2_r1_1; 
volatile int atom_2_r1_2; 
volatile int atom_2_r1_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  int v2_r2 = vars[1];
  int v33 = (v2_r2 == 0);
  atom_0_r2_0 = v33;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v4_r2 = vars[1];

  vars[0] = 2;
  int v34 = (v4_r2 == 2);
  atom_1_r2_2 = v34;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v6_r1 = vars[0];
  int v35 = (v6_r1 == 1);
  atom_2_r1_1 = v35;
  int v36 = (v6_r1 == 2);
  atom_2_r1_2 = v36;
  int v37 = (v6_r1 == 0);
  atom_2_r1_0 = v37;
  return NULL;
}

void *t3(void *arg){
label_4:;
  vars[1] = 2;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  vars[1] = 0;
  vars[0] = 0;
  atom_0_r2_0 = 0;
  atom_1_r2_2 = 0;
  atom_2_r1_1 = 0;
  atom_2_r1_2 = 0;
  atom_2_r1_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v7 = atom_0_r2_0;
  int v8 = atom_1_r2_2;
  int v9 = atom_2_r1_1;
  int v10 = vars[0];
  int v11 = (v10 == 1);
  int v12_conj = v9 & v11;
  int v13_conj = v8 & v12_conj;
  int v14_conj = v7 & v13_conj;
  int v15 = atom_0_r2_0;
  int v16 = atom_1_r2_2;
  int v17 = atom_2_r1_2;
  int v18 = vars[0];
  int v19 = (v18 == 1);
  int v20_conj = v17 & v19;
  int v21_conj = v16 & v20_conj;
  int v22_conj = v15 & v21_conj;
  int v23 = atom_0_r2_0;
  int v24 = atom_1_r2_2;
  int v25 = atom_2_r1_0;
  int v26 = vars[0];
  int v27 = (v26 == 1);
  int v28_conj = v25 & v27;
  int v29_conj = v24 & v28_conj;
  int v30_conj = v23 & v29_conj;
  int v31_disj = v22_conj | v30_conj;
  int v32_disj = v14_conj | v31_disj;
  if (v32_disj == 1) assert(0);
  return 0;
}
