// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/wrcv4.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_1_r5_1; 
volatile int atom_2_r5_1; 
volatile int atom_3_r5_1; 
volatile int atom_2_r6_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r5 = vars[0];
  vars[1] = 1;
  int v18 = (v2_r5 == 1);
  atom_1_r5_1 = v18;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v4_r5 = vars[2];
  int v5_r10 = v4_r5 ^ v4_r5;
  int v8_r6 = vars[0+v5_r10];
  int v19 = (v4_r5 == 1);
  atom_2_r5_1 = v19;
  int v21 = (v8_r6 == 0);
  atom_2_r6_0 = v21;
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v10_r5 = vars[1];
  vars[2] = 1;
  int v20 = (v10_r5 == 1);
  atom_3_r5_1 = v20;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  vars[1] = 0;
  vars[2] = 0;
  vars[0] = 0;
  atom_1_r5_1 = 0;
  atom_2_r5_1 = 0;
  atom_3_r5_1 = 0;
  atom_2_r6_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v11 = atom_1_r5_1;
  int v12 = atom_2_r5_1;
  int v13 = atom_3_r5_1;
  int v14 = atom_2_r6_0;
  int v15_conj = v13 & v14;
  int v16_conj = v12 & v15_conj;
  int v17_conj = v11 & v16_conj;
  if (v17_conj == 1) assert(0);
  return 0;
}
