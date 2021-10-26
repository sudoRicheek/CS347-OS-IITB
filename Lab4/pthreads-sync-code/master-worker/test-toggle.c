#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>
#include <pthread.h>
#include "zemaphore.h"

#define NUM_THREADS 3
#define NUM_ITER 10

zem_t zemi[NUM_THREADS];

void *justprint(void *data)
{
  int thread_id = *((int *)data);

  for (int i = 0; i < NUM_ITER; i++) {
    zem_down(&zemi[thread_id]);

    printf("This is thread %d\n", thread_id);
    
    zem_up(&zemi[(thread_id+1)%NUM_THREADS]);
  }
  return 0;
}

int main(int argc, char *argv[])
{
  pthread_t mythreads[NUM_THREADS];
  int mythread_id[NUM_THREADS];
  
  // start with thread 0 ->
  zem_init(&zemi[0], 1);
  for (int i = 1; i < NUM_THREADS; i++) {
    zem_init(&zemi[i], 0);
  }
  
  for (int i = 0; i < NUM_THREADS; i++) {
    mythread_id[i] = i;
    pthread_create(&mythreads[i], NULL, justprint, (void *)&mythread_id[i]);
  }

  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(mythreads[i], NULL);
  }

  return 0;
}
