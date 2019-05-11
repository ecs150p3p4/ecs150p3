#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tps.h>
#include <sem.h>

static char msg1[TPS_SIZE] = "Hello world!\n";
static char msg2[TPS_SIZE] = "hello world!\n";

static sem_t sem1, sem2;
void* global_addr;

void* __real_mmap(void *address, size_t length, int protect, int flags, int filedes, off_t offset){
void* __wrap_mmap(void *address, size_t length, int protect, int flags, int filedes, off_t offset){
    global_addr = __real_mmap(address, length, protect, flags, filedes, offset);
    return global_addr;
}

void *thread1(void* arg)
{
	pthread_t tid;
	char *buffer = malloc(TPS_SIZE);

	tps_create();

    /*seg fault*/
    *(char*)global_addr = 'c';
	
	tps_destroy();
	return NULL;
}

int main(int argc, char **argv)
{
	pthread_t tid;


	/* Init TPS API */
	tps_init(1);

	/* Create thread 1 and wait */
	pthread_create(&tid, NULL, thread1, NULL);
	pthread_join(tid, NULL);

	return 0;
}

