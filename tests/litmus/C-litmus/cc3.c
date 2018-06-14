// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/cc3.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_2_r2_0; 
volatile int atom_3_r2_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 2;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[0] = 1;

  int v2_r2 = vars[1];
  int v8 = (v2_r2 == 0);
  atom_2_r2_0 = v8;
  return NULL;
}

void *t3(void *arg){
label_4:;
  vars[1] = 1;

  int v4_r2 = vars[0];
  int v9 = (v4_r2 == 0);
  atom_3_r2_0 = v9;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  vars[1] = 0;
  vars[0] = 0;
  atom_2_r2_0 = 0;
  atom_3_r2_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v5 = atom_2_r2_0;
  int v6 = atom_3_r2_0;
  int v7_conj = v5 & v6;
  if (v7_conj == 1) assert(0);
  return 0;
}
