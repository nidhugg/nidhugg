// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/e5.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[2]; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  int v2_r3 = vars[0];
  int v3_r9 = v2_r3 ^ v2_r3;
  vars[1+v3_r9] = 2;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[1] = 1;
  int v5_r3 = vars[1];
  int v6_r9 = v5_r3 ^ v5_r3;
  vars[0+v6_r9] = 2;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 0;
  vars[1] = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v7 = vars[0];
  int v8 = (v7 == 1);
  int v9 = vars[1];
  int v10 = (v9 == 1);
  int v11_conj = v8 & v10;
  if (v11_conj == 1) assert(0);
  return 0;
}
