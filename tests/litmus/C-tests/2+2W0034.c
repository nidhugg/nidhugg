// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/2+2W0034.litmus

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
  vars[2] = 1;
  int v2_r5 = vars[0];
  vars[0] = 1;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[2] = 0;
  vars[0] = 0;
  vars[1] = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v3 = vars[0];
  int v4 = (v3 == 2);
  int v5 = vars[1];
  int v6 = (v5 == 2);
  int v7_conj = v4 & v6;
  if (v7_conj == 1) assert(0);
  return 0;
}
