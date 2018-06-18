// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/DETOUR0464.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r6_2; 
volatile int atom_1_r1_3; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;
  vars[1] = 1;
  vars[1] = 2;
  int v2_r6 = vars[1];
  vars[1] = 3;
  int v13 = (v2_r6 == 2);
  atom_0_r6_2 = v13;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v4_r1 = vars[1];
  int v5_r3 = v4_r1 ^ v4_r1;
  int v6_r3 = v5_r3 + 1;
  vars[0] = v6_r3;
  int v14 = (v4_r1 == 3);
  atom_1_r1_3 = v14;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 0;
  vars[1] = 0;
  atom_0_r6_2 = 0;
  atom_1_r1_3 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v7 = atom_0_r6_2;
  int v8 = vars[0];
  int v9 = (v8 == 2);
  int v10 = atom_1_r1_3;
  int v11_conj = v9 & v10;
  int v12_conj = v7 & v11_conj;
  if (v12_conj == 1) assert(0);
  return 0;
}
