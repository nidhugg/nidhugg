// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/b8.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[4]; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;

  vars[1] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  int v2_r1 = vars[1];
  vars[3] = v2_r1;
  int v4_r1 = vars[0];
  vars[2] = v4_r1;
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

  int v5 = vars[3];
  int v6 = (v5 == 1);
  int v7 = vars[2];
  int v8 = (v7 == 0);
  int v9_conj = v6 & v8;
  if (v9_conj == 1) assert(0);
  return 0;
}
