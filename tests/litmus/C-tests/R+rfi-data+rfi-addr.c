// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/R+rfi-data+rfi-addr.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r3_1; 
volatile int atom_1_r3_2; 
volatile int atom_1_r5_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  int v2_r3 = vars[0];
  int v3_r4 = v2_r3 ^ v2_r3;
  int v4_r4 = v3_r4 + 1;
  vars[1] = v4_r4;
  int v19 = (v2_r3 == 1);
  atom_0_r3_1 = v19;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 2;
  int v6_r3 = vars[1];
  int v7_r4 = v6_r3 ^ v6_r3;
  int v10_r5 = vars[0+v7_r4];
  int v20 = (v6_r3 == 2);
  atom_1_r3_2 = v20;
  int v21 = (v10_r5 == 0);
  atom_1_r5_0 = v21;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 0;
  vars[1] = 0;
  atom_0_r3_1 = 0;
  atom_1_r3_2 = 0;
  atom_1_r5_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v11 = vars[1];
  int v12 = (v11 == 2);
  int v13 = atom_0_r3_1;
  int v14 = atom_1_r3_2;
  int v15 = atom_1_r5_0;
  int v16_conj = v14 & v15;
  int v17_conj = v13 & v16_conj;
  int v18_conj = v12 & v17_conj;
  if (v18_conj == 1) assert(0);
  return 0;
}
