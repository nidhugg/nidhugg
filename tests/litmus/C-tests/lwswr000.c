// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/lwswr000.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r3_1; 
volatile int atom_1_r3_2; 
volatile int atom_1_r5_0; 
volatile int atom_2_r3_1; 
volatile int atom_3_r3_2; 
volatile int atom_3_r5_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  int v2_r3 = vars[0];
  int v34 = (v2_r3 == 1);
  atom_0_r3_1 = v34;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[0] = 2;

  int v4_r3 = vars[0];
  int v5_r4 = v4_r3 ^ v4_r3;
  int v8_r5 = vars[1+v5_r4];
  int v35 = (v4_r3 == 2);
  atom_1_r3_2 = v35;
  int v36 = (v8_r5 == 0);
  atom_1_r5_0 = v36;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[1] = 1;

  int v10_r3 = vars[1];
  int v37 = (v10_r3 == 1);
  atom_2_r3_1 = v37;
  return NULL;
}

void *t3(void *arg){
label_4:;
  vars[1] = 2;

  int v12_r3 = vars[1];
  int v13_r4 = v12_r3 ^ v12_r3;
  int v16_r5 = vars[0+v13_r4];
  int v38 = (v12_r3 == 2);
  atom_3_r3_2 = v38;
  int v39 = (v16_r5 == 0);
  atom_3_r5_0 = v39;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  vars[1] = 0;
  vars[0] = 0;
  atom_0_r3_1 = 0;
  atom_1_r3_2 = 0;
  atom_1_r5_0 = 0;
  atom_2_r3_1 = 0;
  atom_3_r3_2 = 0;
  atom_3_r5_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v17 = vars[0];
  int v18 = (v17 == 2);
  int v19 = vars[1];
  int v20 = (v19 == 2);
  int v21 = atom_0_r3_1;
  int v22 = atom_1_r3_2;
  int v23 = atom_1_r5_0;
  int v24 = atom_2_r3_1;
  int v25 = atom_3_r3_2;
  int v26 = atom_3_r5_0;
  int v27_conj = v25 & v26;
  int v28_conj = v24 & v27_conj;
  int v29_conj = v23 & v28_conj;
  int v30_conj = v22 & v29_conj;
  int v31_conj = v21 & v30_conj;
  int v32_conj = v20 & v31_conj;
  int v33_conj = v18 & v32_conj;
  if (v33_conj == 1) assert(0);
  return 0;
}
