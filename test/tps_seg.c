#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <tps.h>
#include <sem.h>

void *latest_mmap_addr; // global variable to make address returned by mmap accessible
void *__real_mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off);
void *__wrap_mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off){
    latest_mmap_addr = __real_mmap(addr, len, prot, flags, fildes, off);return latest_mmap_addr;
}


void *thread1(void* arg)
{

	tps_create();

    /*seg fault*/
    char *tps_addr = latest_mmap_addr;
    
    tps_addr[0] = '\0';
    
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

