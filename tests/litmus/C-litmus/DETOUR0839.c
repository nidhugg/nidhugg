// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/DETOUR0839.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_0_r5_2; 
volatile int atom_1_r3_3; 
volatile int atom_1_r7_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  vars[1] = 1;

  int v2_r5 = vars[1];
  int v16 = (v2_r5 == 2);
  atom_0_r5_2 = v16;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 2;
  int v4_r3 = vars[1];
  int v6_r4 = vars[2];
  int v7_r6 = v6_r4 ^ v6_r4;
  int v10_r7 = vars[0+v7_r6];
  int v17 = (v4_r3 == 3);
  atom_1_r3_3 = v17;
  int v18 = (v10_r7 == 0);
  atom_1_r7_0 = v18;
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

  vars[1] = 0;
  vars[2] = 0;
  vars[0] = 0;
  atom_0_r5_2 = 0;
  atom_1_r3_3 = 0;
  atom_1_r7_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v11 = atom_0_r5_2;
  int v12 = atom_1_r3_3;
  int v13 = atom_1_r7_0;
  int v14_conj = v12 & v13;
  int v15_conj = v11 & v14_conj;
  if (v15_conj == 1) assert(0);
  return 0;
}
