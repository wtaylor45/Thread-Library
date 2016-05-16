/* 
 * thread library function prototypes
 */
void t_create(void(*function)(int), int thread_id, int priority);
void t_yield(void);
void t_init(void);
void t_shutdown();
void t_terminate();

typedef void sem_t;  // for semaphore

typedef void mbox; 
typedef void messageNode;