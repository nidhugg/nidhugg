// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MOREDETOUR0634.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_1_r1_1; 
volatile int atom_2_r3_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  vars[0] = 3;
  int v2_r4 = vars[1];
  int v3_r6 = v2_r4 ^ v2_r4;
  int v4_r6 = v3_r6 + 1;
  vars[2] = v4_r6;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v6_r1 = vars[0];
  vars[0] = 2;
  int v18 = (v6_r1 == 1);
  atom_1_r1_1 = v18;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[2] = 2;

  int v8_r3 = vars[0];
  int v19 = (v8_r3 == 0);
  atom_2_r3_0 = v19;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[2] = 0;
  vars[1] = 0;
  vars[0] = 0;
  atom_1_r1_1 = 0;
  atom_2_r3_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v9 = vars[2];
  int v10 = (v9 == 2);
  int v11 = vars[0];
  int v12 = (v11 == 3);
  int v13 = atom_1_r1_1;
  int v14 = atom_2_r3_0;
  int v15_conj = v13 & v14;
  int v16_conj = v12 & v15_conj;
  int v17_conj = v10 & v16_conj;
  if (v17_conj == 1) assert(0);
  return 0;
}
