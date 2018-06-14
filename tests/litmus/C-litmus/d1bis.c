// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/d1bis.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[4]; 
volatile int atom_0_r2_1; 
volatile int atom_1_r2_1; 
volatile int atom_2_r2_1; 
volatile int atom_3_r2_1; 

void *t0(void *arg){
label_1:;
  int v1_r8 = 100 * 10;
  int v2_r8 = v1_r8 / 10;
  int v3_r8 = v2_r8 * 10;
  int v4_r8 = v3_r8 / 10;
  int v5_r8 = v4_r8 ^ v4_r8;
  int v8_r2 = vars[1+v5_r8];
  vars[0] = 1;
  int v40 = (v8_r2 == 1);
  atom_0_r2_1 = v40;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v9_r8 = 100 / 10;
  int v10_r8 = v9_r8 * 10;
  int v11_r8 = v10_r8 / 10;
  int v12_r8 = v11_r8 * 10;
  int v13_r8 = v12_r8 ^ v12_r8;
  int v16_r2 = vars[0+v13_r8];
  vars[1] = 1;
  int v41 = (v16_r2 == 1);
  atom_1_r2_1 = v41;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v17_r8 = 100 * 10;
  int v18_r8 = v17_r8 / 10;
  int v19_r8 = v18_r8 * 10;
  int v20_r8 = v19_r8 / 10;
  int v21_r8 = v20_r8 ^ v20_r8;
  int v24_r2 = vars[3+v21_r8];
  vars[2] = 1;
  int v42 = (v24_r2 == 1);
  atom_2_r2_1 = v42;
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v25_r8 = 100 / 10;
  int v26_r8 = v25_r8 * 10;
  int v27_r8 = v26_r8 / 10;
  int v28_r8 = v27_r8 * 10;
  int v29_r8 = v28_r8 ^ v28_r8;
  int v32_r2 = vars[2+v29_r8];
  vars[3] = 1;
  int v43 = (v32_r2 == 1);
  atom_3_r2_1 = v43;
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
  vars[3] = 0;
  atom_0_r2_1 = 0;
  atom_1_r2_1 = 0;
  atom_2_r2_1 = 0;
  atom_3_r2_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v33 = atom_0_r2_1;
  int v34 = atom_1_r2_1;
  int v35_conj = v33 & v34;
  int v36 = atom_2_r2_1;
  int v37 = atom_3_r2_1;
  int v38_conj = v36 & v37;
  int v39_disj = v35_conj | v38_conj;
  if (v39_disj == 1) assert(0);
  return 0;
}
