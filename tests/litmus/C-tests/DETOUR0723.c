// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/DETOUR0723.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_1_r1_1; 
volatile int atom_1_r6_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[1];
  vars[2] = 1;
  vars[2] = 2;
  int v4_r6 = vars[0];
  int v8 = (v2_r1 == 1);
  atom_1_r1_1 = v8;
  int v9 = (v4_r6 == 0);
  atom_1_r6_0 = v9;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 0;
  vars[1] = 0;
  vars[2] = 0;
  atom_1_r1_1 = 0;
  atom_1_r6_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v5 = atom_1_r1_1;
  int v6 = atom_1_r6_0;
  int v7_conj = v5 & v6;
  if (v7_conj == 1) assert(0);
  return 0;
}
