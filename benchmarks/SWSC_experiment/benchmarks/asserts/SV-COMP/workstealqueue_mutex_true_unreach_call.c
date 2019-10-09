/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */

/* Adapted from: https://raw.githubusercontent.com/sosy-lab/sv-benchmarks/master/c/pthread-complex/workstealqueue_mutex_true-unreach-call.c */

#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#ifndef N
#  warning "N was not defined, must be power of 2"
#  define N 2
#endif


/********************************************************
 *                                                       *
 *     Copyright (C) Microsoft. All rights reserved.     *
 *                                                       *
 ********************************************************/

#define STATICSIZE 16


pthread_mutex_t  lock;

void __my_atomic_begin(void) {
  pthread_mutex_lock(&lock);
}

void __my_atomic_end(void) {
  pthread_mutex_unlock(&lock);
}




typedef struct Obj {
  atomic_int field;
} Obj;

void Init_ObjType(Obj *r) {
  atomic_init(&r->field, 0);
}

void Operation(Obj *r) {
  int tmp = atomic_load_explicit(&r->field, memory_order_seq_cst);
  atomic_store_explicit(&r->field, tmp+1, memory_order_seq_cst);
}

void Check(Obj *r) {
  assert(atomic_load_explicit(&r->field, memory_order_seq_cst) == 1);
}

//
// A WorkStealQueue is a wait-free, lock-free structure associated with a single
// thread that can Push and Pop elements. Other threads can do Take operations
// on the other end of the WorkStealQueue with little contention.
// </summary>
//
typedef struct WorkStealQueue {
  // A 'WorkStealQueue' always runs its code in a single OS thread. We call this the
  // 'bound' thread. Only the code in the Take operation can be executed by
  // other 'foreign' threads that try to steal work.
  //
  // The queue is implemented as an array. The head and tail index this
  // array. To avoid copying elements, the head and tail index the array modulo
  // the size of the array. By making this a power of two, we can use a cheap
  // bit-and operation to take the modulus. The "mask" is always equal to the
  // size of the task array minus one (where the size is a power of two).
  //
  // The head and tail are volatile as they can be updated from different OS threads.
  // The "head" is only updated by foreign threads as they Take (steal) a task from
  // this queue. By putting a lock in Take, there is at most one foreign thread
  // changing head at a time. The tail is only updated by the bound thread.
  //
  // invariants:
  //   tasks.length is a power of 2
  //   mask == tasks.length-1
  //   head is only written to by foreign threads
  //   tail is only written to by the bound thread
  //   At most one foreign thread can do a Take
  //   All methods except Take are executed from a single bound thread
  //   tail points to the first unused location
  //
  pthread_mutex_t cs;
  
  atomic_long MaxSize;
  atomic_long InitialSize; // must be a power of 2
  
  atomic_long head;  // only updated by Take
  atomic_long tail;  // only updated by Push and Pop
  
  Obj*  elems[STATICSIZE];         // the array of tasks
  atomic_long mask;           // the mask for taking modulus
  
} WorkStealQueue;


WorkStealQueue q;


long my_atomic_exchange(atomic_long *obj, long v) {
  __my_atomic_begin();
  long t = atomic_load_explicit(obj, memory_order_seq_cst);
  atomic_store_explicit(obj, v, memory_order_seq_cst);
  __my_atomic_end();
  return t;
}

_Bool my_atomic_compare_exchange_strong(atomic_long * obj, long* expected, long desired) {
  int ret = 0;
  __my_atomic_begin();
  if (atomic_load_explicit(obj, memory_order_seq_cst)== *expected) {
    atomic_store_explicit(obj, desired, memory_order_seq_cst);
    ret = 1;
  } else {
    *expected = atomic_load_explicit(obj, memory_order_seq_cst);
    ret = 0;
  }
  __my_atomic_end();
  return ret;
}

long readV(atomic_long *v) {
  long expected = 0;
  my_atomic_compare_exchange_strong(v, &expected, 0);
  return expected;
}

void writeV(atomic_long *v, long w) {
  my_atomic_exchange(v, w);
}


