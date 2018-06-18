// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/DETOUR0879.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_1_r6_2; 
volatile int atom_1_r7_2; 

void *t0(void *arg){
label_1:;
  vars[0] = 3;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 2;
  vars[0] = 1;
  vars[0] = 2;
  int v2_r6 = vars[0];
  int v4_r7 = vars[0];
  int v14 = (v2_r6 == 2);
  atom_1_r6_2 = v14;
  int v15 = (v4_r7 == 2);
  atom_1_r7_2 = v15;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[1] = 0;
  vars[0] = 0;
  atom_1_r6_2 = 0;
  atom_1_r7_2 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v5 = vars[1];
  int v6 = (v5 == 2);
  int v7 = vars[0];
  int v8 = (v7 == 3);
  int v9 = atom_1_r6_2;
  int v10 = atom_1_r7_2;
  int v11_conj = v9 & v10;
  int v12_conj = v8 & v11_conj;
  int v13_conj = v6 & v12_conj;
  if (v13_conj == 1) assert(0);
  return 0;
}
