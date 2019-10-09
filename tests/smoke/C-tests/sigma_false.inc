#include <pthread.h>
// #include <assert.h>
#include <stdatomic.h>

atomic_int array[N];
atomic_int array_index=-1;

void *thread(void * arg)
{
    atomic_store(array + atomic_load(&array_index), 1);
    return 0;
}

int main()
{
    int tid, sum;
    pthread_t t[N];

    for (tid=0; tid<N; tid++) {
	atomic_store(&array_index, atomic_load(&array_index)+1);
	pthread_create(&t[tid], 0, thread, 0);
    }

    for (tid=0; tid<N; tid++) {
	pthread_join(t[tid], 0);
    }

    for (tid=sum=0; tid<N; tid++) {
	sum += atomic_load(array + tid);
    }

    //assert(sum == N);  // <-- wrong, different threads might use the same array offset when writing

    return 0;
}
