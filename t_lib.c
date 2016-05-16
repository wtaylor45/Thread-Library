#include "t_lib.h"

/* Running and ready, where ready points to head of queue */
tcb *running;
tcb *ready_0;
tcb* ready_1;

/* Rear pointer, tmp to free memeory, and queue size */
tcb *rear_0;
tcb *rear_1;
tcb *tmp;

/*
 * Relinquishes control of the CPU to the first thread in the ready queue (if one exists) 
 */
void t_yield()
{
  ualarm(0,0);
  if(ready_0 || ready_1){	
    int pri = enq(running);
    deq();
	if(pri == 0){
		swapcontext(rear_0->thread_context, running->thread_context);
	}else
		swapcontext(rear_1->thread_context, running->thread_context);
  }else{
	  ualarm(1,0);
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
  
  signal(SIGALRM, signal_handler);
  ualarm(1,0);
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

  uc->uc_stack.ss_sp = mmap(0, sz,
       PROT_READ | PROT_WRITE | PROT_EXEC,
       MAP_PRIVATE | MAP_ANON, -1, 0);
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
		free(running->thread_context);
		free(running);
		running = NULL;
	}
	
	tcb *curr = ready_0;
	while(curr){
		tcb *tmp = curr;
		curr = curr->next;
		free(tmp->thread_context);
		free(tmp);
		tmp = NULL;
	}
	curr = ready_1;
	while(curr){
		tcb *tmp = curr;
		curr = curr->next;
		free(tmp->thread_context);
		free(tmp);
		tmp = NULL;
	}
	free(curr);
}

/* 
 * Terminate the calling thread (running) 
 * Run the next ready thread 
 */
void t_terminate(){
	if(ready_0 == NULL && ready_1 == NULL){
		t_shutdown();
	}else{
		remove_running();
		if(deq())
			setcontext(running->thread_context);
	}
}

/*
 * Add the passed thread to the end of the correct queue
 * Return the priority of the enqueued thread
 */
int enq(tcb *thread){
	if(thread->thread_priority == 0){
		if(rear_0 == NULL){
			rear_0 = thread;
			rear_0->next = NULL;
			ready_0 = rear_0;
		}else{
			rear_0->next = thread;
			thread->next = NULL;
			rear_0 = thread;
		}
		return 0;
	}else if(thread->thread_priority == 1){
		if(rear_1 == NULL){
			rear_1 = thread;
			rear_1->next = NULL;
			ready_1 = rear_1;
		}else{
			rear_1->next = thread;
			thread->next = NULL;
			rear_1 = thread;
		}
		return 1;
	}
}

/*
 * Remove the first element in the queue, set it to run
 * Return 1 if there something was deq, 0 if not
 */
int deq(){
	if(ready_0 != NULL){
		tmp = running;
		if(ready_0->next != NULL){	
			running = ready_0;
			ready_0 = running->next;
		}else{
			running = ready_0;
			ready_0 = NULL;
		}
		//Set an alarm to go off after 1 micro second
		ualarm (1, 0);
		return 1;
	}else if(ready_1 != NULL){
		if(running == NULL || running->thread_priority != 0){
			tmp = running;
			
			if(ready_1->next != NULL){	
				running = ready_1;
				ready_1 = running->next;
			}else{
				running = ready_1;
				ready_1 = NULL;
			}
			//Set an alarm to go off after 1 micro second
			ualarm (1, 0);
			return 1;
		}else{
			ualarm(1,0);
			return 1;
		}
	}else{
		ualarm(1,0);
		return 0;
	}
}

/* 
 * Remove the running thread entirely
 */
void remove_running(){
	free(running);
	running = NULL;
}

/*
 * Catch a signal, yield to the next thread
 */
void signal_handler(int x){
	signal(SIGALRM, signal_handler);
	t_yield();
}

