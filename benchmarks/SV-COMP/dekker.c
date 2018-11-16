/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */

/* Adapted from https://github.com/sosy-lab/sv-benchmarks/blob/master/c/pthread-atomic/dekker_true-unreach-call.c */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>


// shared variables
atomic_int flag1;
atomic_int flag2;
atomic_int  turn;

atomic_int x;

#define LOOP 2

void *p1(void *arg)
{
  	int ok1, ok2;
  	for (int jj=0; jj<LOOP; jj++) {
    	atomic_store_explicit(&flag1, 1, memory_order_seq_cst);
    
    	ok1 = 0;
    	for (int i = 0; i<LOOP; i++)
    	{
      		if (atomic_load_explicit(&flag2, memory_order_seq_cst)) {
        		if (atomic_load_explicit(&turn, memory_order_seq_cst) != 0)
        		{
          			atomic_store_explicit(&flag1, 0, memory_order_seq_cst);
          
					ok2 = 0;
          			for (int j=0; j<LOOP; j++)
          			{
            			if (atomic_load_explicit(&turn, memory_order_seq_cst) == 0) {
              				ok2 = 1;
              				break;
            			}
          			}
          
					if (ok2 == 0) return NULL;
          			atomic_store_explicit(&flag1, 1, memory_order_seq_cst);
        		}
        
      		} else {
        		ok1 = 1;
        		break;
      		}
    	}
    	
		if (ok1 == 0) return NULL;    
    	// critical section
    	atomic_store_explicit(&x, 1, memory_order_seq_cst);
    	assert(atomic_load_explicit(&x, memory_order_seq_cst) == 1);
    
    	atomic_store_explicit(&turn, 1, memory_order_seq_cst);
    	atomic_store_explicit(&flag1, 0, memory_order_seq_cst);
  	}
	return NULL;
}


void *p2(void *arg)
{
  	int ok1, ok2;
  
  	for (int jj=0; jj<LOOP; jj++) {
    	atomic_store_explicit(&flag2, 1, memory_order_seq_cst);
    
    	ok1 = 0;
    	for (int i=0; i<LOOP; i++)
    	{
      		if (atomic_load_explicit(&flag1, memory_order_seq_cst)) {
        		if (atomic_load_explicit(&turn, memory_order_seq_cst) != 1)
        		{
          			atomic_store_explicit(&flag2, 0, memory_order_seq_cst);
          			
					ok2 = 0;
          			for (int j=0; j<LOOP; j++)
          			{
            			if (atomic_load_explicit(&turn, memory_order_seq_cst) == 1) {
              				ok2 = 1;
              				break;
            			}
          			}

          			if (ok2==0) return NULL;
          			atomic_store_explicit(&flag2, 1, memory_order_seq_cst);
        		}
      		} else {
        		ok1 = 1;
        		break;
      		}
    	}

    	if (ok1==0) return NULL;    
    	// critical section
    	atomic_store_explicit(&x, 2, memory_order_seq_cst);
    	assert(atomic_load_explicit(&x, memory_order_seq_cst) == 2);
    
    	atomic_store_explicit(&turn, 0, memory_order_seq_cst);
    	atomic_store_explicit(&flag2, 0, memory_order_seq_cst);
  	}
	return NULL;
}

int main(int argc, char *argv[])
{
  	pthread_t a, b;
  
  	atomic_init(&flag1, 0);
  	atomic_init(&flag2, 0);
  	atomic_init(&turn, 0);
  	atomic_init(&x, 0);
  	
  	pthread_create(&a, NULL, p1, NULL);
  	pthread_create(&b, NULL, p2, NULL);
  
  	pthread_join(a, NULL);
  	pthread_join(b, NULL);
  
  	return 0;
}
