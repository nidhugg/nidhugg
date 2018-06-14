// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/m3d2.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[4]; 
volatile int atom_1_r2_2; 
volatile int atom_2_r2_1; 
volatile int atom_2_r1_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  vars[1] = 2;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r2 = vars[1];
  int v3_r9 = v2_r2 ^ v2_r2;
  vars[3+v3_r9] = 1;
  int v5_r3 = vars[3];
  vars[2] = v5_r3;
  int v17 = (v2_r2 == 2);
  atom_1_r2_2 = v17;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v7_r2 = vars[2];
  int v8_r9 = v7_r2 ^ v7_r2;
  int v11_r1 = vars[0+v8_r9];
  int v18 = (v7_r2 == 1);
  atom_2_r2_1 = v18;
  int v19 = (v11_r1 == 0);
  atom_2_r1_0 = v19;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[3] = 0;
  vars[0] = 0;
  vars[1] = 0;
  vars[2] = 0;
  atom_1_r2_2 = 0;
  atom_2_r2_1 = 0;
  atom_2_r1_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v12 = atom_1_r2_2;
  int v13 = atom_2_r2_1;
  int v14 = atom_2_r1_0;
  int v15_conj = v13 & v14;
  int v16_conj = v12 & v15_conj;
  if (v16_conj == 1) assert(0);
  return 0;
}
