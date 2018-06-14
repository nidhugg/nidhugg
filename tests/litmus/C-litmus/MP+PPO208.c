// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/MP+PPO208.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_1_r1_1; 
volatile int atom_1_r11_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[1];
  int v4_r3 = vars[1];
  int v5_r4 = v4_r3 ^ v4_r3;
  int v8_r5 = vars[2+v5_r4];
  vars[2] = 1;
  int v10_r8 = vars[2];
  int v12_r9 = vars[2];
  int v13_r10 = v12_r9 ^ v12_r9;
  int v16_r11 = vars[0+v13_r10];
  int v20 = (v2_r1 == 1);
  atom_1_r1_1 = v20;
  int v21 = (v16_r11 == 0);
  atom_1_r11_0 = v21;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 0;
  vars[1] = 0;
  vars[2] = 0;
  atom_1_r1_1 = 0;
  atom_1_r11_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v17 = atom_1_r1_1;
  int v18 = atom_1_r11_0;
  int v19_conj = v17 & v18;
  if (v19_conj == 1) assert(0);
  return 0;
}
