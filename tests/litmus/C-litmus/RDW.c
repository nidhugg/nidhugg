// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/RDW.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_1_r1_2; 
volatile int atom_1_r2_0; 
volatile int atom_1_r3_0; 
volatile int atom_1_r4_1; 

void *t0(void *arg){
label_1:;
  vars[2] = 1;

  vars[1] = 2;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[1];
  int v3_r9 = v2_r1 ^ v2_r1;
  int v6_r3 = vars[0+v3_r9];
  int v8_r4 = vars[0];
  int v9_r10 = v8_r4 ^ v8_r4;
  int v12_r2 = vars[2+v9_r10];
  int v20 = (v2_r1 == 2);
  atom_1_r1_2 = v20;
  int v21 = (v12_r2 == 0);
  atom_1_r2_0 = v21;
  int v22 = (v6_r3 == 0);
  atom_1_r3_0 = v22;
  int v23 = (v8_r4 == 1);
  atom_1_r4_1 = v23;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[0] = 1;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[1] = 0;
  vars[0] = 0;
  vars[2] = 0;
  atom_1_r1_2 = 0;
  atom_1_r2_0 = 0;
  atom_1_r3_0 = 0;
  atom_1_r4_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v13 = atom_1_r1_2;
  int v14 = atom_1_r2_0;
  int v15 = atom_1_r3_0;
  int v16 = atom_1_r4_1;
  int v17_conj = v15 & v16;
  int v18_conj = v14 & v17_conj;
  int v19_conj = v13 & v18_conj;
  if (v19_conj == 1) assert(0);
  return 0;
}
