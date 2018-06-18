// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/susv1.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_1_r2_2; 
volatile int atom_1_r3_0; 
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
  int v6_r3 = vars[2+v3_r9];
  int v14 = (v2_r2 == 2);
  atom_1_r2_2 = v14;
  int v15 = (v6_r3 == 0);
  atom_1_r3_0 = v15;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[2] = 3;

  int v8_r1 = vars[0];
  int v16 = (v8_r1 == 0);
  atom_2_r1_0 = v16;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[0] = 0;
  vars[2] = 0;
  vars[1] = 0;
  atom_1_r2_2 = 0;
  atom_1_r3_0 = 0;
  atom_2_r1_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v9 = atom_1_r2_2;
  int v10 = atom_1_r3_0;
  int v11 = atom_2_r1_0;
  int v12_conj = v10 & v11;
  int v13_conj = v9 & v12_conj;
  if (v13_conj == 1) assert(0);
  return 0;
}
