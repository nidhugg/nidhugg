// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/DETOUR0062.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_0_r3_0; 
volatile int atom_0_r6_1; 
volatile int atom_1_r1_1; 
volatile int atom_1_r4_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  int v2_r3 = vars[1];
  vars[1] = 1;
  int v4_r6 = vars[1];
  int v5_r7 = v4_r6 ^ v4_r6;
  int v6_r7 = v5_r7 + 1;
  vars[2] = v6_r7;
  int v20 = (v2_r3 == 0);
  atom_0_r3_0 = v20;
  int v21 = (v4_r6 == 1);
  atom_0_r6_1 = v21;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v8_r1 = vars[2];
  int v9_r3 = v8_r1 ^ v8_r1;
  int v12_r4 = vars[0+v9_r3];
  int v22 = (v8_r1 == 1);
  atom_1_r1_1 = v22;
  int v23 = (v12_r4 == 0);
  atom_1_r4_0 = v23;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[2] = 0;
  vars[1] = 0;
  vars[0] = 0;
  atom_0_r3_0 = 0;
  atom_0_r6_1 = 0;
  atom_1_r1_1 = 0;
  atom_1_r4_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v13 = atom_0_r3_0;
  int v14 = atom_0_r6_1;
  int v15 = atom_1_r1_1;
  int v16 = atom_1_r4_0;
  int v17_conj = v15 & v16;
  int v18_conj = v14 & v17_conj;
  int v19_conj = v13 & v18_conj;
  if (v19_conj == 1) assert(0);
  return 0;
}
