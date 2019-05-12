#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <tps.h>
#include <sem.h>

static sem_t sem3, sem4;

void *latest_mmap_addr; // global variable to make address returned by mmap accessible
void *__real_mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off);
void *__wrap_mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off){
    latest_mmap_addr = __real_mmap(addr, len, prot, flags, fildes, off);
    return latest_mmap_addr;
}


void *thread1(void* arg)
{

	assert(tps_create() == 0);
    assert(tps_create() == -1);

    /*seg fault*/
    char *tps_addr = latest_mmap_addr;
    
    tps_addr[0] = '\0';
    
	assert(tps_destroy() == 0);
    assert(tps_destroy() == -1);
    
	return NULL;
}

void *thread2(void* arg){
    char *buffer = malloc(TPS_SIZE);
    memset(buffer, 0, TPS_SIZE);
    
    assert(tps_read(0, TPS_SIZE, buffer) == -1);
    
    tps_create();
    
    assert(tps_read(0, TPS_SIZE + 1, buffer) == -1);
    
    assert(tps_read(1, TPS_SIZE, buffer) == -1);
    
    assert(tps_read(0, TPS_SIZE, NULL) == -1);
    
    tps_destroy();
    
    assert(tps_write(0, TPS_SIZE, buffer) == -1);
    
    tps_create();
    
    assert(tps_write(0, TPS_SIZE + 1, buffer) == -1);
    
    assert(tps_write(1, TPS_SIZE, buffer) == -1);
    
    assert(tps_write(0, TPS_SIZE, NULL) == -1);
    
    tps_destroy();
    
    return NULL;
}

void *thread4(void* arg)
{
    tps_create();
    sem_up(sem3);
    sem_down(sem4);
    
    return NULL;
}


void *thread3(void* arg)
{
    pthread_t tid;
    pthread_create(&tid, NULL, thread4, NULL);
    
    assert(tps_clone(tid) == -1);
    
    tps_create();
    
    sem_down(sem3);
    
    assert(tps_clone(tid) == -1);
    
    sem_up(sem4);
    
    return NULL;
}

void test1(){
    pthread_t tid;
    
    /* Create thread 1 and wait */
    pthread_create(&tid, NULL, thread1, NULL);
    pthread_join(tid, NULL);
}

void test2(){
    pthread_t tid;
    
    pthread_create(&tid, NULL, thread2, NULL);
    pthread_join(tid, NULL);
    
}

void test3(){
    pthread_t tid;
    
    sem3 = sem_create(0);
    sem4 = sem_create(0);
    
    pthread_create(&tid, NULL, thread3, NULL);
    pthread_join(tid, NULL);
    
    sem_destroy(sem3);
    sem_destroy(sem4);
    
}

int main(int argc, char **argv)
{
    assert(tps_init(1) == 0);
    assert(tps_init(1) == -1);
    
    test2();
    test3();
    test1();
    
	return 0;
}


