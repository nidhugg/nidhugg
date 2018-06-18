// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MP+lwsync+data-wsi-rfi-addr.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_1_r1_1; 
volatile int atom_1_r6_2; 
volatile int atom_1_r8_0; 

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
  vars[2] = 2;
  int v6_r6 = vars[2];
  int v7_r7 = v6_r6 ^ v6_r6;
  int v10_r8 = vars[0+v7_r7];
  int v19 = (v2_r1 == 1);
  atom_1_r1_1 = v19;
  int v20 = (v6_r6 == 2);
  atom_1_r6_2 = v20;
  int v21 = (v10_r8 == 0);
  atom_1_r8_0 = v21;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[2] = 0;
  vars[0] = 0;
  vars[1] = 0;
  atom_1_r1_1 = 0;
  atom_1_r6_2 = 0;
  atom_1_r8_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v11 = vars[2];
  int v12 = (v11 == 2);
  int v13 = atom_1_r1_1;
  int v14 = atom_1_r6_2;
  int v15 = atom_1_r8_0;
  int v16_conj = v14 & v15;
  int v17_conj = v13 & v16_conj;
  int v18_conj = v12 & v17_conj;
  if (v18_conj == 1) assert(0);
  return 0;
}
