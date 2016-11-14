#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"       // provided by the prof. Maziero. All functions MUST follow the prototypes in this header
#include "ppos_data.h"  // ppos data structure
#include <ucontext.h>   // for changing context
#include "queue.h"
#include <sys/time.h>
#include <signal.h>

//#define DEBUG        // for debbuging purposes
#define DEFAULT_QUANTUM 20

int currentId = 1;     // keeping track of our contexts' ids. Starting at 1 to avoid one operation currentId++
int aging = -1;    // task aging
int clockTicks = 0;
int startTime = 0;
int endTime = 0;

task_t *currentTask;   // pointer to the current task, so that I can keep track of it during context exchange
task_t mainTask;       // structure to the main task
task_t dispatcherTask; // to create a dispatcher as a task
task_t *queueTask = NULL; // queue of tasks using FCFS policy.

/* to compute time */
struct sigaction action;
struct itimerval timer;

/* scheduler and dispatcher function headers that are not in ppos.h */
void dispatcher_body();
task_t *scheduler();
void handler(int signum);
unsigned int systime();

/** @function ppos_init
 *  Init SO's internal structures. 
 *  For now, it only initiates some variables and deactivate the buffer used by 'printf' function
**/
void ppos_init() {
    // deactivate default output buffer (stdout), used by 'printf' function
    setvbuf(stdout, 0, _IONBF, 0);
    
    // setting up the main task variables
    mainTask.next = NULL;   // at this point there's no next task
	mainTask.prev = NULL;   // nor previous task
	mainTask.tid = currentId; 	   // starting at 1
    mainTask.taskType = SYSTEM_TASK;
    mainTask.staticPriority = 0;
	mainTask.dynamicPriority = 0;
	mainTask.activations = 0;
    mainTask.processorTime = 0;
    
    getcontext(&mainTask.context); // gets the current context
    queue_append((queue_t **) &queueTask,(queue_t*) &mainTask);

	currentTask = &mainTask;

	#ifdef DEBUG
	printf("ppos_init: first task created at id: %d\n", mainTask.tid);  // this must be 1
	#endif
    
    // there's no need to set up dispatcherTask at this point since it's gonna happen in task_create.
    // creating dispatcher as a task, passing dispatcher_body function and no args
	task_create(&dispatcherTask, (void*)(dispatcher_body), NULL);  
    dispatcherTask.taskType = USER_TASK;
    // dispatcherTask.activations = 0;
    // dispatcherTask.processorTime = 0;

    action.sa_handler = handler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;

    if (sigaction(SIGALRM, &action, 0) < 0) {
        perror("ppos_init: sigaction error ");
        exit(1);
    }

    timer.it_value.tv_usec = 100; // primeiro disparo, em micro-segundos
	timer.it_value.tv_sec  = 0;   // primeiro disparo, em segundos
	timer.it_interval.tv_usec = 1000; // disparos subsequentes, em micro-segundos
	timer.it_interval.tv_sec  = 0;   // disparos subsequentes, em segundos

	// arma o temporizador ITIMER_REAL (vide man setitimer)
	if (setitimer(ITIMER_REAL, &timer, 0) < 0) {
        perror ("ppos_init: setitimer error ");
        exit (1);
    }

    task_yield();
}

