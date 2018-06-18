// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/podrwposwr051.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[4]; 
volatile int atom_2_r1_1; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 2;

  vars[2] = 1;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v2_r1 = vars[2];
  vars[3] = 1;
  int v4_r5 = vars[3];
  int v5_r6 = v4_r5 ^ v4_r5;
  vars[0+v5_r6] = 1;
  int v13 = (v2_r1 == 1);
  atom_2_r1_1 = v13;
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
  atom_2_r1_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v6 = vars[0];
  int v7 = (v6 == 2);
  int v8 = vars[1];
  int v9 = (v8 == 2);
  int v10 = atom_2_r1_1;
  int v11_conj = v9 & v10;
  int v12_conj = v7 & v11_conj;
  if (v12_conj == 1) assert(0);
  return 0;
}
