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

//wrapper to save location of page
void *__real_mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off);
void *__wrap_mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off)
{
  latest_mmap_addr = __real_mmap(addr, len, prot, flags, fildes, off);
  return latest_mmap_addr;
}

//gets page and tries to access it illegally, causing a seg fault
void *thread1(void* arg)
{
	tps_create();

  //seg fault
  char *tps_addr = latest_mmap_addr;

  tps_addr[0] = '\0';

  tps_destroy();

	return NULL;
}

//checks create, destroy, read, and write error cases
void *thread2(void* arg)
{
  char *buffer = malloc(TPS_SIZE);
  memset(buffer, 0, TPS_SIZE);

  //read with no tps fails
  assert(tps_read(0, TPS_SIZE, buffer) == -1);

  assert(tps_create() == 0);

  //create twice fails
  assert(tps_create() == -1);

  //access out of bounds fails: length
  assert(tps_read(0, TPS_SIZE + 1, buffer) == -1);

  //access out of bounds fails: offset
  assert(tps_read(1, TPS_SIZE, buffer) == -1);

  //null buffer fails
  assert(tps_read(0, TPS_SIZE, NULL) == -1);

  tps_destroy();

  //write no tps fails
  assert(tps_write(0, TPS_SIZE, buffer) == -1);

  tps_create();

  //write out of bound length fails
  assert(tps_write(0, TPS_SIZE + 1, buffer) == -1);

  //write out of bound offset fails
  assert(tps_write(1, TPS_SIZE, buffer) == -1);

  //write null buffer fails
  assert(tps_write(0, TPS_SIZE, NULL) == -1);

  assert(tps_destroy() == 0);
  //destroy twice fails
  assert(tps_destroy() == -1);

  return NULL;
}

void *thread4(void* arg)
{
  //create tps for testing later
  tps_create();
  sem_up(sem3);
  sem_down(sem4);

  return NULL;
}


void *thread3(void* arg)
{
  pthread_t tid;
  //create thread 4
  pthread_create(&tid, NULL, thread4, NULL);

  //clone a tps that doesnt exist fails
  assert(tps_clone(tid) == -1);

  tps_create();
  sem_down(sem3);

  //clone when we already have a tps fails
  assert(tps_clone(tid) == -1);

  sem_up(sem4);

  return NULL;
}

void test_seg()
{
  pthread_t tid;

  //create thread 1 and wait
  pthread_create(&tid, NULL, thread1, NULL);
  pthread_join(tid, NULL);
}

void test_read_write_create_destroy(){
  pthread_t tid;

  pthread_create(&tid, NULL, thread2, NULL);
  pthread_join(tid, NULL);

}

void test_clone()
{
  pthread_t tid;

  //create sems for syncing
  sem3 = sem_create(0);
  sem4 = sem_create(0);

  //create and wait
  pthread_create(&tid, NULL, thread3, NULL);
  pthread_join(tid, NULL);

  sem_destroy(sem3);
  sem_destroy(sem4);
}

int main(int argc, char **argv)
{
  assert(tps_init(1) == 0);
  assert(tps_init(1) == -1);

  test_read_write_create_destroy();
  test_clone();
  test_seg();

	return 0;
}
