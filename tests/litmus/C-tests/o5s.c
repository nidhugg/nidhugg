// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/o5s.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_1_r1_2; 
volatile int atom_1_r2_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[1];

  int v4_r2 = vars[0];
  int v11 = (v2_r1 == 2);
  atom_1_r1_2 = v11;
  int v12 = (v4_r2 == 0);
  atom_1_r2_0 = v12;
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

  vars[1] = 0;
  vars[0] = 0;
  atom_1_r1_2 = 0;
  atom_1_r2_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v5 = atom_1_r1_2;
  int v6 = atom_1_r2_0;
  int v7 = vars[1];
  int v8 = (v7 == 2);
  int v9_conj = v6 & v8;
  int v10_conj = v5 & v9_conj;
  if (v10_conj == 1) assert(0);
  return 0;
}
