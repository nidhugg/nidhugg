// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/m5dl.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_1_r1_2; 
volatile int atom_1_r2_0; 
volatile int atom_1_r2_2; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[1];
  int v3_r9 = v2_r1 ^ v2_r1;
  int v6_r2 = vars[0+v3_r9];
  int v36 = (v2_r1 == 2);
  atom_1_r1_2 = v36;
  int v37 = (v6_r2 == 0);
  atom_1_r2_0 = v37;
  int v38 = (v6_r2 == 2);
  atom_1_r2_2 = v38;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[0] = 2;
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

  vars[0] = 0;
  vars[1] = 0;
  atom_1_r1_2 = 0;
  atom_1_r2_0 = 0;
  atom_1_r2_2 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v7 = atom_1_r1_2;
  int v8 = atom_1_r2_0;
  int v9 = vars[0];
  int v10 = (v9 == 1);
  int v11 = vars[1];
  int v12 = (v11 == 2);
  int v13_conj = v10 & v12;
  int v14_conj = v8 & v13_conj;
  int v15_conj = v7 & v14_conj;
  int v16 = atom_1_r1_2;
  int v17 = atom_1_r2_0;
  int v18 = vars[0];
  int v19 = (v18 == 2);
  int v20 = vars[1];
  int v21 = (v20 == 2);
  int v22_conj = v19 & v21;
  int v23_conj = v17 & v22_conj;
  int v24_conj = v16 & v23_conj;
  int v25 = atom_1_r1_2;
  int v26 = atom_1_r2_2;
  int v27 = vars[0];
  int v28 = (v27 == 1);
  int v29 = vars[1];
  int v30 = (v29 == 2);
  int v31_conj = v28 & v30;
  int v32_conj = v26 & v31_conj;
  int v33_conj = v25 & v32_conj;
  int v34_disj = v24_conj | v33_conj;
  int v35_disj = v15_conj | v34_disj;
  if (v35_disj == 1) assert(0);
  return 0;
}