void Init_WorkStealQueue(long size) {
  atomic_store_explicit(&q.MaxSize, 1024 * 1024, memory_order_seq_cst);
  atomic_store_explicit(&q.InitialSize, 1024, memory_order_seq_cst);
  pthread_mutex_init(&q.cs, NULL);
  writeV(&q.head, 0);
  atomic_store_explicit(&q.mask, size - 1, memory_order_seq_cst);
  writeV(&q.tail, 0);
  // q.elems = malloc(size * sizeof(Obj*));
}

void Destroy_WorkStealQueue() {}

// Push/Pop and Steal can be executed interleaved. In particular:
// 1) A take and pop should be careful when there is just one element
//    in the queue. This is done by first incrementing the head/decrementing the tail
//    and than checking if it interleaved (head > tail).
// 2) A push and take can interleave in the sense that a push can overwrite the
//    value that is just taken. To account for this, we check conservatively in
//    the push to assume that the size is one less than it actually is.
//
// See the CILK "THE" protocol for more information:
//   "The implementation of the CILK-5 multi-threaded language"
//   Matteo Frigo, Charles Leiserson, and Keith Randall.
//

_Bool Steal(Obj **result) {
  _Bool found;
  pthread_mutex_lock(&q.cs);
  
  // ensure that at most one (foreign) thread writes to head
  // increment the head. Save in local h for efficiency
  //
  long h = readV(&q.head);
  writeV(&q.head, h + 1);
  
  // insert a memory fence here if memory is not sequentially consistent
  //
  if (h < readV(&q.tail)) {
    // == (h+1 <= tail) == (head <= tail)
    //
    // BUG: writeV(&q.head, h + 1);
    long temp = h & atomic_load_explicit(&q.mask, memory_order_seq_cst);
    *result = q.elems[temp];
    found = 1;
  } else {
    // failure: either empty or single element interleaving with pop
    //
    writeV(&q.head, h);              // restore the head
    found = 0;
  }
  pthread_mutex_unlock(&q.cs);
  return found;
}

_Bool SyncPop(Obj **result) {
  _Bool found;
  
  pthread_mutex_lock(&q.cs);
  
  // ensure that no Steal interleaves with this pop
  //
  long t = readV(&q.tail) - 1;
  writeV(&q.tail, t);
  if (readV(&q.head) <= t) {
    // == (head <= tail)
    //
    long temp = t & atomic_load_explicit(&q.mask, memory_order_seq_cst);
    *result = q.elems[temp];
    found = 1;
  } else {
    writeV(&q.tail, t + 1);       // restore tail
    found = 0;
  }
  if (readV(&q.head) > t) {
    // queue is empty: reset head and tail
    //
    writeV(&q.head, 0);
    writeV(&q.tail, 0);
    found = 0;
  }
  pthread_mutex_unlock(&q.cs);
  return found;
}

_Bool Pop(Obj **result) {
  // decrement the tail. Use local t for efficiency.
  //
  long t = readV(&q.tail) - 1;
  writeV(&q.tail, t);
  
  // insert a memory fence here if memory is not sequentially consistent
  //
  if (readV(&q.head) <= t) {
    // BUG:  writeV(&q.tail, t);
    
    // == (head <= tail)
    //
    long temp = t & atomic_load_explicit(&q.mask, memory_order_seq_cst);;
    *result = q.elems[temp];
    return 1;
  } else {
    // failure: either empty or single element interleaving with take
    //
    writeV(&q.tail, t + 1);             // restore the tail
    return SyncPop(result);   // do a single-threaded pop
  }
}

