// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/S+PPO628.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[5]; 
volatile int atom_1_r1_1; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[1];
  int v3_r3 = v2_r1 ^ v2_r1;
  int v6_r4 = vars[2+v3_r3];
  int v7_cmpeq = (v6_r4 == v6_r4);
  if (v7_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;
  vars[3] = 1;
  int v9_r8 = vars[3];
  int v11_r9 = vars[3];
  int v12_r10 = v11_r9 ^ v11_r9;
  int v15_r11 = vars[4+v12_r10];
  int v16_cmpeq = (v15_r11 == v15_r11);
  if (v16_cmpeq)  goto lbl_LC01; else goto lbl_LC01;
lbl_LC01:;
  vars[0] = 1;
  int v21 = (v2_r1 == 1);
  atom_1_r1_1 = v21;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[2] = 0;
  vars[3] = 0;
  vars[4] = 0;
  vars[1] = 0;
  vars[0] = 0;
  atom_1_r1_1 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v17 = vars[0];
  int v18 = (v17 == 2);
  int v19 = atom_1_r1_1;
  int v20_conj = v18 & v19;
  if (v20_conj == 1) assert(0);
  return 0;
}
