// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/SyncWith3NoLoop.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[4]; 
volatile int atom_2_r6_1; 
volatile int atom_2_r1_1; 
volatile int atom_2_r3_0; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  vars[1] = 1;

  vars[2] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[2];
  vars[3] = v2_r1;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v4_r6 = vars[3];
  int v5_cmpeq = (v4_r6 == v4_r6);
  if (v5_cmpeq)  goto lbl_L2; else goto lbl_L2;
lbl_L2:;

  int v7_r1 = vars[1];
  int v9_r3 = vars[0];
  int v15 = (v4_r6 == 1);
  atom_2_r6_1 = v15;
  int v16 = (v7_r1 == 1);
  atom_2_r1_1 = v16;
  int v17 = (v9_r3 == 0);
  atom_2_r3_0 = v17;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 

  vars[1] = 0;
  vars[0] = 0;
  vars[2] = 0;
  vars[3] = 0;
  atom_2_r6_1 = 0;
  atom_2_r1_1 = 0;
  atom_2_r3_0 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);

  int v10 = atom_2_r6_1;
  int v11 = atom_2_r1_1;
  int v12 = atom_2_r3_0;
  int v13_conj = v11 & v12;
  int v14_conj = v10 & v13_conj;
  if (v14_conj == 1) assert(0);
  return 0;
}
