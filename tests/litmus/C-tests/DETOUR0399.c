// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/DETOUR0399.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_0_r3_3; 
volatile int atom_2_r5_2; 
volatile int atom_2_r1_1; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;
  int v2_r3 = vars[0];
  int v3_r4 = v2_r3 ^ v2_r3;
  int v6_r5 = vars[1+v3_r4];
  int v7_r7 = v6_r5 ^ v6_r5;
  vars[2+v7_r7] = 1;
  int v19 = (v2_r3 == 3);
  atom_0_r3_3 = v19;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[0] = 3;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v9_r1 = vars[2];
  int v10_r3 = v9_r1 ^ v9_r1;
  int v11_r3 = v10_r3 + 1;
  vars[0] = v11_r3;

  int v13_r5 = vars[0];
  int v20 = (v13_r5 == 2);
  atom_2_r5_2 = v20;
  int v21 = (v9_r1 == 1);
  atom_2_r1_1 = v21;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[2] = 0;
  vars[0] = 0;
  vars[1] = 0;
  atom_0_r3_3 = 0;
  atom_2_r5_2 = 0;
  atom_2_r1_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v14 = atom_0_r3_3;
  int v15 = atom_2_r5_2;
  int v16 = atom_2_r1_1;
  int v17_conj = v15 & v16;
  int v18_conj = v14 & v17_conj;
  if (v18_conj == 1) assert(0);
  return 0;
}
