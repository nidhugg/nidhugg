// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MOREDETOUR0789.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_1_r1_1; 
volatile int atom_1_r6_0; 
volatile int atom_2_r3_2; 

void *t0(void *arg){
label_1:;
  vars[0] = 3;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[1];
  int v3_cmpeq = (v2_r1 == v2_r1);
  if (v3_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;
  int v5_r3 = vars[2];
  int v6_r5 = v5_r3 ^ v5_r3;
  int v9_r6 = vars[0+v6_r5];
  vars[0] = 2;
  int v20 = (v2_r1 == 1);
  atom_1_r1_1 = v20;
  int v21 = (v9_r6 == 0);
  atom_1_r6_0 = v21;
  return NULL;
}

void *t2(void *arg){
label_3:;
  vars[0] = 1;

  int v11_r3 = vars[0];
  int v22 = (v11_r3 == 2);
  atom_2_r3_2 = v22;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[0] = 0;
  vars[1] = 0;
  vars[2] = 0;
  atom_1_r1_1 = 0;
  atom_1_r6_0 = 0;
  atom_2_r3_2 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v12 = vars[0];
  int v13 = (v12 == 3);
  int v14 = atom_1_r1_1;
  int v15 = atom_1_r6_0;
  int v16 = atom_2_r3_2;
  int v17_conj = v15 & v16;
  int v18_conj = v14 & v17_conj;
  int v19_conj = v13 & v18_conj;
  if (v19_conj == 1) assert(0);
  return 0;
}
