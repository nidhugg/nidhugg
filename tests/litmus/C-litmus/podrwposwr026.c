// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/podrwposwr026.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[4]; 
volatile int atom_1_r1_1; 
volatile int atom_1_r4_0; 
volatile int atom_3_r1_1; 
volatile int atom_3_r5_1; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[1];
  int v3_r3 = v2_r1 ^ v2_r1;
  int v6_r4 = vars[2+v3_r3];
  int v21 = (v2_r1 == 1);
  atom_1_r1_1 = v21;
  int v22 = (v6_r4 == 0);
  atom_1_r4_0 = v22;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[2] = 1;

  vars[3] = 1;
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v8_r1 = vars[3];
  vars[0] = 1;
  int v10_r5 = vars[0];
  int v23 = (v8_r1 == 1);
  atom_3_r1_1 = v23;
  int v24 = (v10_r5 == 1);
  atom_3_r5_1 = v24;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  vars[3] = 0;
  vars[0] = 0;
  vars[1] = 0;
  vars[2] = 0;
  atom_1_r1_1 = 0;
  atom_1_r4_0 = 0;
  atom_3_r1_1 = 0;
  atom_3_r5_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v11 = vars[0];
  int v12 = (v11 == 2);
  int v13 = atom_1_r1_1;
  int v14 = atom_1_r4_0;
  int v15 = atom_3_r1_1;
  int v16 = atom_3_r5_1;
  int v17_conj = v15 & v16;
  int v18_conj = v14 & v17_conj;
  int v19_conj = v13 & v18_conj;
  int v20_conj = v12 & v19_conj;
  if (v20_conj == 1) assert(0);
  return 0;
}