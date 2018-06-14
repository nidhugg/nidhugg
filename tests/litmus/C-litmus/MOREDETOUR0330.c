// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MOREDETOUR0330.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_0_r6_0; 
volatile int atom_2_r1_2; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;
  int v2_r3 = vars[1];
  int v3_r5 = v2_r3 ^ v2_r3;
  int v6_r6 = vars[2+v3_r5];
  vars[2] = 2;
  int v20 = (v6_r6 == 0);
  atom_0_r6_0 = v20;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[2] = 1;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v8_r1 = vars[2];
  int v9_r3 = v8_r1 ^ v8_r1;
  int v10_r3 = v9_r3 + 1;
  vars[0] = v10_r3;
  int v21 = (v8_r1 == 2);
  atom_2_r1_2 = v21;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[1] = 0;
  vars[2] = 0;
  vars[0] = 0;
  atom_0_r6_0 = 0;
  atom_2_r1_2 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v11 = atom_0_r6_0;
  int v12 = vars[2];
  int v13 = (v12 == 2);
  int v14 = vars[0];
  int v15 = (v14 == 2);
  int v16 = atom_2_r1_2;
  int v17_conj = v15 & v16;
  int v18_conj = v13 & v17_conj;
  int v19_conj = v11 & v18_conj;
  if (v19_conj == 1) assert(0);
  return 0;
}
