#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"       // provided by the prof. Maziero. All functions MUST follow the prototypes in this header
#include "ppos_data.h"  // ppos data structure
#include <ucontext.h>   // for changing context
#include "queue.h"

//#define DEBUG        // for debbuging purposes

int currentId = 1;     // keeping track of our contexts' ids. Starting at 1 to avoid one operation currentId++
task_t *currentTask;   // pointer to the current task, so that I can keep track of it during context exchange
task_t mainTask;       // structure to the main task

task_t dispatcherTask; // to create a dispatcher as a task
task_t *queueTask = NULL; // queue of tasks using FCFS policy.

/* scheduler and dispatcher function headers that are not in ppos.h */
void dispatcher_body();
task_t *scheduler();


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
	getcontext(&mainTask.context); // gets the current context

    // updating the current context
	currentTask = &mainTask;

	#ifdef DEBUG
	printf("ppos_init: first task created at id: %d\n", mainTask.tid);  // this must be 1
	#endif
    
    // there's no need to set up dispatcherTask at this point since it's gonna happen in task_create.
    // creating dispatcher as a task, passing dispatcher_body function and no args
	task_create(&dispatcherTask, (void*)(dispatcher_body), NULL);  
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

    // append the task created to the queue
    queue_append((queue_t **)&queueTask, (queue_t *)task);

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
    // switching to dispatcherTask or mainTask, based on what step we're at.
    currentTask != &dispatcherTask ? task_switch(&dispatcherTask) : task_switch(&mainTask);

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

/** @function scheduler
 *  Choose the next task to be executed in every context exchange. The criteria is based on FCFS (First-Come, First-Served) policy.
 *  @return {task_t} *task
**/
task_t *scheduler() {
    task_t * res = NULL;
    res = (task_t *)queue_remove((queue_t **)&queueTask, (queue_t *)queueTask);
    
    #ifdef DEBUG
    if (!res) printf("scheduler: there is no element to remove. Returned NULL\n");
	#endif
    
    return res;
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
        
        if (nextTask)
            task_switch(nextTask);
        
        // remove the first element in the queue because we're using FCFS 
        userTasks = queue_size((queue_t *)queueTask);
    }
    
    // exiting task at 0, but this number represents nothing for now, thus, could be any number
    task_exit(0);
}

/** @function dispatcher
 *  Allow some task to go back to the end of the queue, returning the processor to dispatcher
**/
void task_yield() {
    if (currentTask != &mainTask)
        queue_append((queue_t **)&queueTask, (queue_t *)currentTask);
    
    // returning the processor to dispatcher
    task_switch(&dispatcherTask);
}