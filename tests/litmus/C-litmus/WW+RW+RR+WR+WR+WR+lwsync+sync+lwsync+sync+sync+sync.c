// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/WW+RW+RR+WR+WR+WR+lwsync+sync+lwsync+sync+sync+sync.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[6]; 
volatile int atom_1_r1_1; 
volatile int atom_2_r1_1; 
volatile int atom_2_r3_0; 
volatile int atom_3_r3_0; 
volatile int atom_4_r3_0; 
volatile int atom_5_r3_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[1];

  vars[2] = 1;
  int v24 = (v2_r1 == 1);
  atom_1_r1_1 = v24;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v4_r1 = vars[2];

  int v6_r3 = vars[3];
  int v25 = (v4_r1 == 1);
  atom_2_r1_1 = v25;
  int v26 = (v6_r3 == 0);
  atom_2_r3_0 = v26;
  return NULL;
}

void *t3(void *arg){
label_4:;
  vars[3] = 1;

  int v8_r3 = vars[4];
  int v27 = (v8_r3 == 0);
  atom_3_r3_0 = v27;
  return NULL;
}

void *t4(void *arg){
label_5:;
  vars[4] = 1;

  int v10_r3 = vars[5];
  int v28 = (v10_r3 == 0);
  atom_4_r3_0 = v28;
  return NULL;
}

void *t5(void *arg){
label_6:;
  vars[5] = 1;

  int v12_r3 = vars[0];
  int v29 = (v12_r3 == 0);
  atom_5_r3_0 = v29;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 
  pthread_t thr4; 
  pthread_t thr5; 

  vars[3] = 0;
  vars[5] = 0;
  vars[4] = 0;
  vars[2] = 0;
  vars[0] = 0;
  vars[1] = 0;
  atom_1_r1_1 = 0;
  atom_2_r1_1 = 0;
  atom_2_r3_0 = 0;
  atom_3_r3_0 = 0;
  atom_4_r3_0 = 0;
  atom_5_r3_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);
  pthread_create(&thr4, NULL, t4, NULL);
  pthread_create(&thr5, NULL, t5, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);
  pthread_join(thr4, NULL);
  pthread_join(thr5, NULL);

  int v13 = atom_1_r1_1;
  int v14 = atom_2_r1_1;
  int v15 = atom_2_r3_0;
  int v16 = atom_3_r3_0;
  int v17 = atom_4_r3_0;
  int v18 = atom_5_r3_0;
  int v19_conj = v17 & v18;
  int v20_conj = v16 & v19_conj;
  int v21_conj = v15 & v20_conj;
  int v22_conj = v14 & v21_conj;
  int v23_conj = v13 & v22_conj;
  if (v23_conj == 1) assert(0);
  return 0;
}
