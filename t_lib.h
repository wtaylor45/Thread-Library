/*
 * types used by thread library
 */
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <sys/mman.h>

/*
 * Thread control block for each thread
 */
struct tcb {
	int thread_id;
	int thread_priority;
	struct ucontext_t *thread_context;
	struct tcb *next;
};

typedef struct tcb tcb;

/* Queue functions */
void enq(tcb *thread);
void deq();