/** @function task_create
 *  Create a new task. Return the new task's id.
 *  @param  {task_t} *task     - task's descriptor
 *  @param  {void} *start_func - task's body function
 *  @param  {void} *arg        - task's args
 *  @return {int} - new task's id. If the task is created correctly, id should be > 0 and smaller than 0 otherwise.
**/
int task_create(task_t *task, void(*start_func)(void *), void *arg) {
    task->prev = NULL; // just to make sure that the task
    task->next = NULL; // has no previous or next tasks
    task->tid = currentId++; // increases task id
    task->taskType = task != &dispatcherTask && task != &mainTask ? USER_TASK : SYSTEM_TASK;
    task->processorTime = 0;
    task->activations = 0;
    task->exitCode = 0; 

    // getting task's context
    getcontext(&task->context);

    char *stack;
    stack = malloc(STACKSIZE);

    if (!stack) // can't create the stack, so returns -1 (no id) 
        return -1;
    
    task->context.uc_stack.ss_sp = stack;       // stack is stored in task's context
    task->context.uc_stack.ss_size = STACKSIZE; // stack's size
    task->context.uc_stack.ss_flags = 0;  // the alternate signal stack is currently disabled
    task->context.uc_link = 0;  // link to the next context to be executed, which is NULL, so 0

    // create the context for the given task
    makecontext(&task->context, (void*)(*start_func), 1, arg);

    if (task != &dispatcherTask && task != &mainTask) {
        queue_append((queue_t **)&queueTask, (queue_t *)task); // append the task created to the queue
        task->staticPriority = 0; // default priority
        task->dynamicPriority = 0; // default priority
        task->taskType = SYSTEM_TASK;
    }

    #ifdef DEBUG
    printf ("task_create: task created at id: %d\n", task->tid) ;
    #endif

    return task->tid; // returns the id for the task created
}

/** @function task_exit
 *  Finish the current task, indicating a closing status.
 *  For now, we will not use exitCode
 *  @param  {int} exitCode - exit code returned from the current task
**/
void task_exit(int exitCode) {
    printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n", currentTask->tid, systime(), currentTask->processorTime, currentTask->activations);

    // switching to dispatcherTask or mainTask, based on what step we're at.
    //currentTask != &dispatcherTask ? task_switch(&dispatcherTask) : task_switch(&mainTask);

    task_t *task;
    currentTask->exitCode = exitCode;

    if (currentTask == &dispatcherTask){
		task_switch(&mainTask);
	}
	else {
        while (queue_size((queue_t *)queueTask) > 0) {
            task = queue_remove((queue_t **)&(currentTask->taskJoin), (queue_t *)currentTask->taskJoin);
            queue_append((queue_t **)&queueTask, (queue_t *)task);
        }

        //queue_remove((queue_t **) &queueTask,(queue_t*) currentTask);
        task_switch(&dispatcherTask);
    }

    #ifdef DEBUG
	printf("task_exit: exiting task %d\n", currentTask->tid);
    #endif
}

/** @function task_switch
 *  Switches two contexts
 *  @param  {task_t} *task - task that will be its context changed
 *  @return {int} - 0 if it succeed, -1 otherwise
**/
int task_switch(task_t *task) {
    task_t *aux = currentTask;
    currentTask = task;

    #ifdef DEBUG
	printf("task_switch: switching tasks from %d -> %d\n", aux->tid, task->tid);
    #endif

    return swapcontext(&aux->context, &task->context);
}

/** @function task_id
 *  Return a task id. If it is the main task, return 0. Otherwise, return the current task id
 *  @return {int} - 0 if it is the main task, > 0 otherwise
**/
int task_id() {
    return currentTask == &mainTask ? 0 : currentTask->tid;
}

/** @function dispatcher
 *  Allow some task to go back to the end of the queue, returning the processor to dispatcher
**/
void task_yield() {
    //if (currentTask != &mainTask)
    queue_append((queue_t **)&queueTask, (queue_t *)currentTask);
    
    // returning the processor to dispatcher
    task_switch(&dispatcherTask);
}

/** @function task_setprio
 *  Set the priority of a given task
 *  @param  {task_t} *task - task to set the priority to. If task is NULL, the current task has its priority set.
 *  @param  {int} prio - priority from -20 to +20
**/
void task_setprio(task_t *task, int prio) {
    #ifdef DEBUG
        if (prio <= -20 || prio >= 20)
            task == NULL ?
                printf("task_setprio: task->tid: %d // task priority (%d) is not between -20 and +20\n", currentTask->tid, prio) :
                printf("task_setprio: task->tid: %d // task priority (%d) is not between -20 and +20\n", task->tid, prio);

        task == NULL ?
            printf("task_setprio: task->tid: %d set to task->staticPriority: %d\n", currentTask->tid, currentTask->staticPriority) :
            printf("task_setprio: task->tid: %d set to task->staticPriority: %d\n", task->tid,task->staticPriority);
    #endif

    if (prio >= -20 && prio <= 20) {
        if (task == NULL) {
            currentTask->staticPriority = prio;
            currentTask->dynamicPriority = prio;
        }
        else {
            task->staticPriority = prio;
            task->dynamicPriority = prio;
        }
    }
}

