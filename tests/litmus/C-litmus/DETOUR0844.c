// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/DETOUR0844.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r5_2; 
volatile int atom_1_r3_2; 
volatile int atom_1_r4_3; 
volatile int atom_1_r6_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  vars[1] = 1;

  int v2_r5 = vars[1];
  int v18 = (v2_r5 == 2);
  atom_0_r5_2 = v18;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 2;
  int v4_r3 = vars[1];
  int v6_r4 = vars[1];
  int v7_r5 = v6_r4 ^ v6_r4;
  int v10_r6 = vars[0+v7_r5];
  int v19 = (v4_r3 == 2);
  atom_1_r3_2 = v19;
  int v20 = (v6_r4 == 3);
  atom_1_r4_3 = v20;
  int v21 = (v10_r6 == 0);
  atom_1_r6_0 = v21;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[1] = 3;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[0] = 0;
  vars[1] = 0;
  atom_0_r5_2 = 0;
  atom_1_r3_2 = 0;
  atom_1_r4_3 = 0;
  atom_1_r6_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v11 = atom_0_r5_2;
  int v12 = atom_1_r3_2;
  int v13 = atom_1_r4_3;
  int v14 = atom_1_r6_0;
  int v15_conj = v13 & v14;
  int v16_conj = v12 & v15_conj;
  int v17_conj = v11 & v16_conj;
  if (v17_conj == 1) assert(0);
  return 0;
}
