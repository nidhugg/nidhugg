// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/CO-R.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[1]; 
volatile int atom_1_r1_1; 
volatile int atom_1_r1_2; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  vars[0] = 2;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[0] = 3;
  int v2_r1 = vars[0];
  int v12 = (v2_r1 == 1);
  atom_1_r1_1 = v12;
  int v13 = (v2_r1 == 2);
  atom_1_r1_2 = v13;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 0;
  atom_1_r1_1 = 0;
  atom_1_r1_2 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v3 = vars[0];
  int v4 = (v3 == 3);
  int v5 = atom_1_r1_1;
  int v6 = atom_1_r1_2;
  int v7_disj = v5 | v6;
  int v8_conj = v4 & v7_disj;
  int v9 = vars[0];
  int v10 = (v9 == 1);
  int v11_disj = v8_conj | v10;
  if (v11_disj == 1) assert(0);
  return 0;
}
