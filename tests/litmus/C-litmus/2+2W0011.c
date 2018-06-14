// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/2+2W0011.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[4]; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 2;
  int v2_r3 = vars[2];
  int v3_r5 = v2_r3 ^ v2_r3;
  int v6_r6 = vars[3+v3_r5];
  int v7_r8 = v6_r6 ^ v6_r6;
  int v8_r8 = v7_r8 + 1;
  vars[0] = v8_r8;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 0;
  vars[2] = 0;
  vars[3] = 0;
  vars[1] = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v9 = vars[0];
  int v10 = (v9 == 2);
  int v11 = vars[1];
  int v12 = (v11 == 2);
  int v13_conj = v10 & v12;
  if (v13_conj == 1) assert(0);
  return 0;
}
