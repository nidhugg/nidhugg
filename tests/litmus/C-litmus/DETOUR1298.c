// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/DETOUR1298.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_1_r6_2; 

void *t0(void *arg){
label_1:;
  vars[0] = 4;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 2;
  vars[0] = 1;
  vars[0] = 2;
  int v2_r6 = vars[0];
  vars[0] = 3;
  int v10 = (v2_r6 == 2);
  atom_1_r6_2 = v10;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[1] = 0;
  vars[0] = 0;
  atom_1_r6_2 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v3 = vars[1];
  int v4 = (v3 == 2);
  int v5 = vars[0];
  int v6 = (v5 == 4);
  int v7 = atom_1_r6_2;
  int v8_conj = v6 & v7;
  int v9_conj = v4 & v8_conj;
  if (v9_conj == 1) assert(0);
  return 0;
}
