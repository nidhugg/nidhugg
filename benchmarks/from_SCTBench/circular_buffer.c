/* This is program circular_buffer_ok.c from SCTBench
 *   https://github.com/mc-imperial/sctbench
 * It has been modified to be parametric on N from the command line
 * and to compile without warnings.
 */

#include <stddef.h>
#include <assert.h>
#include <pthread.h>

#define BUFFER_MAX  10
#ifndef N
#  warning "N is not defined; assuming 7"
#  define N 7
#endif
#define ERROR -1
#define FALSE 0
#define TRUE 1

static char buffer[BUFFER_MAX];   /* BUFFER */

static unsigned int first;        /* Variable to point to the input buffer   */
static unsigned int next;         /* Variable to point to the output pointer */
static int buffer_size;           /* Max amount of elements in the buffer    */

_Bool send, receive;
int value;

pthread_mutex_t m;

void initLog(int max) 
{
  buffer_size = max;
  first = next = 0;
}

int removeLogElement(void) 
{
  assert(first>=0);

  if (next > 0 && first < buffer_size) 
  {
    first++;
    return buffer[first-1];
  }
  else 
  {
    return ERROR;
  }
}  

int insertLogElement(int b) 
{
  if (next < buffer_size && buffer_size > 0) 
  {
    buffer[next] = b;
    next = (next+1)%buffer_size;
    assert(next<buffer_size);
  } 
  else 
  {
    return ERROR;
  }

  return b;
}

void *t1(void *arg) 
{
  int i;

  for(i=0; i<N; i++)
  {
    pthread_mutex_lock(&m);
    if (send) 
    {
      assert(i==insertLogElement(i));
      value=i;
      send=FALSE;
      receive=TRUE;
    }
    pthread_mutex_unlock(&m);
  }
  return NULL;
}

void *t2(void *arg) 
{
  int i;

  for(i=0; i<N; i++)
  {
    pthread_mutex_lock(&m);
    if (receive) 
    {
      assert(removeLogElement()==value);
      receive=FALSE;
      send=TRUE;
    }
    pthread_mutex_unlock(&m);
  }
  return NULL;
}

int main(int argc, char *argv[]) {

  pthread_t id1, id2;

  pthread_mutex_init(&m, 0);

  initLog(10);
  send=TRUE;
  receive=FALSE;

  pthread_create(&id1, NULL, t1, NULL);
  pthread_create(&id2, NULL, t2, NULL);

  pthread_join(id1, NULL);
  pthread_join(id2, NULL);

  return 0;
}

