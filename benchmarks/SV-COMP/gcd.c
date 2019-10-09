/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */

/* Adapted from: https://github.com/sosy-lab/sv-benchmarks/blob/master/c/pthread-atomic/gcd_true-unreach-call_true-termination.c */

/*
VerifyThis ETAPS 2015, Challenge 2

PARALLEL GCD (60 minutes)
=========================

Algorithm description
---------------------

Various parallel GCD algorithms exist. In this challenge, we consider a
simple Euclid-like algorithm with two parallel threads. One thread
subtracts in one direction, the other thread subtracts in the other
direction, and eventually this procedure converges on GCD.


Implementation
--------------

In pseudocode, the algorithm is described as follows:

(
  WHILE a != b DO                                        
      IF a>b THEN a:=a-b ELSE SKIP FI
  OD
||
  WHILE a != b DO                                        
       IF b>a THEN b:=b-a ELSE SKIP FI
  OD
);
OUTPUT a


Verification tasks
------------------

Specify and verify the following behaviour of this parallel GCD algorithm:

Input:  two positive integers a and b
Output: a positive number that is the greatest common divisor of a and b

Feel free to add synchronisation where appropriate, but try to avoid
blocking of the parallel threads.


Sequentialization
-----------------

If your tool does not support reasoning about parallel threads, you may
verify the following pseudocode algorithm:

WHILE a != b DO
    CHOOSE(
         IF a > b THEN a := a - b ELSE SKIP FI,
         IF b > a THEN b := b - a ELSE SKIP FI
    )
OD;
OUTPUT a
*/

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>


#define A 17
#define B 7
#define LOOP 5

// shared variables
atomic_int a, b; 
pthread_mutex_t lock;


unsigned __VERIFIER_nondet_uint();
void __VERIFIER_assume(int);


void atomic_dec_a(void)
{
	pthread_mutex_lock(&lock);
  	if(atomic_load_explicit(&a, memory_order_seq_cst) > atomic_load_explicit(&b, memory_order_seq_cst))
	{
		int _a = atomic_load_explicit(&a, memory_order_seq_cst);
		int _b = atomic_load_explicit(&b, memory_order_seq_cst);
		atomic_store_explicit(&a, _a - _b, memory_order_seq_cst);
	}
	pthread_mutex_unlock(&lock);
}


void atomic_dec_b(void)
{
	pthread_mutex_lock(&lock);
  	if(atomic_load_explicit(&b, memory_order_seq_cst) > atomic_load_explicit(&a, memory_order_seq_cst))
	{
		int _a = atomic_load_explicit(&a, memory_order_seq_cst);
		int _b = atomic_load_explicit(&b, memory_order_seq_cst);
		atomic_store_explicit(&b, _b - _a, memory_order_seq_cst);
	}
	pthread_mutex_unlock(&lock);
}


void  *dec_a(void* arg)
{
	// using for-if to have finite runs
	for (int i=0; i<LOOP; i++) {
  		if (atomic_load_explicit(&a, memory_order_seq_cst) != atomic_load_explicit(&b, memory_order_seq_cst) != b)
  			atomic_dec_a();
  	}
  	return NULL;
}


void  *dec_b(void* arg)
{
	// using for-if to have finite runs
	for (int i=0; i<LOOP; i++) {
  		if (atomic_load_explicit(&a, memory_order_seq_cst) != atomic_load_explicit(&b, memory_order_seq_cst) != b) 
  			atomic_dec_b();
  	}
  	return NULL;
}

void check_gcd(unsigned a_in, unsigned b_in, unsigned gcd)
{
  	unsigned guessed_gcd=__VERIFIER_nondet_uint();
  	__VERIFIER_assume(guessed_gcd > 1);
  	__VERIFIER_assume(a_in % guessed_gcd == 0);
  	__VERIFIER_assume(b_in % guessed_gcd == 0);

  	assert(a_in % gcd == 0);
  	assert(b_in % gcd == 0);
  	assert(gcd >= guessed_gcd);
}

int main(int argc, char *argv[])
{

	pthread_t t1, t2;

	pthread_mutex_init(&lock, NULL);
	atomic_init(&a, A);
	atomic_init(&b, B);
	
	pthread_create(&t1, NULL, dec_a, NULL);
	pthread_create(&t2, NULL, dec_b, NULL);
 	
 	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	check_gcd(A, B, a);

	return 0;
}
