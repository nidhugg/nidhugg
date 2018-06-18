// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/podrwposwr024.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[6]; 
volatile int atom_1_r1_1; 
volatile int atom_1_r7_0; 
volatile int atom_3_r1_1; 
volatile int atom_3_r7_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[1];
  vars[2] = 1;
  int v4_r5 = vars[2];
  int v5_r6 = v4_r5 ^ v4_r5;
  int v8_r7 = vars[3+v5_r6];
  int v24 = (v2_r1 == 1);
  atom_1_r1_1 = v24;
  int v25 = (v8_r7 == 0);
  atom_1_r7_0 = v25;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[3] = 1;

  vars[4] = 1;
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v10_r1 = vars[4];
  vars[5] = 1;
  int v12_r5 = vars[5];
  int v13_r6 = v12_r5 ^ v12_r5;
  int v16_r7 = vars[0+v13_r6];
  int v26 = (v10_r1 == 1);
  atom_3_r1_1 = v26;
  int v27 = (v16_r7 == 0);
  atom_3_r7_0 = v27;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  vars[3] = 0;
  vars[5] = 0;
  vars[1] = 0;
  vars[0] = 0;
  vars[2] = 0;
  vars[4] = 0;
  atom_1_r1_1 = 0;
  atom_1_r7_0 = 0;
  atom_3_r1_1 = 0;
  atom_3_r7_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v17 = atom_1_r1_1;
  int v18 = atom_1_r7_0;
  int v19 = atom_3_r1_1;
  int v20 = atom_3_r7_0;
  int v21_conj = v19 & v20;
  int v22_conj = v18 & v21_conj;
  int v23_conj = v17 & v22_conj;
  if (v23_conj == 1) assert(0);
  return 0;
}
