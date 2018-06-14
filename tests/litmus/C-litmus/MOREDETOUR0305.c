// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MOREDETOUR0305.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_1_r4_3; 
volatile int atom_1_r1_1; 
volatile int atom_2_r1_4; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;
  vars[1] = 1;
  vars[1] = 3;
  vars[1] = 4;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[1];
  vars[1] = 2;

  int v4_r4 = vars[1];
  int v17 = (v4_r4 == 3);
  atom_1_r4_3 = v17;
  int v18 = (v2_r1 == 1);
  atom_1_r1_1 = v18;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v6_r1 = vars[1];
  int v7_r3 = v6_r1 ^ v6_r1;
  int v8_r3 = v7_r3 + 1;
  vars[0] = v8_r3;
  int v19 = (v6_r1 == 4);
  atom_2_r1_4 = v19;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[0] = 0;
  vars[1] = 0;
  atom_1_r4_3 = 0;
  atom_1_r1_1 = 0;
  atom_2_r1_4 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v9 = atom_1_r4_3;
  int v10 = atom_1_r1_1;
  int v11 = vars[0];
  int v12 = (v11 == 2);
  int v13 = atom_2_r1_4;
  int v14_conj = v12 & v13;
  int v15_conj = v10 & v14_conj;
  int v16_conj = v9 & v15_conj;
  if (v16_conj == 1) assert(0);
  return 0;
}