// nidhuggc: -sc -optimal -unroll=4

#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>

typedef struct {
    pthread_mutex_t m;
    atomic_uint sequence;
} seqlock_t;

unsigned read_seqbegin3(seqlock_t *l) {
    unsigned ret;
    do {
	ret = l->sequence;
    } while(ret & 1);
    return ret;
}
unsigned read_seqbegin2(seqlock_t *l) {
  unsigned ret = read_seqbegin3(l);
  atomic_thread_fence(memory_order_release);
  return ret;
}

unsigned read_seqbegin1(seqlock_t *l) {
  return read_seqbegin2(l);
}

unsigned read_seqbegin(seqlock_t *l) {
  return read_seqbegin1(l);
}


int read_seqretry2(atomic_uint *l, unsigned start) {
    return *l != start;
}

unsigned read_seqretry(seqlock_t *l, unsigned start) {
  return read_seqretry2(&l->sequence, start);
}

void write_seqlock(seqlock_t *l) {
    pthread_mutex_lock(&l->m);
    l->sequence++;
}

void write_sequnlock(seqlock_t *l) {
    l->sequence++;
    pthread_mutex_unlock(&l->m);
}

seqlock_t lock;
atomic_int shared;

void *p(void *arg)
{
	unsigned int seq;
	int data;

	do {
		seq = read_seqbegin(&lock);
		/* Make a copy of the data of interest */
		data = atomic_load(&shared);
	} while (read_seqretry(&lock, seq));
	return arg;
}

void *q(void *unused)
{
	write_seqlock(&lock);
	shared = 1;
	write_sequnlock(&lock);
	return NULL;
}

int main() {
  pthread_t pt, qt;
  pthread_mutex_init(&lock.m, NULL);
  pthread_create(&pt, NULL, p, NULL);
  pthread_create(&qt, NULL, q, NULL);
}
