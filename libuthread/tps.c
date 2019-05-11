#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "queue.h"
#include "thread.h"
#include "tps.h"

/* TODO: Phase 2 */
typedef struct page{
    int reference_count;
    void* addr;
}page;

typedef struct tps{
    page* tps_page;
    pthread_t tid;
}tps;

queue_t tps_queue;

static int find_tps(void* data, void* argv){
    pthread_t tid = *((pthread_t*)argv);
    
    if(((tps*)data)->tid == tid){
        return 1;
    }
    return 0;
}

static int find_addr(void* data, void* argv){
    void* addr = argv;
    
    if(((tps*)data)->tps_page->addr == addr){
        return 1;
    }
    return 0;
}

static void segv_handler(int sig, siginfo_t *si, void *context)
{
    tps* my_tps;
    /*
     * Get the address corresponding to the beginning of the page where the
     * fault occurred
     */
    void *p_fault = (void*)((uintptr_t)si->si_addr & ~(TPS_SIZE - 1));
    
    /*
     * Iterate through all the TPS areas and find if p_fault matches one of them
     */
    
    queue_iterate(tps_queue, find_addr, p_fault, (void**)&my_tps);
    
    if (my_tps != NULL)
    /* Printf the following error message */
        fprintf(stderr, "TPS protection error!\n");
    
    /* In any case, restore the default signal handlers */
    signal(SIGSEGV, SIG_DFL);
    signal(SIGBUS, SIG_DFL);
    /* And transmit the signal again in order to cause the program to crash */
    raise(sig);
}



int tps_init(int segv)
{
	/* TODO: Phase 2 */
    
    if (tps_queue != NULL){
        return -1;
    }
    tps_queue = queue_create();
    if (tps_queue == NULL){
        return -1;
    }
    
    if (segv) {
        struct sigaction sa;
        
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = segv_handler;
        sigaction(SIGBUS, &sa, NULL);
        sigaction(SIGSEGV, &sa, NULL);
    }
    
    
    return 0;
}

int tps_create(void)
{
	/* TODO: Phase 2 */
    pthread_t cur_tid = pthread_self();
    tps* my_tps;
    enter_critical_section();
    queue_iterate(tps_queue, find_tps, (void*)&cur_tid, (void**)&my_tps);
    exit_critical_section();
    if (my_tps != NULL){
        return -1;
    }
    
    my_tps = malloc(sizeof(tps));
    if (my_tps == NULL){
        return -1;
    }
    
    my_tps->tps_page = malloc(sizeof(page));
    if (my_tps->tps_page == NULL){
        return -1;
    }
    
    my_tps->tps_page->addr = mmap(NULL, TPS_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANON, 10, 0);
    if (my_tps->tps_page->addr == MAP_FAILED){
        return -1;
    }
    
    my_tps->tid = cur_tid;
    my_tps->tps_page->reference_count = 1;
    enter_critical_section();
    queue_enqueue(tps_queue, (void*)my_tps);
    exit_critical_section();
    
    return 0;
}

int tps_destroy(void)
{
	/* TODO: Phase 2 */
    pthread_t cur_tid = pthread_self();
    tps* my_tps;
    enter_critical_section();
    queue_iterate(tps_queue, find_tps, (void*)&cur_tid, (void**)&my_tps);
    exit_critical_section();
    if (my_tps == NULL){
        return -1;
    }
    enter_critical_section();
    queue_delete(tps_queue, (void*) my_tps);
    exit_critical_section();
    munmap(my_tps->tps_page->addr, TPS_SIZE);
    free(my_tps->tps_page);
    free(my_tps);
    
    
    return 0;
}

