// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/m6.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_1_r2_1; 
volatile int atom_2_r2_2; 
volatile int atom_2_r1_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r2 = vars[0];
  vars[1] = 1;
  int v15 = (v2_r2 == 1);
  atom_1_r2_1 = v15;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v4_r2 = vars[1];
  int v6_r1 = vars[0];
  int v16 = (v4_r2 == 2);
  atom_2_r2_2 = v16;
  int v17 = (v6_r1 == 0);
  atom_2_r1_0 = v17;
  return NULL;
}

void *t3(void *arg){
label_4:;
  vars[1] = 2;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  vars[1] = 0;
  vars[0] = 0;
  atom_1_r2_1 = 0;
  atom_2_r2_2 = 0;
  atom_2_r1_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v7 = atom_1_r2_1;
  int v8 = atom_2_r2_2;
  int v9 = atom_2_r1_0;
  int v10 = vars[1];
  int v11 = (v10 == 2);
  int v12_conj = v9 & v11;
  int v13_conj = v8 & v12_conj;
  int v14_conj = v7 & v13_conj;
  if (v14_conj == 1) assert(0);
  return 0;
}
