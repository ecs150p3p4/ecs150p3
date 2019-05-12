#include <stddef.h>
#include <stdlib.h>

#include "queue.h"
#include "sem.h"
#include "thread.h"

struct semaphore {
  size_t curr_count;
  queue_t block_queue;
};

sem_t sem_create(size_t count)
{
  //initialize space for sem, and blocked queue, return null if mem failure.
  sem_t my_sem = (sem_t) malloc(sizeof(struct semaphore));
  if (my_sem == NULL){
      return NULL;
  }
  my_sem->curr_count = count;
  my_sem->block_queue = queue_create();
  if (my_sem->block_queue == NULL){
      return NULL;
  }
  return my_sem;
}

int sem_destroy(sem_t sem)
{
  //check sem not null, try and destory queue, then free sem.
  enter_critical_section();
  if (sem == NULL){
    exit_critical_section();
    return -1;
  }

  if (queue_destroy(sem->block_queue) == -1){
    exit_critical_section();
    return -1;
  }

  free(sem);
  exit_critical_section();
  return 0;
}

int sem_down(sem_t sem)
{
  pthread_t* tid;

  enter_critical_section();
  if (sem == NULL){
      exit_critical_section();
      return -1;
  }

  //if available take resource
  if (sem->curr_count != 0) {
      sem->curr_count--;
  }
  //else add tid to queue
  else{
    //tid must be freed when it is dequeued
    tid = malloc(sizeof(pthread_t));
    *tid = pthread_self();

    queue_enqueue(sem->block_queue, tid);

    thread_block();
    //when resumed thread is guaranteed to have resource since sem_up does not incriment and instead "gives" the resource to this thread when it gets unblocked
  }

  exit_critical_section();
  return 0;
}

int sem_up(sem_t sem)
{
  pthread_t* tid;

  enter_critical_section();
  if (sem == NULL){
    exit_critical_section();
    return -1;
  }
  //unblock tid if waiting, do not incriment so unblocked thread gets resource
  if (queue_dequeue(sem->block_queue,(void **) &tid) == 0){
    thread_unblock(*tid);
    free(tid); //free dequeued tid
  }
  //else incriment count
  else {
    sem->curr_count ++;
  }
  exit_critical_section();
  return 0;
}

int sem_getvalue(sem_t sem, int *sval)
{
  enter_critical_section();
  if (sem == NULL || sval == NULL) {
    exit_critical_section();
    return -1;
  }
  //return count if resources are available
  if (sem->curr_count > 0) {
    *sval = sem->curr_count;
  }
  //return negative queue length
  else {
    *sval = -1* queue_length(sem->block_queue);
  }
  exit_critical_section();
  return 0;
}
