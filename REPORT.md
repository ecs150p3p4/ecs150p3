
# ECS150 Project 3 Report

* **Introduction**

Our semaphore implementation using waiting queue can provide mutual exclusion 
and syschronization for threads. By maintaining a internal count, the semaphore 
api can keep thack of the number of available resource so that it controls the 
access to common resources.

Our TPS(Thread private storage) api can assign threads the ability of creating
its private storage. By calling the corresponding API, we implement basic 
operations for each thread's private storage such as read, write and clone.


* **Semaphore Implementation**   

A semaphore object consists of a count number, which counts the current number
of available resource, and a wait queue, which a thread will join when it 
require for a resource and the count is 0. In all folowing APIs, we check the 
edge cases first.

When we create a semaphore, we initialize the count to be the total number
of resource units, which is given, and initialize the queue. Semaphore_destroy
will destroy the waiting queue first and release the memory of semaphore itself.

In order to handle the corner case, if one thread calls sem_up to release the
resource and there are some threads waiting in the queue, we don't increment 
the count and let the unblocked thread grab the resource. This strategy can not 
only avoid two threads competing for the available resource, but also prevent 
sratvation for the unblocked thread. So in the sem_up, we check whether there 
are blocked threads waiting in the queue. If so, we let the first blocked 
thread grab the resource. Otherwise we increase the count. When call sem_down, 
we first check if there are available resources. If so, we reduce the count in 
the semaphore. Otherwise, we enqueue the current thread's tid and call 
thread_block. We don't need to recheck the count here since when thread is 
resumed, sem_up can guarantee that this thread will get the resource. All these
operations are in critical sections for mutual exclusion.

For the sem_getvalue function, it also requires us to check the count first. If
resources are available, the target value equal the count. Otherwise the target 
value should be the negative queue length.

* **TPS Implementation** 

We still choose queue as our data structure to implement TPS. We first define a 
page structure consisting of the address pointer and the reference count to the 
current page.Then each TPS object includes the corresponding thread id and a 
page pointer pointing to its page. A global queue is also defined to store all 
pointers to tps objects.

The initialization of TPS first checks if the global tps queue is created. If 
not, it creates the global queue. It also set the segfault handler if desired 
by user. The new segfault handler checks if the segfault is within the TPS 
areas and print corresponding message. 

We create a tps for the current running thread by allocating memory space for a 
tps object, the internal page object and the private storage space for this 
page. We set the port to be PROT_NONE in mmap so the page can't be accessed.
We also initialize the page count to be 1 and the tid in this tps object to be 
the current thread's id. Finaly, we let the pointer of this tps object enter 
the global queue. 

When destroying a tps, we first search the tps queue for current thread's tps 
and delete it in the queue. Then we check if the tps page is refered by 
multiple tps objects by checking the page count. If it is, we only reduce the 
count, otherwise we need to free the memory for that page. Finally, we free 
the tps object.

The read and write api both search the tps queue for current thread's tps. We 
use memcpy for both read and write operation and we also need to change 
permission of the tps pages using mprotect for valid read/write. In order to 
implement the Copy-on-Write cloning, we will check the page reference count in 
tps to protect other tps's page. If there are multiple tps refering to this page, 
we alloate and make a copy for this page first, set the reference count of 
this new page to be 1 and reduce the count of the old page. We let the current
thread's tps to point to this new page and finish the writing.

For the naive clone, we create a new page and use memcpy. But for the
Copy-on-Write cloning, we don't need to copy the whole page at this point. 
Instead, we let the current thread's page pointer point to the target thread's 
page so that both tps share the same page. We then increase the count of that
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

 
