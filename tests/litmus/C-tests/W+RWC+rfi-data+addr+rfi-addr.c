// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/W+RWC+rfi-data+addr+rfi-addr.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_0_r3_1; 
volatile int atom_1_r1_1; 
volatile int atom_1_r4_0; 
volatile int atom_2_r3_1; 
volatile int atom_2_r5_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  int v2_r3 = vars[0];
  int v3_r4 = v2_r3 ^ v2_r3;
  int v4_r4 = v3_r4 + 1;
  vars[1] = v4_r4;
  int v26 = (v2_r3 == 1);
  atom_0_r3_1 = v26;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v6_r1 = vars[1];
  int v7_r3 = v6_r1 ^ v6_r1;
  int v10_r4 = vars[2+v7_r3];
  int v27 = (v6_r1 == 1);
  atom_1_r1_1 = v27;
  int v28 = (v10_r4 == 0);
  atom_1_r4_0 = v28;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[2] = 1;
  int v12_r3 = vars[2];
  int v13_r4 = v12_r3 ^ v12_r3;
  int v16_r5 = vars[0+v13_r4];
  int v29 = (v12_r3 == 1);
  atom_2_r3_1 = v29;
  int v30 = (v16_r5 == 0);
  atom_2_r5_0 = v30;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[2] = 0;
  vars[1] = 0;
  vars[0] = 0;
  atom_0_r3_1 = 0;
  atom_1_r1_1 = 0;
  atom_1_r4_0 = 0;
  atom_2_r3_1 = 0;
  atom_2_r5_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v17 = atom_0_r3_1;
  int v18 = atom_1_r1_1;
  int v19 = atom_1_r4_0;
  int v20 = atom_2_r3_1;
  int v21 = atom_2_r5_0;
  int v22_conj = v20 & v21;
  int v23_conj = v19 & v22_conj;
  int v24_conj = v18 & v23_conj;
  int v25_conj = v17 & v24_conj;
  if (v25_conj == 1) assert(0);
  return 0;
}
