// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/dd4.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_2_r2_1; 
volatile int atom_3_r2_1; 

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
  int v2_r2 = vars[1];
  int v3_r10 = v2_r2 ^ v2_r2;
  vars[0+v3_r10] = 1;
  int v10 = (v2_r2 == 1);
  atom_2_r2_1 = v10;
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v5_r2 = vars[0];
  int v6_r10 = v5_r2 ^ v5_r2;
  vars[1+v6_r10] = 1;
  int v11 = (v5_r2 == 1);
  atom_3_r2_1 = v11;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  vars[0] = 0;
  vars[1] = 0;
  atom_2_r2_1 = 0;
  atom_3_r2_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v7 = atom_2_r2_1;
  int v8 = atom_3_r2_1;
  int v9_conj = v7 & v8;
  if (v9_conj == 1) assert(0);
  return 0;
}
