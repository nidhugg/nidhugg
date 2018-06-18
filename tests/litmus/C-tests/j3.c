// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/j3.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r2_2; 
volatile int atom_0_r4_2; 
volatile int atom_1_r1_1; 
volatile int atom_2_r1_3; 
volatile int atom_2_r3_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  int v2_r2 = vars[0];
  int v3_r3 = v2_r2 ^ v2_r2;
  int v6_r4 = vars[1+v3_r3];
  vars[1] = 3;
  int v30 = (v2_r2 == 2);
  atom_0_r2_2 = v30;
  int v31 = (v6_r4 == 2);
  atom_0_r4_2 = v31;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v8_r1 = vars[0];
  vars[1] = 2;

  vars[0] = 2;
  int v32 = (v8_r1 == 1);
  atom_1_r1_1 = v32;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v10_r1 = vars[1];
  int v11_r2 = v10_r1 ^ v10_r1;
  int v14_r3 = vars[0+v11_r2];
  int v33 = (v10_r1 == 3);
  atom_2_r1_3 = v33;
  int v34 = (v14_r3 == 0);
  atom_2_r3_0 = v34;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[0] = 0;
  vars[1] = 0;
  atom_0_r2_2 = 0;
  atom_0_r4_2 = 0;
  atom_1_r1_1 = 0;
  atom_2_r1_3 = 0;
  atom_2_r3_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v15 = atom_0_r2_2;
  int v16 = atom_0_r4_2;
  int v17 = atom_1_r1_1;
  int v18 = atom_2_r1_3;
  int v19 = atom_2_r3_0;
  int v20 = vars[0];
  int v21 = (v20 == 2);
  int v22 = vars[1];
  int v23 = (v22 == 3);
  int v24_conj = v21 & v23;
  int v25_conj = v19 & v24_conj;
  int v26_conj = v18 & v25_conj;
  int v27_conj = v17 & v26_conj;
  int v28_conj = v16 & v27_conj;
  int v29_conj = v15 & v28_conj;
  if (v29_conj == 1) assert(0);
  return 0;
}
