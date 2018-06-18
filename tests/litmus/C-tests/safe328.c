// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/safe328.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_0_r1_2; 
volatile int atom_0_r3_0; 
volatile int atom_2_r1_1; 
volatile int atom_2_r4_0; 

void *t0(void *arg){
label_1:;
  int v2_r1 = vars[0];

  int v4_r3 = vars[1];
  int v21 = (v2_r1 == 2);
  atom_0_r1_2 = v21;
  int v22 = (v4_r3 == 0);
  atom_0_r3_0 = v22;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 1;

  vars[2] = 1;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v6_r1 = vars[2];
  int v7_r3 = v6_r1 ^ v6_r1;
  int v10_r4 = vars[0+v7_r3];
  int v23 = (v6_r1 == 1);
  atom_2_r1_1 = v23;
  int v24 = (v10_r4 == 0);
  atom_2_r4_0 = v24;
  return NULL;
}

void *t3(void *arg){
label_4:;
  vars[0] = 1;

  vars[0] = 2;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  vars[0] = 0;
  vars[1] = 0;
  vars[2] = 0;
  atom_0_r1_2 = 0;
  atom_0_r3_0 = 0;
  atom_2_r1_1 = 0;
  atom_2_r4_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v11 = vars[0];
  int v12 = (v11 == 2);
  int v13 = atom_0_r1_2;
  int v14 = atom_0_r3_0;
  int v15 = atom_2_r1_1;
  int v16 = atom_2_r4_0;
  int v17_conj = v15 & v16;
  int v18_conj = v14 & v17_conj;
  int v19_conj = v13 & v18_conj;
  int v20_conj = v12 & v19_conj;
  if (v20_conj == 1) assert(0);
  return 0;
}
