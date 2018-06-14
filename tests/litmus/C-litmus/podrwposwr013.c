// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/podrwposwr013.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[5]; 
volatile int atom_3_r1_1; 
volatile int atom_3_r8_1; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 2;

  vars[2] = 1;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[2] = 2;

  vars[3] = 1;
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v2_r1 = vars[3];
  int v3_r3 = v2_r1 ^ v2_r1;
  int v6_r4 = vars[4+v3_r3];
  vars[0] = 1;
  int v8_r8 = vars[0];
  int v21 = (v2_r1 == 1);
  atom_3_r1_1 = v21;
  int v22 = (v8_r8 == 1);
  atom_3_r8_1 = v22;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  vars[4] = 0;
  vars[1] = 0;
  vars[3] = 0;
  vars[2] = 0;
  vars[0] = 0;
  atom_3_r1_1 = 0;
  atom_3_r8_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v9 = vars[0];
  int v10 = (v9 == 2);
  int v11 = vars[1];
  int v12 = (v11 == 2);
  int v13 = vars[2];
  int v14 = (v13 == 2);
  int v15 = atom_3_r1_1;
  int v16 = atom_3_r8_1;
  int v17_conj = v15 & v16;
  int v18_conj = v14 & v17_conj;
  int v19_conj = v12 & v18_conj;
  int v20_conj = v10 & v19_conj;
  if (v20_conj == 1) assert(0);
  return 0;
}
