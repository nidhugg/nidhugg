// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/co10.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[1]; 

void *t0(void *arg){
label_1:;
  vars[0] = 1;
  vars[0] = 2;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 

  vars[0] = 0;

  pthread_create(&thr0, NULL, t0, NULL);

  pthread_join(thr0, NULL);

  int v1 = vars[0];
  int v2 = (v1 == 1);
  if (v2 == 1) assert(0);
  return 0;
}
