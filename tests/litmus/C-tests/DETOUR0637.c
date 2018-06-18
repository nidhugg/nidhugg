// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/DETOUR0637.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_0_r3_0; 
volatile int atom_1_r5_0; 
volatile int atom_1_r7_1; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;

  int v2_r3 = vars[1];
  int v17 = (v2_r3 == 0);
  atom_0_r3_0 = v17;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 1;
  int v4_r3 = vars[2];
  int v6_r5 = vars[0];
  int v8_r7 = vars[0];
  int v18 = (v6_r5 == 0);
  atom_1_r5_0 = v18;
  int v19 = (v8_r7 == 1);
  atom_1_r7_1 = v19;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[0] = 1;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[1] = 0;
  vars[0] = 0;
  vars[2] = 0;
  atom_0_r3_0 = 0;
  atom_1_r5_0 = 0;
  atom_1_r7_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v9 = atom_0_r3_0;
  int v10 = vars[0];
  int v11 = (v10 == 2);
  int v12 = atom_1_r5_0;
  int v13 = atom_1_r7_1;
  int v14_conj = v12 & v13;
  int v15_conj = v11 & v14_conj;
  int v16_conj = v9 & v15_conj;
  if (v16_conj == 1) assert(0);
  return 0;
}