/** @function task_getprio
 *  Get the priority of a given task
 *  @param  {task_t} *task - task to get the priority from
 *  @return {int} - task's static priority
**/
int task_getprio(task_t *task) {
    #ifdef DEBUG
        task == NULL ?
            printf("task_getprio: task->tid: %d // task->staticPriority: %d\n", currentTask->tid, currentTask->staticPriority) :
            printf("task_getprio: task->tid %d // task->staticPriority: %d\n", task->tid,task->staticPriority);
    #endif

    return task == NULL ? currentTask->staticPriority : task->staticPriority;
}

int task_join(task_t *task) {
    if (task == NULL) {
        //enable_preemption(1);
        return -1;
    } else {
        queue_remove((queue_t **)&queueTask, (queue_t *)currentTask);
        queue_append((queue_t **)&(task->taskJoin), (queue_t *)currentTask);
        task_yield();
        //enable_preemption(1);
        return task->exitCode;
    }

    return 0;
}

/** @function scheduler
 *  Choose the next task to be executed in every context exchange. The criteria is based on FCFS (First-Come, First-Served) policy.
 *  @return {task_t} *task
**/
task_t *scheduler() {
    int userTasks = queue_size((queue_t *)queueTask);
    task_t *priorityTask = NULL;

    if (userTasks > 0) {
        task_t *nextTask = queueTask->next;
        priorityTask = queueTask;

        do {
            if (nextTask->dynamicPriority < priorityTask->dynamicPriority)
                priorityTask = nextTask;
            nextTask = nextTask->next;
        } while(nextTask != queueTask);

        nextTask = queueTask;
        
        do {
            if (nextTask != priorityTask) nextTask->dynamicPriority += aging;
            else priorityTask->dynamicPriority = priorityTask->staticPriority;
            
            nextTask = nextTask->next;
        } while (nextTask != queueTask);
    }

    #ifdef DEBUG
    if (!priorityTask) printf("scheduler: there is no element to schedule. Returned NULL\n");
	#endif

    return priorityTask;
}

/** @function dispatcher
 *  Responsible for general control of tasks.It finishes when there're no more user tasks.
**/
void dispatcher_body() {
    int userTasks = queue_size((queue_t *)queueTask);
    task_t *nextTask = NULL;
     
    // execute while there're user tasks
    while (userTasks > 0) {
        nextTask = scheduler();
        currentTask->activations++;

        if (nextTask) {
            startTime = systime();
            nextTask->quantum = DEFAULT_QUANTUM;
            queue_remove((queue_t **)&queueTask,(queue_t*)nextTask);
            task_switch(nextTask);
        }
        
        // remove the first element in the queue because we're using FCFS 
        userTasks = queue_size((queue_t *)queueTask);
    }
    
    // exiting task at 0, but this number represents nothing for now, thus, could be any number
    task_exit(0);
}

/** @function handler
 *  Works as an interrupt handler.
**/
void handler(int signum) {
    clockTicks++;

	if (currentTask->taskType == SYSTEM_TASK) {
		if (currentTask->quantum == 0) {
            endTime = systime();

			currentTask->processorTime += endTime - startTime;
			currentTask->activations++;
			startTime = 0;
            endTime = 0;
			task_yield();
		}
		else currentTask->quantum--;
    }
}

/** @function systime
 *  Works as a clock getting time in miliseconds
 *  @return {int} clockTicks
**/
unsigned int systime() {
	return clockTicks;
}