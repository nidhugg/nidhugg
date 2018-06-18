// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MOREDETOUR0722.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_1_r1_1; 
volatile int atom_1_r4_3; 
volatile int atom_2_r1_1; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[1];
  vars[1] = 3;
  int v4_r4 = vars[1];
  int v5_r5 = v4_r4 ^ v4_r4;
  int v6_r5 = v5_r5 + 1;
  vars[0] = v6_r5;
  int v20 = (v2_r1 == 1);
  atom_1_r1_1 = v20;
  int v21 = (v4_r4 == 3);
  atom_1_r4_3 = v21;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v8_r1 = vars[1];
  vars[1] = 2;
  int v22 = (v8_r1 == 1);
  atom_2_r1_1 = v22;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[0] = 0;
  vars[1] = 0;
  atom_1_r1_1 = 0;
  atom_1_r4_3 = 0;
  atom_2_r1_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v9 = vars[0];
  int v10 = (v9 == 2);
  int v11 = atom_1_r1_1;
  int v12 = atom_1_r4_3;
  int v13 = vars[1];
  int v14 = (v13 == 3);
  int v15 = atom_2_r1_1;
  int v16_conj = v14 & v15;
  int v17_conj = v12 & v16_conj;
  int v18_conj = v11 & v17_conj;
  int v19_conj = v10 & v18_conj;
  if (v19_conj == 1) assert(0);
  return 0;
}
