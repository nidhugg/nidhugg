// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/bc.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_1_r1_1; 
volatile int atom_1_r2_0; 
volatile int atom_2_r1_1; 
volatile int atom_2_r2_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[1];
  int v3_r7 = v2_r1 ^ v2_r1;
  int v6_r2 = vars[0+v3_r7];
  int v20 = (v2_r1 == 1);
  atom_1_r1_1 = v20;
  int v21 = (v6_r2 == 0);
  atom_1_r2_0 = v21;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v8_r1 = vars[1];
  int v9_r7 = v8_r1 ^ v8_r1;
  int v12_r2 = vars[0+v9_r7];
  int v22 = (v8_r1 == 1);
  atom_2_r1_1 = v22;
  int v23 = (v12_r2 == 0);
  atom_2_r2_0 = v23;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[0] = 0;
  vars[1] = 0;
  atom_1_r1_1 = 0;
  atom_1_r2_0 = 0;
  atom_2_r1_1 = 0;
  atom_2_r2_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v13 = atom_1_r1_1;
  int v14 = atom_1_r2_0;
  int v15_conj = v13 & v14;
  int v16 = atom_2_r1_1;
  int v17 = atom_2_r2_0;
  int v18_conj = v16 & v17;
  int v19_disj = v15_conj | v18_conj;
  if (v19_disj == 1) assert(0);
  return 0;
}