/* Create a semaphore from the specified sem pointer */
int sem_init(sem_t **sp, int sem_count)
{
  *sp = malloc(sizeof(sem_t));
  (*sp)->count = sem_count;
  (*sp)->first = NULL;
  (*sp)->last = NULL;
}

/*
 * Tries to gain access to a critical section.
 * Will either continue, or be put in the semaphore's private queue
 */
void sem_wait(sem_t *s){
	sighold(14);
	if(s->count > 0){
		s->count--;
		sigrelse(14);
		return;
	}
	/* Count is 0 */
	sem_enq(s, running);
	deq();
	if(s->last)
		swapcontext(s->last->thread_context, running->thread_context);
	sigrelse(14);
}

/*
 * Tells the other semaphores that the running thread is done with the critical section
 * Removes the first waiting thread in the semaphore queue
 */
void sem_signal(sem_t *s){
	sighold(14);
	if(s->first == NULL || s->count > 0){
		s->count++;
	}else if(s->count <= 0){ /* There is a waiting thread, counter is 0 */
		sem_deq(s);/* Add thread to ready queue */
	}
	sigrelse(14);
}

void sem_destroy(sem_t **s){
	sighold(14);
	
	tcb *curr = (*s)->first;
	while(curr){
		tcb *tmp = curr;
		curr = curr->next;
		free(tmp->thread_context);
		free(tmp);
		tmp = NULL;
	}
	free(curr);
	
	sighold(14);
}

/*
 *--------------------------------------------------------------------
 * THE FOLLOWING FUNCTIONS ARE FOR THE SEMAPHORE QUEUES
 *--------------------------------------------------------------------
 */
 
/*
 * Remove the first thread from the semaphore queue
 * Add it to the ready queue, as per Mesa Semantics
 */
void sem_deq(sem_t *s){
	tmp = s->first;
	if(s->first->next){
		s->first = s->first->next;
	}else
		s->first = NULL;
	enq(tmp);
}

/*
 * Adds a thread to the end of the semaphore queue
 */
void sem_enq(sem_t *s, tcb *thread){
	if(s->last == NULL){
			s->last = thread;
			s->last->next = NULL;
			s->first = s->last;
	}else{
			s->last->next = thread;
			thread->next = NULL;
			s->last = thread;
	}
 }
 
/*
 *--------------------------------------------------------------------
 * MAILBOX
 *--------------------------------------------------------------------
 */ 
 
/*
 *
 */
int mbox_create(mbox **mb){
	*mb = malloc(sizeof(mbox));
	(*mb)->first = NULL;
	(*mb)->last = NULL;
	sem_init(&(*mb)->mbox_sem, 1);
}
 
/*
 *
 */
void mbox_destroy(mbox **mb){
	tcb *curr = (*mb)->first;
	while(curr){
		tcb *tmp = curr;
		curr = curr->next;
		free(tmp->thread_context);
		free(tmp);
	}
	free(curr);
}

/*
 *
 */
void mbox_deposit(mbox *mb, char *msg, int len){
	//Create the message to be deposited
	messageNode *dep = malloc(sizeof(messageNode));
	dep->message = malloc(len);
	strcpy(dep->message, msg);
	dep->length = len;
	dep->sender = -1;
	dep->reciever = -1;
	
	if(mb->last == NULL){
			mb->last = dep;
			mb->last->next = NULL;
			mb->first = mb->last;
	}else{
			mb->last->next = dep;
			dep->next = NULL;
			mb->last = dep;
	}
}

/*
 *
 */
void mbox_withdraw(mbox *mb, char *msg, int *len){
	if(mb->first){
		strcpy(msg, mb->first->message);
		*len = mb->first->length;
		
		messageNode *tmp = mb->first;
		if(mb->first->next){
			mb->first = mb->first->next;
		}else
			mb->first = NULL;
		free(tmp);
	}else{
		printf("There are no messages to withdraw from the mailbox\n");
		*len = 0;
	}
}


 