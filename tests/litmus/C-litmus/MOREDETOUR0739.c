// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MOREDETOUR0739.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_1_r1_1; 
volatile int atom_2_r1_1; 

void *t0(void *arg){
label_1:;
  vars[0] = 3;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[1];
  vars[1] = 3;
  vars[0] = 1;
  vars[0] = 2;
  int v14 = (v2_r1 == 1);
  atom_1_r1_1 = v14;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v4_r1 = vars[1];
  vars[1] = 2;
  int v15 = (v4_r1 == 1);
  atom_2_r1_1 = v15;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[1] = 0;
  vars[0] = 0;
  atom_1_r1_1 = 0;
  atom_2_r1_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v5 = vars[0];
  int v6 = (v5 == 3);
  int v7 = atom_1_r1_1;
  int v8 = vars[1];
  int v9 = (v8 == 3);
  int v10 = atom_2_r1_1;
  int v11_conj = v9 & v10;
  int v12_conj = v7 & v11_conj;
  int v13_conj = v6 & v12_conj;
  if (v13_conj == 1) assert(0);
  return 0;
}
