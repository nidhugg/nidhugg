// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/Z6.3+sync+rfi-data+addr.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_1_r3_2; 
volatile int atom_2_r1_1; 
volatile int atom_2_r4_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 2;
  int v2_r3 = vars[1];
  int v3_r4 = v2_r3 ^ v2_r3;
  int v4_r4 = v3_r4 + 1;
  vars[2] = v4_r4;
  int v19 = (v2_r3 == 2);
  atom_1_r3_2 = v19;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v6_r1 = vars[2];
  int v7_r3 = v6_r1 ^ v6_r1;
  int v10_r4 = vars[0+v7_r3];
  int v20 = (v6_r1 == 1);
  atom_2_r1_1 = v20;
  int v21 = (v10_r4 == 0);
  atom_2_r4_0 = v21;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[1] = 0;
  vars[2] = 0;
  vars[0] = 0;
  atom_1_r3_2 = 0;
  atom_2_r1_1 = 0;
  atom_2_r4_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v11 = vars[1];
  int v12 = (v11 == 2);
  int v13 = atom_1_r3_2;
  int v14 = atom_2_r1_1;
  int v15 = atom_2_r4_0;
  int v16_conj = v14 & v15;
  int v17_conj = v13 & v16_conj;
  int v18_conj = v12 & v17_conj;
  if (v18_conj == 1) assert(0);
  return 0;
}
