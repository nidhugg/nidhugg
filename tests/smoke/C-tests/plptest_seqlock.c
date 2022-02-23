// nidhuggc: -sc -optimal -no-assume-await

#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>

typedef struct {
    pthread_mutex_t m;
    atomic_uint sequence;
} seqlock_t;

unsigned read_seqbegin(seqlock_t *l) {
    unsigned ret;
    do {
	ret = l->sequence;
    } while(ret & 1);
    return ret;
}

bool read_seqretry(seqlock_t *l, unsigned start) {
    return l->sequence != start;
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
