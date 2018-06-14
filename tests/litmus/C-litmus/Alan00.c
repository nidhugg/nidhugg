// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/Alan00.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[5]; 
volatile int atom_2_r6_2; 
volatile int atom_2_r7_0; 
volatile int atom_2_r8_1; 
volatile int atom_3_r6_1; 
volatile int atom_3_r7_0; 
volatile int atom_3_r8_1; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  vars[1] = 1;

  vars[2] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[3] = 2;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v2_r6 = vars[3];
  int v3_r7 = v2_r6 ^ v2_r6;
  int v6_r7 = vars[0+v3_r7];
  int v8_r8 = vars[1];
  int v9_r9 = v8_r8 ^ v8_r8;
  vars[4+v9_r9] = 1;
  int v33 = (v2_r6 == 2);
  atom_2_r6_2 = v33;
  int v34 = (v6_r7 == 0);
  atom_2_r7_0 = v34;
  int v35 = (v8_r8 == 1);
  atom_2_r8_1 = v35;
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v11_r6 = vars[4];
  int v12_r7 = v11_r6 ^ v11_r6;
  int v15_r7 = vars[1+v12_r7];
  int v17_r8 = vars[2];
  int v18_r9 = v17_r8 ^ v17_r8;
  vars[3+v18_r9] = 1;
  int v36 = (v11_r6 == 1);
  atom_3_r6_1 = v36;
  int v37 = (v15_r7 == 0);
  atom_3_r7_0 = v37;
  int v38 = (v17_r8 == 1);
  atom_3_r8_1 = v38;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  vars[3] = 0;
  vars[1] = 0;
  vars[4] = 0;
  vars[2] = 0;
  vars[0] = 0;
  atom_2_r6_2 = 0;
  atom_2_r7_0 = 0;
  atom_2_r8_1 = 0;
  atom_3_r6_1 = 0;
  atom_3_r7_0 = 0;
  atom_3_r8_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v19 = vars[3];
  int v20 = (v19 == 2);
  int v21 = atom_2_r6_2;
  int v22 = atom_2_r7_0;
  int v23 = atom_2_r8_1;
  int v24 = atom_3_r6_1;
  int v25 = atom_3_r7_0;
  int v26 = atom_3_r8_1;
  int v27_conj = v25 & v26;
  int v28_conj = v24 & v27_conj;
  int v29_conj = v23 & v28_conj;
  int v30_conj = v22 & v29_conj;
  int v31_conj = v21 & v30_conj;
  int v32_conj = v20 & v31_conj;
  if (v32_conj == 1) assert(0);
  return 0;
}
