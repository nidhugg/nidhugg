// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/ISA2+lwsync+data+addr-fri-rfi-addr.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[4]; 
volatile int atom_1_r1_1; 
volatile int atom_2_r1_1; 
volatile int atom_2_r4_0; 
volatile int atom_2_r7_1; 
volatile int atom_2_r9_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[1];
  int v3_r3 = v2_r1 ^ v2_r1;
  int v4_r3 = v3_r3 + 1;
  vars[2] = v4_r3;
  int v26 = (v2_r1 == 1);
  atom_1_r1_1 = v26;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v6_r1 = vars[2];
  int v7_r3 = v6_r1 ^ v6_r1;
  int v10_r4 = vars[3+v7_r3];
  vars[3] = 1;
  int v12_r7 = vars[3];
  int v13_r8 = v12_r7 ^ v12_r7;
  int v16_r9 = vars[0+v13_r8];
  int v27 = (v6_r1 == 1);
  atom_2_r1_1 = v27;
  int v28 = (v10_r4 == 0);
  atom_2_r4_0 = v28;
  int v29 = (v12_r7 == 1);
  atom_2_r7_1 = v29;
  int v30 = (v16_r9 == 0);
  atom_2_r9_0 = v30;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[3] = 0;
  vars[2] = 0;
  vars[1] = 0;
  vars[0] = 0;
  atom_1_r1_1 = 0;
  atom_2_r1_1 = 0;
  atom_2_r4_0 = 0;
  atom_2_r7_1 = 0;
  atom_2_r9_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v17 = atom_1_r1_1;
  int v18 = atom_2_r1_1;
  int v19 = atom_2_r4_0;
  int v20 = atom_2_r7_1;
  int v21 = atom_2_r9_0;
  int v22_conj = v20 & v21;
  int v23_conj = v19 & v22_conj;
  int v24_conj = v18 & v23_conj;
  int v25_conj = v17 & v24_conj;
  if (v25_conj == 1) assert(0);
  return 0;
}
