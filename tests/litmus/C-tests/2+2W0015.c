// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/2+2W0015.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[3]; 

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
  int v4_r5 = vars[2];
  int v5_r6 = v4_r5 ^ v4_r5;
  int v6_r6 = v5_r6 + 1;
  vars[0] = v6_r6;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[1] = 0;
  vars[0] = 0;
  vars[2] = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v7 = vars[0];
  int v8 = (v7 == 2);
  int v9 = vars[1];
  int v10 = (v9 == 2);
  int v11_conj = v8 & v10;
  if (v11_conj == 1) assert(0);
  return 0;
}