int tps_read(size_t offset, size_t length, char *buffer)
{
	/* TODO: Phase 2 */
    pthread_t cur_tid = pthread_self();
    tps* my_tps;
    enter_critical_section();
    queue_iterate(tps_queue, find_tps, (void*)&cur_tid, (void**)&my_tps);
    exit_critical_section();
    if (my_tps == NULL || offset + length > TPS_SIZE || buffer == NULL){
        return -1;
    }
    mprotect(my_tps->tps_page->addr, TPS_SIZE, PROT_READ);
    memcpy((void*)buffer, my_tps->tps_page->addr + offset, length);
    mprotect(my_tps->tps_page->addr, TPS_SIZE, PROT_NONE);
    return 0;
}

int tps_write(size_t offset, size_t length, char *buffer)
{
	/* TODO: Phase 2 */
    pthread_t cur_tid = pthread_self();
    tps* my_tps;
    enter_critical_section();
    queue_iterate(tps_queue, find_tps, (void*)&cur_tid, (void**)&my_tps);
    exit_critical_section();
    if (my_tps == NULL || offset + length > TPS_SIZE || buffer == NULL){
        return -1;
    }
    enter_critical_section();
    if (my_tps->tps_page->reference_count > 1){
        page* new_page = malloc(sizeof(page));
        new_page->addr = mmap(NULL, TPS_SIZE, PROT_WRITE, MAP_PRIVATE | MAP_ANON, 10, 0);
        if (new_page->addr == MAP_FAILED){
            return -1;
        }
        new_page->reference_count = 1;
        
        mprotect(my_tps->tps_page->addr, TPS_SIZE, PROT_READ);
        memcpy(new_page->addr, my_tps->tps_page->addr, TPS_SIZE);
        mprotect(my_tps->tps_page->addr, TPS_SIZE, PROT_NONE);
        mprotect(new_page->addr, TPS_SIZE, PROT_NONE);
        my_tps->tps_page->reference_count --;
        my_tps->tps_page = new_page;
    }
    exit_critical_section();
    
    mprotect(my_tps->tps_page->addr, TPS_SIZE, PROT_WRITE);
    memcpy( my_tps->tps_page->addr + offset, (void*)buffer, length);
    mprotect(my_tps->tps_page->addr, TPS_SIZE, PROT_NONE);
    return 0;
}

int tps_clone(pthread_t tid)
{
	/* TODO: Phase 2 */
    tps* target_tps;
    enter_critical_section();
    queue_iterate(tps_queue, find_tps, (void*)&tid, (void**)&target_tps);
    exit_critical_section();
    if (target_tps == NULL){
        return -1;
    }
    
    
    /*
    if (tps_create() == -1){
        return -1;
    }
    
    tps* my_tps;
    pthread_t cur_tid = pthread_self();
    enter_critical_section();
    queue_iterate(tps_queue, find_tps, (void*)&cur_tid, (void**)&my_tps);
    exit_critical_section();
    
    mprotect(target_tps->tps_page->addr, TPS_SIZE, PROT_READ);
    mprotect(my_tps->tps_page->addr, TPS_SIZE, PROT_WRITE);
    memcpy(my_tps->tps_page->addr, target_tps->tps_page->addr, TPS_SIZE);
    mprotect(my_tps->tps_page->addr, TPS_SIZE, PROT_NONE);
    mprotect(target_tps->tps_page->addr, TPS_SIZE, PROT_NONE);
     
    */
    pthread_t cur_tid = pthread_self();
    tps* my_tps;
    enter_critical_section();
    queue_iterate(tps_queue, find_tps, (void*)&cur_tid, (void**)&my_tps);
    exit_critical_section();
    if (my_tps != NULL){
        return -1;
    }
    
    my_tps = malloc(sizeof(tps));
    if (my_tps == NULL){
        return -1;
    }
    enter_critical_section();
    my_tps->tps_page = target_tps->tps_page;
    my_tps->tps_page->reference_count ++;
    
    my_tps->tid = cur_tid;
    queue_enqueue(tps_queue, (void*)my_tps);
    exit_critical_section();
    
    return 0;
}

