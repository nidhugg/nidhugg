// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/DETOUR1073.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_0_r5_2; 
volatile int atom_2_r3_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  vars[1] = 1;
  int v2_r5 = vars[1];
  vars[2] = 1;
  int v11 = (v2_r5 == 2);
  atom_0_r5_2 = v11;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 2;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[2] = 2;

  int v4_r3 = vars[0];
  int v12 = (v4_r3 == 0);
  atom_2_r3_0 = v12;
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
  atom_2_r3_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v5 = vars[2];
  int v6 = (v5 == 2);
  int v7 = atom_0_r5_2;
  int v8 = atom_2_r3_0;
  int v9_conj = v7 & v8;
  int v10_conj = v6 & v9_conj;
  if (v10_conj == 1) assert(0);
  return 0;
}
