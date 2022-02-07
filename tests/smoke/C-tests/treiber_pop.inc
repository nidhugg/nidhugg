// -*- mode: c -*-

#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include <stdatomic.h>

#ifndef N
# define N 3
#endif

struct node {
  struct node *next;
  int value;
};

#ifdef USE_ATOMIC_POINTER_TYPE
typedef struct node *topval;
#  define TO_TOPVAL(ptr) (ptr)
#  define FROM_TOPVAL(val) (val)
#else
 /* Using atomic pointers generates allocas that mem2reg does not
  * remove. */
typedef uintptr_t topval;
#  define TO_TOPVAL(PTR) ((topval)PTR)
#  define FROM_TOPVAL(VAL) ((struct node *)VAL)
#endif

struct stack {
  _Atomic(topval) top;
};

void free_at_next_quiescent_state(void *ptr) { /* We assume this is correct */ }

int pop(struct stack *stack) {
  topval oldHead, newHead;
  do {
    oldHead = stack->top; /* sic! We intentionally re-read oldHead here */
    if (!oldHead) return -1;
    newHead = TO_TOPVAL(FROM_TOPVAL(oldHead)->next);
  } while (!atomic_compare_exchange_strong(&stack->top, &oldHead, newHead));
  int ret = FROM_TOPVAL(oldHead)->value;
  free_at_next_quiescent_state(FROM_TOPVAL(oldHead));
  return ret;
}

void unsafe_push(struct stack *stack, int item) {
  struct node *newNode = malloc(sizeof(*newNode));
  newNode->value = item;
  newNode->next = FROM_TOPVAL(stack->top);
  stack->top = TO_TOPVAL(newNode);
}

void init(struct stack *stack) {
  atomic_init(&stack->top, TO_TOPVAL(NULL));
}

struct stack *stack;

void *thread(void *arg) {
  int val = pop(stack);
  assert(val >= 0 && val < N);
  return arg;
}

int main() {
  stack = malloc(sizeof(*stack));
  init(stack);
  for (int i = 0; i < N; ++i) unsafe_push(stack, i);

  pthread_t ts[N];
  for (int i = 0; i < N; ++i) pthread_create(ts+i, NULL, thread, NULL);
  return 0;
}
