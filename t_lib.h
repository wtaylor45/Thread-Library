/*
 * types used by thread library
 */
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>

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


/* Semaphore Struct */
typedef struct {
  int count;
  tcb *first;
  tcb *last;
} sem_t; 

/* Message Struct */
typedef struct messageNode {
  char       *message;
  int         length;
  int sender;
  int reciever;
  struct msg *next;
} messageNode;

/* Mailbox struct */
typedef struct {
  struct messageNode *first;
  struct messageNode *last;
  sem_t *mbox_sem;
} mbox;

/* Queue functions */
int enq(tcb *thread);
int deq();
void remove_running();

/* Signal Handler */
void signal_handler(int sig);

/* Semaphore Functions */
int sem_init(sem_t **sp, int count);
void sem_wait(sem_t *sp);
void sem_signal(sem_t *sp);
void sem_destroy(sem_t **sp);
void sem_deq(sem_t *s);
void sem_enq(sem_t *s, tcb *thread);

/* Mailbox Stuff */
int mbox_create(mbox **mb); 
void mbox_deposit(mbox *mb, char *msg, int len);
void mbox_withdraw(mbox *mb, char *msg, int *len);
void mbox_destory(mbox **mb);
