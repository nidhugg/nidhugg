// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/RWC+addr-fri-rfi-addr+sync.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_1_r1_1; 
volatile int atom_1_r4_0; 
volatile int atom_1_r7_1; 
volatile int atom_1_r9_0; 
volatile int atom_2_r3_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[0];
  int v3_r3 = v2_r1 ^ v2_r1;
  int v6_r4 = vars[1+v3_r3];
  vars[1] = 1;
  int v8_r7 = vars[1];
  int v9_r8 = v8_r7 ^ v8_r7;
  int v12_r9 = vars[2+v9_r8];
  int v24 = (v2_r1 == 1);
  atom_1_r1_1 = v24;
  int v25 = (v6_r4 == 0);
  atom_1_r4_0 = v25;
  int v26 = (v8_r7 == 1);
  atom_1_r7_1 = v26;
  int v27 = (v12_r9 == 0);
  atom_1_r9_0 = v27;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[2] = 1;

  int v14_r3 = vars[0];
  int v28 = (v14_r3 == 0);
  atom_2_r3_0 = v28;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[1] = 0;
  vars[0] = 0;
  vars[2] = 0;
  atom_1_r1_1 = 0;
  atom_1_r4_0 = 0;
  atom_1_r7_1 = 0;
  atom_1_r9_0 = 0;
  atom_2_r3_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v15 = atom_1_r1_1;
  int v16 = atom_1_r4_0;
  int v17 = atom_1_r7_1;
  int v18 = atom_1_r9_0;
  int v19 = atom_2_r3_0;
  int v20_conj = v18 & v19;
  int v21_conj = v17 & v20_conj;
  int v22_conj = v16 & v21_conj;
  int v23_conj = v15 & v22_conj;
  if (v23_conj == 1) assert(0);
  return 0;
}