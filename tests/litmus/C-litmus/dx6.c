// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/dx6.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r2_2; 
volatile int atom_1_r2_1; 

void *t0(void *arg){
label_1:;
  int v2_r2 = vars[1];

  vars[0] = 1;
  int v12 = (v2_r2 == 2);
  atom_0_r2_2 = v12;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v4_r2 = vars[0];
  int v5_r9 = v4_r2 ^ v4_r2;
  vars[1+v5_r9] = 1;
  int v13 = (v4_r2 == 1);
  atom_1_r2_1 = v13;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[1] = 2;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[0] = 0;
  vars[1] = 0;
  atom_0_r2_2 = 0;
  atom_1_r2_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v6 = atom_0_r2_2;
  int v7 = atom_1_r2_1;
  int v8 = vars[1];
  int v9 = (v8 == 2);
  int v10_conj = v7 & v9;
  int v11_conj = v6 & v10_conj;
  if (v11_conj == 1) assert(0);
  return 0;
}
