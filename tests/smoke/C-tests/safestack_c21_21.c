/* A version of the SafeStack benchmark from SCTBench where pure loops
 * have been manually pruned with __VERIFIER_assume calls, the
 * double-load of head been removed from Push(), and a macro parameter N
 * has been introduced for reducing the size of the benchmark. A value
 * of N=4,4,4 corresponds to the original.
 *
 * Here, N=2,1 by default to reproduce an implementation bug of RMWs in
 * RF-SMC. Correct reads-from trace count for N=2,1 is 36.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>

#ifndef N
#  define N 2,1
#endif

const unsigned iters[] = {N};

#define NUM_THREADS (sizeof(iters)/sizeof(*iters))

#ifndef STACK_SIZE
#  define STACK_SIZE 3 // NUM_THREADS
#endif

void __VERIFIER_assume(int truth);

typedef struct SafeStackItem {
    atomic_int Value;
    atomic_int Next;
} SafeStackItem;

typedef struct SafeStack {
    SafeStackItem array[STACK_SIZE];
    atomic_int head;
    atomic_int count;
} SafeStack;

pthread_t threads[NUM_THREADS];
SafeStack stack;

void Init(int pushCount) {
    int i;
    atomic_init(&stack.count, pushCount);
    atomic_init(&stack.head, 0);
    for (i = 0; i < pushCount - 1; i++) {
	atomic_init(&stack.array[i].Next, i + 1);
    }
    atomic_init(&stack.array[pushCount - 1].Next, -1);
}

int Pop(void) {
    while (true) {
	__VERIFIER_assume(atomic_load(&stack.count) > 1);

	int head1 = atomic_load(&stack.head);
        int next1 = atomic_exchange(&stack.array[head1].Next, -1);

	__VERIFIER_assume(next1 != -1);
	if (next1 >= 0) {
            int head2 = head1;
	    if (atomic_compare_exchange_strong(&stack.head, &head2, next1)) {
		atomic_fetch_sub(&stack.count, 1);
                return head1;
	    } else {
		atomic_exchange(&stack.array[head1].Next, next1);
            }
        }
    }
}

void Push(int index) {
    int head1 = atomic_load(&stack.head);
    do {
	atomic_store(&stack.array[index].Next, head1);
    } while (!(atomic_compare_exchange_strong(&stack.head, &head1, index)));
    atomic_fetch_add(&stack.count, 1);
}


void* thread(void* arg) {
    int idx = (int)(uintptr_t)arg;
    uintptr_t i = iters[idx];
    for (;;) {
	if (i-- == 0) return NULL;
	int elem = Pop();

	// Check for double-pop. Requires N=3,3,1 (or greater) to
	// reproduce original bug
	stack.array[elem].Value = idx;
	assert(stack.array[elem].Value == idx);

	if (i-- == 0) return NULL;
	Push(elem);
    }
    return NULL;
}

int main(void) {
    uintptr_t i;
    Init(STACK_SIZE);
    for (i = 0; i < NUM_THREADS; ++i) {
        pthread_create(&threads[i], NULL, thread, (void*) i);
    }

    for (i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
