// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MOREDETOUR0097.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_0_r3_0; 
volatile int atom_1_r3_2; 
volatile int atom_2_r1_2; 
volatile int atom_3_r1_4; 
volatile int atom_3_r4_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  int v2_r3 = vars[1];
  vars[1] = 2;
  vars[1] = 4;
  int v25 = (v2_r3 == 0);
  atom_0_r3_0 = v25;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 1;

  int v4_r3 = vars[1];
  int v26 = (v4_r3 == 2);
  atom_1_r3_2 = v26;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v6_r1 = vars[1];
  vars[1] = 3;
  int v27 = (v6_r1 == 2);
  atom_2_r1_2 = v27;
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v8_r1 = vars[1];
  int v9_r3 = v8_r1 ^ v8_r1;
  int v12_r4 = vars[0+v9_r3];
  int v28 = (v8_r1 == 4);
  atom_3_r1_4 = v28;
  int v29 = (v12_r4 == 0);
  atom_3_r4_0 = v29;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  vars[1] = 0;
  vars[0] = 0;
  atom_0_r3_0 = 0;
  atom_1_r3_2 = 0;
  atom_2_r1_2 = 0;
  atom_3_r1_4 = 0;
  atom_3_r4_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v13 = atom_0_r3_0;
  int v14 = atom_1_r3_2;
  int v15 = vars[1];
  int v16 = (v15 == 4);
  int v17 = atom_2_r1_2;
  int v18 = atom_3_r1_4;
  int v19 = atom_3_r4_0;
  int v20_conj = v18 & v19;
  int v21_conj = v17 & v20_conj;
  int v22_conj = v16 & v21_conj;
  int v23_conj = v14 & v22_conj;
  int v24_conj = v13 & v23_conj;
  if (v24_conj == 1) assert(0);
  return 0;
}
