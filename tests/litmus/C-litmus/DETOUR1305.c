// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/DETOUR1305.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_1_r3_0; 
volatile int atom_1_r5_1; 

void *t0(void *arg){
label_1:;
  vars[0] = 3;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 2;
  int v2_r3 = vars[0];
  int v4_r5 = vars[0];
  vars[0] = 2;
  int v14 = (v2_r3 == 0);
  atom_1_r3_0 = v14;
  int v15 = (v4_r5 == 1);
  atom_1_r5_1 = v15;
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
  atom_1_r3_0 = 0;
  atom_1_r5_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v5 = vars[1];
  int v6 = (v5 == 2);
  int v7 = vars[0];
  int v8 = (v7 == 3);
  int v9 = atom_1_r3_0;
  int v10 = atom_1_r5_1;
  int v11_conj = v9 & v10;
  int v12_conj = v8 & v11_conj;
  int v13_conj = v6 & v12_conj;
  if (v13_conj == 1) assert(0);
  return 0;
}
