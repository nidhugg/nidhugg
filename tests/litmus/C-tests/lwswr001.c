// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/lwswr001.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r3_1; 
volatile int atom_0_r5_0; 
volatile int atom_1_r3_1; 
volatile int atom_2_r3_2; 
volatile int atom_2_r5_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  int v2_r3 = vars[0];
  int v3_r4 = v2_r3 ^ v2_r3;
  int v6_r5 = vars[1+v3_r4];
  int v27 = (v2_r3 == 1);
  atom_0_r3_1 = v27;
  int v28 = (v6_r5 == 0);
  atom_0_r5_0 = v28;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 1;

  int v8_r3 = vars[1];
  int v29 = (v8_r3 == 1);
  atom_1_r3_1 = v29;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[1] = 2;

  int v10_r3 = vars[1];
  int v11_r4 = v10_r3 ^ v10_r3;
  int v14_r5 = vars[0+v11_r4];
  int v30 = (v10_r3 == 2);
  atom_2_r3_2 = v30;
  int v31 = (v14_r5 == 0);
  atom_2_r5_0 = v31;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[0] = 0;
  vars[1] = 0;
  atom_0_r3_1 = 0;
  atom_0_r5_0 = 0;
  atom_1_r3_1 = 0;
  atom_2_r3_2 = 0;
  atom_2_r5_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v15 = vars[1];
  int v16 = (v15 == 2);
  int v17 = atom_0_r3_1;
  int v18 = atom_0_r5_0;
  int v19 = atom_1_r3_1;
  int v20 = atom_2_r3_2;
  int v21 = atom_2_r5_0;
  int v22_conj = v20 & v21;
  int v23_conj = v19 & v22_conj;
  int v24_conj = v18 & v23_conj;
  int v25_conj = v17 & v24_conj;
  int v26_conj = v16 & v25_conj;
  if (v26_conj == 1) assert(0);
  return 0;
}
