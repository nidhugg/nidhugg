// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MOREDETOUR0891.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r5_2; 
volatile int atom_2_r1_2; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;

  vars[1] = 1;

  int v2_r5 = vars[1];
  int v16 = (v2_r5 == 2);
  atom_0_r5_2 = v16;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 2;
  vars[1] = 4;
  int v4_r4 = vars[0];
  vars[0] = 1;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v6_r1 = vars[1];
  vars[1] = 3;
  int v17 = (v6_r1 == 2);
  atom_2_r1_2 = v17;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[0] = 0;
  vars[1] = 0;
  atom_0_r5_2 = 0;
  atom_2_r1_2 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v7 = atom_0_r5_2;
  int v8 = vars[0];
  int v9 = (v8 == 2);
  int v10 = vars[1];
  int v11 = (v10 == 4);
  int v12 = atom_2_r1_2;
  int v13_conj = v11 & v12;
  int v14_conj = v9 & v13_conj;
  int v15_conj = v7 & v14_conj;
  if (v15_conj == 1) assert(0);
  return 0;
}
