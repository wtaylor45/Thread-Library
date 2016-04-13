#include "t_lib.h"

/* Running and ready, where ready points to head of queue */
tcb *running;
tcb *ready;

/* Rear pointer, tmp to free memeory, and queue size */
tcb *rear;
tcb *tmp;
int queue_size = 0;

/*
 * Relinquishes control of the CPU to the first thread in the ready queue (if one exists) 
 */
void t_yield()
{
  if(ready){	
    enq(running);
    deq();
	swapcontext(rear->thread_context, running->thread_context);
  }
}

/*
 * Initializes a main thread and its TCB
 */
void t_init()
{
  struct tcb *tmp = malloc(sizeof(tcb));
  tmp->thread_context = (ucontext_t *) malloc(sizeof(ucontext_t));

  getcontext(tmp->thread_context);    /* let tmp be the context of main() */
  
  tmp->thread_id = 0;
  tmp->thread_priority = 1;
  tmp->next = NULL;
  
  running = tmp;
}

/*
 * Creates a new thread with it's own context, a given id & given priority
 * Enqueues it into the ready queue
 */
int t_create(void (*fct)(int), int id, int pri)
{
  size_t sz = 0x10000;
  
  tcb *new_tcb = malloc(sizeof(tcb));
  
  ucontext_t *uc;
  uc = (ucontext_t *) malloc(sizeof(ucontext_t));

  getcontext(uc);
/***
  uc->uc_stack.ss_sp = mmap(0, sz,
       PROT_READ | PROT_WRITE | PROT_EXEC,
       MAP_PRIVATE | MAP_ANON, -1, 0);
***/
  uc->uc_stack.ss_sp = malloc(sz);  /* new statement */
  uc->uc_stack.ss_size = sz;
  uc->uc_stack.ss_flags = 0;
  uc->uc_link = running; 
  makecontext(uc, (void *)fct, 1, id);
  
  new_tcb->thread_id = id;
  new_tcb->thread_priority = pri;
  new_tcb->thread_context = uc;
  new_tcb->next = NULL;
 
  enq(new_tcb);
}

/*
 * Shuts down all threads starting from the calling thread through the ready queue
 */
void t_shutdown(){
	if(running != NULL){
		free(running);
		running = NULL;
	}
	
	tcb *curr = ready;
	while(curr){
		tcb *tmp = curr;
		free(tmp);
		tmp = NULL;
		curr = curr->next;
	}
	free(curr);
}

/* 
 * Terminate the calling thread (running) 
 * Run the next ready thread 
 */
void t_terminate(){
	deq(ready);
	
	free(tmp);
	tmp = NULL;

	setcontext(running->thread_context);
}

/*
 * Add the passed thread to the end of the queue
 */
void enq(tcb *thread){
	if(rear == NULL){
		rear = thread;
		rear->next = NULL;
		ready = rear;
	}else{
		rear->next = thread;
		thread->next = NULL;
		rear = thread;
	}
	queue_size++;
}

/*
 * Remove the first element in the queue, set it to run
 */
void deq(){
	if(ready == NULL){
		printf("Queue is empty! Nothing to dequeue.\n");
	}else{
		tmp = running;
		
		if(ready->next != NULL){	
			running = ready;
			ready = ready->next;
		}else{
			running = ready;
			ready = NULL;
		}
	}
	queue_size--;
}

