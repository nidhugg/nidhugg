// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/podrwposwr002.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[5]; 
volatile int atom_1_r1_1; 
volatile int atom_1_r5_1; 
volatile int atom_3_r1_1; 
volatile int atom_3_r8_1; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[1];
  vars[2] = 1;
  int v4_r5 = vars[2];
  int v26 = (v2_r1 == 1);
  atom_1_r1_1 = v26;
  int v27 = (v4_r5 == 1);
  atom_1_r5_1 = v27;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[2] = 2;

  vars[3] = 1;
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v6_r1 = vars[3];
  int v7_r3 = v6_r1 ^ v6_r1;
  int v10_r4 = vars[4+v7_r3];
  vars[0] = 1;
  int v12_r8 = vars[0];
  int v28 = (v6_r1 == 1);
  atom_3_r1_1 = v28;
  int v29 = (v12_r8 == 1);
  atom_3_r8_1 = v29;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  vars[4] = 0;
  vars[0] = 0;
  vars[3] = 0;
  vars[2] = 0;
  vars[1] = 0;
  atom_1_r1_1 = 0;
  atom_1_r5_1 = 0;
  atom_3_r1_1 = 0;
  atom_3_r8_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v13 = vars[0];
  int v14 = (v13 == 2);
  int v15 = vars[2];
  int v16 = (v15 == 2);
  int v17 = atom_1_r1_1;
  int v18 = atom_1_r5_1;
  int v19 = atom_3_r1_1;
  int v20 = atom_3_r8_1;
  int v21_conj = v19 & v20;
  int v22_conj = v18 & v21_conj;
  int v23_conj = v17 & v22_conj;
  int v24_conj = v16 & v23_conj;
  int v25_conj = v14 & v24_conj;
  if (v25_conj == 1) assert(0);
  return 0;
}
