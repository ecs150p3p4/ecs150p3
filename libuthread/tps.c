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

typedef struct page {
    int reference_count;
    void* addr;
} page;

typedef struct tps {
    page* tps_page;
    pthread_t tid;
} tps;

queue_t tps_queue = NULL;

//searches for tps by comparing tid
static int find_tps(void* data, void* argv) {
  pthread_t tid = *((pthread_t*)argv);

  if(((tps*)data)->tid == tid) {
    return 1;
  }
  return 0;
}

//searches for page by comparing address
static int find_addr(void* data, void* argv)
{
  void* addr = argv;

  if(((tps*)data)->tps_page->addr == addr) {
    return 1;
  }
  return 0;
}

static void segv_handler(int sig, siginfo_t *si, void *context)
{
    tps* my_tps = NULL;
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
  //check if init has already been called by seeing if queue has been created
  if (tps_queue != NULL) {
      return -1;
  }

  //create queue
  tps_queue = queue_create();
  if (tps_queue == NULL) {
      return -1;
  }

  //set segfault handler if desired by user
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
  pthread_t cur_tid = pthread_self();
  tps* my_tps = NULL;

  //search for tps block
  enter_critical_section();
  queue_iterate(tps_queue, find_tps, (void*)&cur_tid, (void**)&my_tps);
  exit_critical_section();
  //return -1 if tps already exists
  if (my_tps != NULL) {
    return -1;
  }
  //create tps block
  my_tps = malloc(sizeof(tps));
  if (my_tps == NULL) {
    return -1;
  }
  //create page struct
  my_tps->tps_page = malloc(sizeof(page));
  if (my_tps->tps_page == NULL) {
    return -1;
  }
  //create page
  my_tps->tps_page->addr = mmap(NULL, TPS_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANON, 10, 0);
  if (my_tps->tps_page->addr == MAP_FAILED) {
    return -1;
  }
  //save tid and set reference_count to 1
  my_tps->tid = cur_tid;
  my_tps->tps_page->reference_count = 1;
  //add to queue
  enter_critical_section();
  queue_enqueue(tps_queue, (void*)my_tps);
  exit_critical_section();

  return 0;
}

int tps_destroy(void)
{
  pthread_t cur_tid = pthread_self();
  tps* my_tps = NULL;
  //search for tps in queue
  enter_critical_section();
  queue_iterate(tps_queue, find_tps, (void*)&cur_tid, (void**)&my_tps);
  exit_critical_section();
  //if not found fail
  if (my_tps == NULL) {
      return -1;
  }
  //delete from queue
  enter_critical_section();
  queue_delete(tps_queue, (void*) my_tps);
  exit_critical_section();

  //if more than one tps points to page, deincrement reference_count and free tps block
  if (my_tps->tps_page->reference_count > 1) {
      my_tps->tps_page->reference_count --;
  }
  //else free page and then free tps block
  else if (my_tps->tps_page->reference_count == 1) {
      munmap(my_tps->tps_page->addr, TPS_SIZE);
      free(my_tps->tps_page);
  }
    free(my_tps);
  return 0;
}

int tps_read(size_t offset, size_t length, char *buffer)
{
  pthread_t cur_tid = pthread_self();
  tps* my_tps = NULL;
  //find tps in queue
  enter_critical_section();
  queue_iterate(tps_queue, find_tps, (void*)&cur_tid, (void**)&my_tps);
  exit_critical_section();
  //check for valid tps block and read info
  if (my_tps == NULL || offset + length > TPS_SIZE || buffer == NULL) {
    return -1;
  }
  //change protections and read
  mprotect(my_tps->tps_page->addr, TPS_SIZE, PROT_READ);
  memcpy((void*)buffer, my_tps->tps_page->addr + offset, length);
  mprotect(my_tps->tps_page->addr, TPS_SIZE, PROT_NONE);
  return 0;
}

int tps_write(size_t offset, size_t length, char *buffer)
{
  pthread_t cur_tid = pthread_self();
  tps* my_tps = NULL;
  //find tps in queue
  enter_critical_section();
  queue_iterate(tps_queue, find_tps, (void*)&cur_tid, (void**)&my_tps);
  exit_critical_section();
  //check for valid tps block and read info
  if (my_tps == NULL || offset + length > TPS_SIZE || buffer == NULL){
    return -1;
  }

  enter_critical_section();
  //if more than one reference, copy page then write
  if (my_tps->tps_page->reference_count > 1) {
    //make space for new page
    page* new_page = malloc(sizeof(page));
    new_page->addr = mmap(NULL, TPS_SIZE, PROT_WRITE, MAP_PRIVATE | MAP_ANON, 10, 0);
    if (new_page->addr == MAP_FAILED) {
      return -1;
    }
    new_page->reference_count = 1;

    //change permissions and copy page
    mprotect(my_tps->tps_page->addr, TPS_SIZE, PROT_READ);
    memcpy(new_page->addr, my_tps->tps_page->addr, TPS_SIZE);
    mprotect(my_tps->tps_page->addr, TPS_SIZE, PROT_NONE);
    mprotect(new_page->addr, TPS_SIZE, PROT_NONE);
    my_tps->tps_page->reference_count --;
    my_tps->tps_page = new_page;
  }
  exit_critical_section();

  //change permissions and write
  mprotect(my_tps->tps_page->addr, TPS_SIZE, PROT_WRITE);
  memcpy( my_tps->tps_page->addr + offset, (void*)buffer, length);
  mprotect(my_tps->tps_page->addr, TPS_SIZE, PROT_NONE);
  return 0;
}

int tps_clone(pthread_t tid)
{
  tps* target_tps = NULL;
  tps* my_tps = NULL;
  pthread_t cur_tid = pthread_self();

  //seach for target tps in queue
  enter_critical_section();
  queue_iterate(tps_queue, find_tps, (void*)&tid, (void**)&target_tps);
  exit_critical_section();
  if (target_tps == NULL) {
    return -1;
  }
  //search for current tps in queue
  enter_critical_section();
  queue_iterate(tps_queue, find_tps, (void*)&cur_tid, (void**)&my_tps);
  exit_critical_section();
  if (my_tps != NULL) {
    return -1;
  }

  //make new tps block
  my_tps = malloc(sizeof(tps));
  if (my_tps == NULL) {
    return -1;
  }
  //update reference_count
  enter_critical_section();
  my_tps->tps_page = target_tps->tps_page;
  my_tps->tps_page->reference_count ++;

  my_tps->tid = cur_tid;
  queue_enqueue(tps_queue, (void*)my_tps);
  exit_critical_section();

  return 0;
}
