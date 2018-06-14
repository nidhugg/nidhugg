// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/mcl.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 
volatile int atom_1_r2_0; 
volatile int atom_1_r2_1; 

void *t0(void *arg){
label_1:;
  vars[1] = 2;

  vars[0] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[0] = 3;

  int v2_r2 = vars[1];
  int v26 = (v2_r2 == 0);
  atom_1_r2_0 = v26;
  int v27 = (v2_r2 == 1);
  atom_1_r2_1 = v27;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[0] = 2;
  return NULL;
}

void *t3(void *arg){
label_4:;
  vars[1] = 1;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  vars[1] = 0;
  vars[0] = 0;
  atom_1_r2_0 = 0;
  atom_1_r2_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v3 = atom_1_r2_0;
  int v4 = vars[0];
  int v5 = (v4 == 3);
  int v6 = vars[1];
  int v7 = (v6 == 1);
  int v8_conj = v5 & v7;
  int v9_conj = v3 & v8_conj;
  int v10 = atom_1_r2_0;
  int v11 = vars[0];
  int v12 = (v11 == 3);
  int v13 = vars[1];
  int v14 = (v13 == 2);
  int v15_conj = v12 & v14;
  int v16_conj = v10 & v15_conj;
  int v17 = atom_1_r2_1;
  int v18 = vars[0];
  int v19 = (v18 == 3);
  int v20 = vars[1];
  int v21 = (v20 == 2);
  int v22_conj = v19 & v21;
  int v23_conj = v17 & v22_conj;
  int v24_disj = v16_conj | v23_conj;
  int v25_disj = v9_conj | v24_disj;
  if (v25_disj == 1) assert(0);
  return 0;
}
