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

static sem_t sem1, sem2, sem3, sem4;

void *thread2(void* arg)
{
	char *buffer = malloc(TPS_SIZE);

	/* Create TPS and initialize with *msg1 */
	assert(tps_create() == 0);
	assert(tps_write(0, TPS_SIZE, msg1) == 0);

	/* Read from TPS and make sure it contains the message */
	memset(buffer, 0, TPS_SIZE);
	tps_read(0, TPS_SIZE, buffer);
	assert(!memcmp(msg1, buffer, TPS_SIZE));
	printf("thread2: read OK!\n");

	/* Transfer CPU to thread 1 and get blocked */
	sem_up(sem1);
	sem_down(sem2);

	/* When we're back, read TPS and make sure it sill contains the original */
	memset(buffer, 0, TPS_SIZE);
	tps_read(0, TPS_SIZE, buffer);
	assert(!memcmp(msg1, buffer, TPS_SIZE));
	printf("thread2: read OK!\n");

	/* Transfer CPU to thread 1 and get blocked */
	sem_up(sem1);
	sem_down(sem2);

	/* Destroy TPS and quit */
	assert(tps_destroy() == 0);
	return NULL;
}

void *thread1(void* arg)
{
	pthread_t tid;
	char *buffer = malloc(TPS_SIZE);

	/* Create thread 2 and get blocked */
	pthread_create(&tid, NULL, thread2, NULL);
	sem_down(sem1);

	/* When we're back, clone thread 2's TPS */
	assert(tps_clone(tid) == 0);

	/* Read the TPS and make sure it contains the original */
	memset(buffer, 0, TPS_SIZE);
	assert(tps_read(0, TPS_SIZE, buffer) == 0);
	assert(!memcmp(msg1, buffer, TPS_SIZE));
	printf("thread1: read OK!\n");

	/* Modify TPS to cause a copy on write */
	buffer[0] = 'h';
	tps_write(0, 1, buffer);

	/* Transfer CPU to thread 2 and get blocked */
	sem_up(sem2);
	sem_down(sem1);

	/* When we're back, make sure our modification is still there */
	memset(buffer, 0, TPS_SIZE);
	tps_read(0, TPS_SIZE, buffer);
	assert(!strcmp(msg2, buffer));
	printf("thread1: read OK!\n");

	/* Transfer CPU to thread 2 */
	sem_up(sem2);

	/* Wait for thread2 to die, and quit */
	pthread_join(tid, NULL);
	tps_destroy();
	return NULL;
}

void *thread4(void* arg)
{
  char *buffer = malloc(TPS_SIZE);

  tps_create();

	//write to tps
  assert(tps_write(0, TPS_SIZE, msg1) == 0);
	//switch to thread 3
  sem_up(sem3);
  sem_down(sem4);

	//read from tps check message is still there
  memset(buffer, 0, TPS_SIZE);
  assert(tps_read(0, TPS_SIZE, buffer) == 0);
  assert(!memcmp(msg1, buffer, TPS_SIZE));
  sem_up(sem3);
  sem_down(sem4);

  return NULL;
}

void *thread3(void* arg){

    pthread_t tid;
    pthread_create(&tid, NULL, thread4, NULL);
    sem_down(sem3);
		//clone thread4's tps
    tps_clone(tid);
		//destroy our reference
    tps_destroy();

		//switch to thead4
		sem_up(sem4);
    sem_down(sem3);

    sem_up(sem4);

    return NULL;
}

void test_clone_write()
{
  pthread_t tid;

  /* Create two semaphores for thread synchro */
  sem1 = sem_create(0);
  sem2 = sem_create(0);

  /* Create thread 1 and wait */
  pthread_create(&tid, NULL, thread1, NULL);
  pthread_join(tid, NULL);

  /* Destroy resources and quit */
  sem_destroy(sem1);
  sem_destroy(sem2);
}

void test_destroy_clone(){
    pthread_t tid;

    /* Create two semaphores for thread synchro */
    sem3 = sem_create(0);
    sem4 = sem_create(0);

    /* Create thread 1 and wait */
    pthread_create(&tid, NULL, thread3, NULL);
    pthread_join(tid, NULL);

    /* Destroy resources and quit */
    sem_destroy(sem3);
    sem_destroy(sem4);
}

int main(int argc, char **argv)
{

	assert(tps_init(1) == 0);

  test_clone_write();
  test_destroy_clone();


	return 0;
}
