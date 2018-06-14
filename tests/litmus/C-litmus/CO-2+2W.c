// /home/osboxes/nidhugg_tests/gen-litmuts/power-tests/CO-2+2W.litmus

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

volatile int vars[1]; 

void *t0(void *arg){
label_1:;
  vars[0] = 2;
  vars[0] = 1;
  return NULL;
}

void *t1(void *arg){
label_2:;
  vars[0] = 2;
  vars[0] = 1;
  return NULL;
}

int main(int argc, char *argv[]){
  pthread_t thr0; 
  pthread_t thr1; 

  vars[0] = 0;

  pthread_create(&thr0, NULL, t0, NULL);
  pthread_create(&thr1, NULL, t1, NULL);

  pthread_join(thr0, NULL);
  pthread_join(thr1, NULL);

  int v1 = vars[0];
  int v2 = (v1 == 2);
  if (v2 == 1) assert(0);
  return 0;
}
