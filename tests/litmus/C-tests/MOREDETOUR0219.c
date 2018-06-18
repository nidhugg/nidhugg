// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MOREDETOUR0219.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r1_3; 
volatile int atom_1_r1_1; 
volatile int atom_1_r4_0; 
volatile int atom_1_r6_1; 
volatile int atom_3_r1_1; 

void *t0(void *arg){
label_1:;
  int v2_r1 = vars[0];
  int v3_r3 = v2_r1 ^ v2_r1;
  int v4_r3 = v3_r3 + 1;
  vars[1] = v4_r3;
  int v27 = (v2_r1 == 3);
  atom_0_r1_3 = v27;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v6_r1 = vars[1];
  int v7_r3 = v6_r1 ^ v6_r1;
  int v10_r4 = vars[0+v7_r3];
  int v12_r6 = vars[0];
  vars[0] = 3;
  int v28 = (v6_r1 == 1);
  atom_1_r1_1 = v28;
  int v29 = (v10_r4 == 0);
  atom_1_r4_0 = v29;
  int v30 = (v12_r6 == 1);
  atom_1_r6_1 = v30;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[0] = 1;
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v14_r1 = vars[0];
  vars[0] = 2;
  int v31 = (v14_r1 == 1);
  atom_3_r1_1 = v31;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  vars[0] = 0;
  vars[1] = 0;
  atom_0_r1_3 = 0;
  atom_1_r1_1 = 0;
  atom_1_r4_0 = 0;
  atom_1_r6_1 = 0;
  atom_3_r1_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v15 = atom_0_r1_3;
  int v16 = atom_1_r1_1;
  int v17 = atom_1_r4_0;
  int v18 = atom_1_r6_1;
  int v19 = vars[0];
  int v20 = (v19 == 3);
  int v21 = atom_3_r1_1;
  int v22_conj = v20 & v21;
  int v23_conj = v18 & v22_conj;
  int v24_conj = v17 & v23_conj;
  int v25_conj = v16 & v24_conj;
  int v26_conj = v15 & v25_conj;
  if (v26_conj == 1) assert(0);
  return 0;
}
