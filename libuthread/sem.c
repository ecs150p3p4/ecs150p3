#include <stddef.h>
#include <stdlib.h>

#include "queue.h"
#include "sem.h"
#include "thread.h"

struct semaphore {
	/* TODO Phase 1 */
    size_t curr_count;
    queue_t block_queue;
};

sem_t sem_create(size_t count)
{
	/* TODO Phase 1 */
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
	/* TODO Phase 1 */
    if (sem == NULL){
        return -1;
    }
    if (queue_destroy(sem->block_queue) == -1){
        return -1;
    }
    free(sem);
    return 0;
}

int sem_down(sem_t sem)
{
	/* TODO Phase 1 */
    if (sem == NULL){
        return -1;
    }
    enter_critical_section();
    if (sem->curr_count != 0){
        sem->curr_count--;
        exit_critical_section();
    }
    else{
        exit_critical_section();
        //free tid when dequeue;
        pthread_t* tid;
        tid = malloc(sizeof(pthread_t));
        *tid = pthread_self();
        enter_critical_section();
        queue_enqueue(sem->block_queue, tid);
        exit_critical_section();
        thread_block();
        //trying to take resource
        sem_down(sem);
    }
    return 0;
}

int sem_up(sem_t sem)
{
	/* TODO Phase 1 */
    if (sem == NULL){
        return -1;
    }
    
    enter_critical_section();
    sem->curr_count ++;
    exit_critical_section();
    
    pthread_t* tid;
    enter_critical_section();
    if (queue_dequeue(sem->block_queue,(void **) &tid) == 0){
        thread_unblock(*tid);
    }
    exit_critical_section();
    return 0;
    
}

int sem_getvalue(sem_t sem, int *sval)
{
	/* TODO Phase 1 */
    if (sem == NULL || sval == NULL){
        return -1;
    }
    if (sem->curr_count > 0){
        *sval = sem->curr_count;
    }else{
        enter_critical_section();
        *sval = -1* queue_length(sem->block_queue);
        exit_critical_section();
    }
    return 0;
}

