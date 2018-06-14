// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/DETOUR0456.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r3_3; 
volatile int atom_2_r5_2; 
volatile int atom_2_r1_2; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;
  int v2_r3 = vars[0];
  int v3_r4 = v2_r3 ^ v2_r3;
  vars[1+v3_r4] = 1;
  vars[1] = 2;
  int v15 = (v2_r3 == 3);
  atom_0_r3_3 = v15;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[0] = 3;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v5_r1 = vars[1];
  int v6_r3 = v5_r1 ^ v5_r1;
  int v7_r3 = v6_r3 + 1;
  vars[0] = v7_r3;

  int v9_r5 = vars[0];
  int v16 = (v9_r5 == 2);
  atom_2_r5_2 = v16;
  int v17 = (v5_r1 == 2);
  atom_2_r1_2 = v17;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[0] = 0;
  vars[1] = 0;
  atom_0_r3_3 = 0;
  atom_2_r5_2 = 0;
  atom_2_r1_2 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v10 = atom_0_r3_3;
  int v11 = atom_2_r5_2;
  int v12 = atom_2_r1_2;
  int v13_conj = v11 & v12;
  int v14_conj = v10 & v13_conj;
  if (v14_conj == 1) assert(0);
  return 0;
}
