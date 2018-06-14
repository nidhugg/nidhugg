// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/DETOUR0533.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 
volatile int atom_0_r3_0; 
volatile int atom_1_r3_0; 
volatile int atom_1_r6_1; 
volatile int atom_1_r7_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  int v2_r3 = vars[1];
  int v17 = (v2_r3 == 0);
  atom_0_r3_0 = v17;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 1;
  int v4_r3 = vars[2];
  vars[2] = 1;
  int v6_r6 = vars[2];
  int v7_cmpeq = (v6_r6 == v6_r6);
  if (v7_cmpeq)  goto lbl_LC00; else goto lbl_LC00;
lbl_LC00:;

  int v9_r7 = vars[0];
  int v18 = (v4_r3 == 0);
  atom_1_r3_0 = v18;
  int v19 = (v6_r6 == 1);
  atom_1_r6_1 = v19;
  int v20 = (v9_r7 == 0);
  atom_1_r7_0 = v20;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[1] = 0;
  vars[2] = 0;
  vars[0] = 0;
  atom_0_r3_0 = 0;
  atom_1_r3_0 = 0;
  atom_1_r6_1 = 0;
  atom_1_r7_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v10 = atom_0_r3_0;
  int v11 = atom_1_r3_0;
  int v12 = atom_1_r6_1;
  int v13 = atom_1_r7_0;
  int v14_conj = v12 & v13;
  int v15_conj = v11 & v14_conj;
  int v16_conj = v10 & v15_conj;
  if (v16_conj == 1) assert(0);
  return 0;
}
