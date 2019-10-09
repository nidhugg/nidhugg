/* Copyright (C) 2018
 * This benchmark is part of SWSC
 */


/* Adapted from the benchmark with the same name in
 the CAV'18 paper https://link.springer.com/chapter/10.1007/978-3-319-96142-2_22
 */

#include <assert.h>
#include <atomic>
#include <pthread.h>


/*
 This benchmark simulates communication between
 one application and a set of servers over a
 lossy communication channel.
 
 This is simulated as follows:
 The main function sends requests via a channel to one server
 which is represented as an element of an array.
 
 Either because the server is busy or because
 the channel is lossy, the request might be lost.
 This is simulated by the fact that the server
 just checks if there is something in the input
 channel and terminates.
 
 This server is chosen non-deterministically
 and the main function checks in the end
 that all servers chosen were valid and how
 many requests were lost.
 There is a non-trivial assertion because
 two adjacent packages cannot be sent to the
 same server, thus a server can at most
 receive NUM_REQUESTS/2 + 1 if the number
 of servers is greater.
 
 For a fixed RNUM (e.g. RNUM 1) and an
 increasing SNUM, we obtain an exponential
 number of SSBs in nidhugg.
 
 For a fixed SNUM > 1 (e.g. SNUM 2) and
 an increasing RNUM, we obtain a linear
 number of SSBs in nidhugg.
 */


#ifndef N
#  warning "N not defined, assuming 4"
#  define N 4
#endif


#define PARAM1 N
#define PARAM2 N

#define SNUM PARAM1  // number of servers
#define RNUM PARAM2  // number of requests

pthread_mutex_t servers[SNUM];
pthread_mutex_t k;
std::atomic<int> sid;           // server id
std::atomic<int> in_channel[SNUM];  // input  communication channel
std::atomic<int> out_channel[SNUM]; // output communicatino channel

// this thread just chooses a server
// different from the last one if there
// are more servers than requests
// Right now, it just chooses a server
void *pick_server(void *arg)
{
  unsigned last_server_id = (unsigned long) arg;
  
  for (int x = 0; x < SNUM; x++)
  {
    pthread_mutex_lock(&k);
    sid.store(x,std::memory_order_seq_cst);
    pthread_mutex_unlock(&k);
  }
  return 0;
}

// this thread represents a server
void *server(void *arg)
{
  unsigned id = (unsigned long) arg;
  
  // locks on the server mutex
  pthread_mutex_lock(&servers[id]);
  // checks if it has something in
  // the input channel and if so
  // signals that to the out channel.
  if (in_channel[id].load(std::memory_order_seq_cst) > 0)
    out_channel[id].store(in_channel[id].load(std::memory_order_seq_cst),
                          std::memory_order_seq_cst);
  pthread_mutex_unlock(&servers[id]);
  
  return 0;
}

// this function sends requests to a server
int process_request(int req_id, int last_server_id)
{
  pthread_t s;
  int idx = 0;
  
  // picks a server non-deterministically
  pthread_create(&s, NULL, pick_server, (void*) (long) last_server_id);
  
  // get the server chosen
  pthread_mutex_lock (&k);
  idx = sid.load(std::memory_order_seq_cst);
  pthread_mutex_unlock (&k);
  
  if (SNUM > 0)
  {
    // send req to server
    pthread_mutex_lock (&servers[idx]);
    in_channel[idx].store(in_channel[idx].load(std::memory_order_seq_cst) + 1,
                          std::memory_order_seq_cst);
    pthread_mutex_unlock (&servers[idx]);
  }
  
  pthread_join(s, NULL);
  
  return idx;
}

int main()
{
  pthread_t idw[SNUM];
  int picked_servers[RNUM];
  
  atomic_init(&sid, 0);
  // spawn SNUM servers
  pthread_mutex_init(&k, NULL);
  for (int x = 0; x < SNUM; x++)
  {
    atomic_init(&in_channel[x], 0);
    atomic_init(&out_channel[x], 0);
    pthread_mutex_init(&servers[x], NULL);
    pthread_create(&idw[x], 0, server, (void*) (long) x);
  }
  
  // process requests
  for (int r = 0; r < RNUM; r++){
    if (r == 0)
      picked_servers[r] = process_request(r, SNUM);
    else
      picked_servers[r] = process_request(r,picked_servers[r-1]);
  }
  
  pthread_exit (0);
  
  
  // wait for servers to join
  for (int x = 0; x < SNUM; x++)
    pthread_join(idw[x],NULL);
  
  // process results:
  // - counts the number of requests that failed
  // - assert that the picked server was good
  // - assert that a server could only be
  int missed_req = 0;
  
  for (int r = 0; r < RNUM; r++)
  {
    int s = picked_servers[r];
    // bound check
    if (SNUM > 0)
      assert(s >= 0 && s < SNUM);
    // value in the out_channel
    //if (SNUM > RNUM)
    //  assert(out_channel[s] <= RNUM/2 + 1);
    //else
    //  assert(out_channel[s] <= RNUM);
    // if the value is 0 it means it was a miss
    if (out_channel[s].load(std::memory_order_seq_cst))
      out_channel[s].store(out_channel[s].load(std::memory_order_seq_cst) - 1,
                           std::memory_order_seq_cst);
    else
      missed_req++;
  }
  
  return 0;
}
