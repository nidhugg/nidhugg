/* A recreation of the bug
 * http://bugs.sun.com/view_bug.do?bug_id=6822370
 *
 * based on the description in
 * https://blogs.oracle.com/dave/entry/a_race_in_locksupport_park
 */

//#define FENCE() asm volatile ("mfence" ::: "memory")
#define FENCE() do {						\
	atomic_thread_fence(memory_order_seq_cst);		\
    } while(0)

#ifdef ENABLE_PSO_FENCES
#define PSO_FENCE() FENCE()
#else
#define PSO_FENCE() /*No FENCE*/
#endif

void __VERIFIER_assume(int);

#include <pthread.h>
#include <assert.h>
#include <stdatomic.h>

/* Testing */
atomic_int global_cond = 0; // Some global condition variable which the parker will wait for
atomic_int unparker_finished = 0; // Flag indicating that the unparker thread has finished

/* Small low-level mutex implementation */
atomic_int x = 0, y = 0;

void lock(int id){
    if(id){
    	atomic_store(&x, 1);
	// FENCE();
	__VERIFIER_assume(atomic_load(&y) == 0);
    }else{
	atomic_store(&y, 1);
	// FENCE();
	__VERIFIER_assume(atomic_load(&x) == 0);
    }
}

void unlock(int id){
    // FENCE();
    if(id){
	atomic_store(&x, 0);
    }else{
	atomic_store(&y, 0);
    }
}

/* Returns 0 on success, nonzero on failure. */
int trylock(int id){
    if(id){
	atomic_store(&x, 1);
	// FENCE();
	if(atomic_load(&y) == 0){
	    return 0;
	}else{
	    atomic_store(&x, 0);
	    return 1;
	}
    }else{
	atomic_store(&y, 1);
	// FENCE();
	if(atomic_load(&x) == 0){
	    return 0;
	}else{
	    atomic_store(&y, 0);
	    return 1;
	}
    }
}

/* The parker */
atomic_int parker_counter;
atomic_int parker_cond;

void parker_cond_signal(){
    atomic_store_explicit(&parker_cond, 0, memory_order_relaxed);
}

void parker_cond_wait(int id){
    atomic_store_explicit(&parker_cond, 1, memory_order_relaxed);
    unlock(id);
    assert(!unparker_finished || atomic_load_explicit(&parker_cond, memory_order_relaxed) == 0); // Otherwise wait forever
    __VERIFIER_assume(atomic_load_explicit(&parker_cond, memory_order_relaxed) == 0);
    lock(id);
}

void parker_unpark(){
    lock(0);
    int s = atomic_load_explicit(&parker_counter, memory_order_relaxed);
    atomic_store_explicit(&parker_counter, 1, memory_order_relaxed);
    unlock(0);
    if(s < 1){
	parker_cond_signal();
    }
}

void parker_park(){
    if(atomic_load_explicit(&parker_counter, memory_order_relaxed) > 0){
	atomic_store_explicit(&parker_counter, 0, memory_order_relaxed);
	PSO_FENCE();
	return;
    }
    if(trylock(1) != 0){
	return;
    }
    if(atomic_load_explicit(&parker_counter, memory_order_relaxed) > 0){
	atomic_store_explicit(&parker_counter, 0, memory_order_relaxed);
	unlock(1);
	return;
    }
    parker_cond_wait(1);
    atomic_store_explicit(&parker_counter, 0, memory_order_relaxed);
    unlock(1);
}

/* Testing */

void *parker(void *_arg){
    for (unsigned i = 0; !atomic_load_explicit(&global_cond, memory_order_relaxed); ++i){
	__VERIFIER_assume(i < BOUND);
	parker_park();
    }

    return NULL;
}

void *unparker(void *_arg){
    parker_unpark();
    atomic_store_explicit(&global_cond, 1, memory_order_relaxed);
    FENCE();
    parker_unpark();

    // Done
    FENCE();
    atomic_store_explicit(&unparker_finished, 1, memory_order_relaxed);
    
    return NULL;
}

int main(int argc, char *argv[]){
    atomic_init(&parker_counter, 0);
    atomic_init(&parker_cond, 0);
#ifndef GOTO
    pthread_t t1;
    pthread_create(&t1,NULL,parker,NULL);
#else
 __CPROVER_ASYNC_0: parker(NULL);
#endif
    unparker(NULL);
    return 0;
}
