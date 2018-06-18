// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MOREDETOUR0446.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_1_r1_1; 
volatile int atom_1_r6_0; 
volatile int atom_2_r1_1; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[1];
  vars[1] = 3;
  int v4_r4 = vars[1];
  int v5_r5 = v4_r4 ^ v4_r4;
  int v8_r6 = vars[0+v5_r5];
  int v19 = (v2_r1 == 1);
  atom_1_r1_1 = v19;
  int v20 = (v8_r6 == 0);
  atom_1_r6_0 = v20;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v10_r1 = vars[1];
  vars[1] = 2;
  int v21 = (v10_r1 == 1);
  atom_2_r1_1 = v21;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[0] = 0;
  vars[1] = 0;
  atom_1_r1_1 = 0;
  atom_1_r6_0 = 0;
  atom_2_r1_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v11 = atom_1_r1_1;
  int v12 = atom_1_r6_0;
  int v13 = vars[1];
  int v14 = (v13 == 3);
  int v15 = atom_2_r1_1;
  int v16_conj = v14 & v15;
  int v17_conj = v12 & v16_conj;
  int v18_conj = v11 & v17_conj;
  if (v18_conj == 1) assert(0);
  return 0;
}
