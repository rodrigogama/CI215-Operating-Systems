#include <stdio.h>
#include <stdlib.h>

#include "ppos.h"       // provided by the prof. Maziero. All functions MUST follow the prototypes in this header
#include "ppos_data.h"  // ppos data structure
#include <ucontext.h>   // for changing context

//#define DEBUG 1         // for debbuging purposes

task_t task_main;
task_t *task_corrente;

int currentId = 1;  // keeping track for our contexts' ids
task_t currentTask; //
task_t mainTask;    //

/** @function ppos_init
 *  Init SO's internal structures. 
 *  For now, it only initiates some variables and deactivate the buffer used by 'printf' function
**/
void ppos_init() {
    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf(stdout, 0, _IONBF, 0);
    
    // getting the current context
    ucontext_t context;
	getcontext(&context);
    
    // setting up the main task variables
    mainTask.next = NULL;
	mainTask.prev = NULL;
	mainTask.tid = id;          // starting at 1
	mainTask.context = context; // current context

    // updating the current context
	currentTask = &mainTask;

	#ifdef DEBUG
	printf("ppos_init: first task created at id: %d\n",mainTask.tid);  // this must be 1
	#endif
}