void SyncPush(Obj* elem) {
  pthread_mutex_lock(&q.cs);
  // ensure that no Steal interleaves here
  // cache head, and calculate number of tasks
  //
  long h = readV(&q.head);
  long count = readV(&q.tail) - h;
  
  // normalize indices
  //
  h = h & atomic_load_explicit(&q.mask, memory_order_seq_cst);           // normalize head
  writeV(&q.head, h);
  writeV(&q.tail, h + count);
  
  // check if we need to enlarge the tasks
  //
  if (count >= atomic_load_explicit(&q.mask, memory_order_seq_cst)) {
    // == (count >= size-1)
    //
    long newsize = (atomic_load_explicit(&q.mask, memory_order_seq_cst) == 0 ?
                    q.InitialSize :
                    2 * (atomic_load_explicit(&q.mask, memory_order_seq_cst) + 1));
    
    assert(newsize < atomic_load_explicit(&q.MaxSize, memory_order_seq_cst));
    
    Obj *newtasks[STATICSIZE];
    long i;
    for (i = 0; i < count; i++) {
      long temp = (h + i) & atomic_load_explicit(&q.mask, memory_order_seq_cst);
      newtasks[i] = q.elems[temp];
    }
    for (i = 0; i < newsize; i++) {
      q.elems[i] = newtasks[i];
    }
    // q.elems = newtasks;
    atomic_store_explicit(&q.mask, newsize - 1, memory_order_seq_cst);
    writeV(&q.head, 0);
    writeV(&q.tail, count);
  }
  
  assert(count < atomic_load_explicit(&q.mask, memory_order_seq_cst));
  
  // push the element
  //
  long t = readV(&q.tail);
  long temp = t & atomic_load_explicit(&q.mask, memory_order_seq_cst);
  q.elems[temp] = elem;
  writeV(&q.tail, t + 1);
  pthread_mutex_unlock(&q.cs);
}


void Push(Obj* elem) {
  long t = readV(&q.tail);
  // Careful here since we might interleave with Steal.
  // This is no problem since we just conservatively check if there is
  // enough space left (t < head + size). However, Steal might just have
  // incremented head and we could potentially overwrite the old head
  // entry, so we always leave at least one extra 'buffer' element and
  // check (tail < head + size - 1). This also plays nicely with our
  // initial mask of 0, where size is 2^0 == 1, but the tasks array is
  // still null.
  //
  // Correct: if (t < readV(&q.head) + mask && t < MaxSize)
  // #define BUG3
#ifdef BUG3
  if (t < readV(&q.head) + atomic_load_explicit(&q.mask, memory_order_seq_cst) + 1
      && t < atomic_load_explicit(&q.MaxSize, memory_order_seq_cst))
#else
    if (t < readV(&q.head) + atomic_load_explicit(&q.mask, memory_order_seq_cst)    // == t < head + size - 1
        && t < atomic_load_explicit(&q.MaxSize, memory_order_seq_cst))
#endif
    {
      long temp = t & atomic_load_explicit(&q.mask, memory_order_seq_cst);
      q.elems[temp] = elem;
      writeV(&q.tail, t + 1);       // only increment once we have initialized the task entry.
    } else {
      // failure: we need to resize or re-index
      //
      SyncPush(elem);
    }
}

#define INITQSIZE N // must be power of 2

#define nItems 4
#define nStealers 2
#define nStealAttempts 1

void *Stealer(void *param) {
  int i;
  Obj *r;
  for (i = 0; i < nStealAttempts; i++) {
    if (Steal(&r)) {
      Operation(r);
    }
  }
  return 0;
}

Obj items[nItems];

int main(void) {
  int i;
  pthread_t handles[nStealers];
  
  pthread_mutex_init(&lock, NULL);
  Init_WorkStealQueue(INITQSIZE);
  
  for (i = 0; i < nItems; i++) {
    Init_ObjType(&items[i]);
  }
  
  for (i = 0; i < nStealers; i++) {
    pthread_create(&handles[i], NULL, Stealer, 0);
  }
  
  for (i = 0; i < nItems / 2; i++) {
    Push(&items[2 * i]);
    Push(&items[2 * i + 1]);
    Obj *r;
    if (Pop(&r)) {
      Operation(r);
    }
  }
  
  for (i = 0; i < nItems / 2; i++) {
    Obj *r;
    if (Pop(&r)) {
      Operation(r);
    }
  }
  
  for (i = 0; i < nStealers; i++) {
    pthread_join(handles[i], NULL);
  }
  
  for (i = 0; i < nItems; i++) {
    Check(&items[i]);
  }
  
  return 0;
}
