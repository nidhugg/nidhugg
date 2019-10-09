/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */

/* Get inspiration from the Ariline example in Tables 1 and 2 in the PLDI 2015 paper:
 https://dl.acm.org/citation.cfm?id=2737975
 */

/* This benchmark is buggy in the sense that it is missing synchronization */


#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#ifndef N
#  warning "N was not defined"
#  define N 5
#endif


#define NumOfExtra  (((N / 10) == 0) ? 1 : (N / 10))
#define MaximumCapacity (N - NumOfExtra)



// shared variables
atomic_int numberOfSeatsSold;
atomic_int stopSales;
atomic_int numOfTickets;


void * salethread(void *arg)
{
  if (!atomic_load_explicit(&stopSales, memory_order_seq_cst)) {
    int _numberOfSeatsSold = atomic_load_explicit(&numberOfSeatsSold, memory_order_seq_cst);
    if (_numberOfSeatsSold >= MaximumCapacity) {
      atomic_store_explicit(&stopSales, 1, memory_order_seq_cst);
    } else {
      atomic_store_explicit(&numberOfSeatsSold, _numberOfSeatsSold+1, memory_order_seq_cst);
    }
  }
  
  return NULL;
}





int main(int argc, char **argv)
{
  pthread_t salethreads[N];;
  
  atomic_init(&numOfTickets, N);
  atomic_init(&numberOfSeatsSold, 0);
  atomic_init(&stopSales, 0);
  
  for (int i = 0; i < N; i++) {
    pthread_create(&salethreads[i], 0, salethread, NULL);
  }
  
  for (int i = 0; i < N; i++) {
    pthread_join(salethreads[i], 0);
  }
  
  int _numberOfSeatsSold = atomic_load_explicit(&numberOfSeatsSold, memory_order_seq_cst);
  //assert(_numberOfSeatsSold <= MaximumCapacity); // not too many tickets sold
  //assert(_numberOfSeatsSold >= MaximumCapacity);	// not too few tickets sold
  
  return 0;
}



