
# ECS150 Project 3 Report

* **Introduction**

Our semaphore implementation using waiting queue can provide mutual exclusion 
and syschronization for threads. By maintaining a internal count, the semaphore 
api can keep track of the number of available resource so that it controls the 
access to common resources.

Our TPS(Thread private storage) api can assign threads the ability of creating
its private storage. By calling the corresponding API, we implement basic 
operations for each thread's private storage such as read, write and clone.


* **Semaphore Implementation**   

A semaphore object consists of a count number, which counts the current number
of available resources, and a waiting queue, which a thread will join when it 
requires a resource and the count is 0. In all folowing APIs, we check the 
edge cases first.

When we create a semaphore, we initialize the count to be the total number
of resources, which is given as an arguement, and initialize the queue. 
Semaphore_destroy will destroy the waiting queue first and release the memory
of semaphore itself.

In order to handle the corner case, if one thread calls sem_up to release the
resource and there are some threads waiting in the queue, we don't increment 
the count and let the unblocked thread grab the resource. This strategy will 
prevent two threads competing for the available resource and also prevent 
starving the unblocked threads. So in the sem_up, we check whether there 
are blocked threads waiting in the queue. If so, we  unblock the thread to be 
scheduled later and don't increment the count. Otherwise we increase the count. 
When we call sem_down, we first check if there are available resources. If so, 
we reduce the count, otherwise, we enqueue the current thread's tid and call 
thread_block. We don't need to recheck the count when the thread is later 
resumed since sem_up guarantees that this thread will get the resource. All 
these operations are in critical sections for mutual exclusion since they 
involve interacting with the queue and checking the count, which are shared 
variables.

For the sem_getvalue function, we check the count, and if above zero return it. 
Otherwise we return the negative length of the queue.

* **TPS Implementation** 

We still choose queue as our data structure to implement TPS. We first define a 
page structure consisting of the address pointer and the reference count to the 
current page. Then each TPS object includes the corresponding thread id and a 
pointer to the page object. A global queue is also defined to store all the 
pointers to tps objects.



The initialization of TPS first checks if the global tps queue is created to 
check if init has already been called. If not, it creates the global queue. 
It also set the segfault handler if desired by user. The new segfault handler 
checks if the segfault is within the TPS areas by searching the queue to see 
if the seg fault address matches any of the TPS addresses in the queue and then
prints the corresponding message, following the template given in the prompt. 

We create a TPS for the current running thread by allocating memory space for a 
TPS object, the internal page object and the private storage space for this 
page. We set the port to be PROT_NONE in mmap so the page can't be accessed.
We also initialize the page count to be 1 and the tid in this tps object to be 
the current thread's id. Finaly, we enqueue the TPS pointer into the global 
queue so it can be accessed in later calls. 

When destroying a TPS, we first search the TPS queue for the current thread's TPS 
and delete it from the queue. Then we check if the TPS page is pointed to by
multiple TPS objects by checking the page reference count. If it is, we only 
reduce the count, otherwise we need to free the memory for that page. Finally, 
we free the TPS object.

The read and write api both search the tps queue for the current thread's TPS. We 
use memcpy for both read and write operation but before we do that we change the 
permission of the TPS pages using mprotect to allow the read/write opperation. In 
order to implement the Copy-on-Write cloning in the write function, we will check 
the page reference count before writing. If there are multiple TPSs refering to this page, 
we allocate and make a copy of page first, set the reference count of 
the new page to be 1 and reduce the count of the old page. We let the current
thread's TPS to point to this new page before completing the actual write.

For Copy-on-Write cloning, we don't need to copy the whole page inside clone. 
Instead, we let the current thread's page pointer point to the target thread's 
page so that both TPSs share the same page. We then increase the count of that
page.


* **Test**  

We run and passed all the given test cases for both semaphore and TPS. 

To furhter test our tps api, we create tps_err.c. We check all the error cases
for tps create, destroy, read, write and clone. We also test illegal 
memory access by wrapping mmap to check whether the segfault handler works 
properly. 

In the test code tps.c, besides testing read, write and clone functions, we 
also test destroying a cloned page immediately. This is also an edge case since 
we shouldn't free the whole page.

Our tps api passed all the test cases that we designed. 

 
