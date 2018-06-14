// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/ppc-iwp2.6.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[1]; 
volatile int atom_2_r5_55; 
volatile int atom_2_r6_66; 
volatile int atom_3_r5_66; 
volatile int atom_3_r6_55; 

void *t0(void *arg){
label_1:;
  vars[0] = 55;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[0] = 66;
  return NULL;
}

void *t2(void *arg){
label_3:;
  int v2_r5 = vars[0];
  int v4_r6 = vars[0];
  int v16 = (v2_r5 == 55);
  atom_2_r5_55 = v16;
  int v17 = (v4_r6 == 66);
  atom_2_r6_66 = v17;
  return NULL;
}

void *t3(void *arg){
label_4:;
  int v6_r5 = vars[0];
  int v8_r6 = vars[0];
  int v18 = (v6_r5 == 66);
  atom_3_r5_66 = v18;
  int v19 = (v8_r6 == 55);
  atom_3_r6_55 = v19;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 
  pthread_t thr2; 
  pthread_t thr3; 

  vars[0] = 0;
  atom_2_r5_55 = 0;
  atom_2_r6_66 = 0;
  atom_3_r5_66 = 0;
  atom_3_r6_55 = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);
  pthread_create(&thr2, NULL, t2, NULL);
  pthread_create(&thr3, NULL, t3, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);
  pthread_join(thr2, NULL);
  pthread_join(thr3, NULL);

  int v9 = atom_2_r5_55;
  int v10 = atom_2_r6_66;
  int v11 = atom_3_r5_66;
  int v12 = atom_3_r6_55;
  int v13_conj = v11 & v12;
  int v14_conj = v10 & v13_conj;
  int v15_conj = v9 & v14_conj;
  if (v15_conj == 1) assert(0);
  return 0;
}
