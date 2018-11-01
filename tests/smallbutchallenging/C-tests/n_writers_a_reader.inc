// -*- c -*-
/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
 * This benchmark is part of SWSC
 */

/* CDSChecker example
 * https://dl.acm.org/citation.cfm?id=2914585.2806886, page 10, section 5.4
 * There are N+1 weak traces
 */

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

// shared variables
volatile int x;
int args[N];

void *writer(void *arg) {
  int id = *((int *) arg);
  x = id;
  return NULL;
}

void *reader(void *arg) {
  int a = x;
  return NULL;
}

int main(int argc, char *argv[]) {
  pthread_t ws[N], r;
  x = 0;

  for (int i=0; i<N; i++) {
    args[i] = i;
    pthread_create(&ws[i], NULL, writer, &args[i]);
  }
  pthread_create(&r, NULL, reader, NULL);

  for (int i=0; i<N; i++)
    pthread_join(ws[i], NULL);
  pthread_join(r, NULL);

  return 0;
}
