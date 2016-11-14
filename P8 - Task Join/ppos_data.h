// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016
//
// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>   // needed for changing contexts
#define STACKSIZE 32768		/* tamanho de pilha das threads */

typedef enum TaskType{ SYSTEM_TASK, USER_TASK } TaskType;

// Estrutura que define uma tarefa
typedef struct task_t
{
    struct task_t *prev, *next; // to use with queue library
    int tid;                    // task's ID
    struct task_t *taskJoin;    // task that will be stored
    ucontext_t context;         // context itself
    int dynamicPriority;        // scale from -20 to +20 (UNIX style)
    int staticPriority;         // scale from -20 to +20 (UNIX style)
    TaskType taskType;          // user task or system task
    int quantum;                // slice of processor time
    int processorTime;          // processor time
    int startTime, endTime;     // 
    int activations;            // how many times it was active
    int exitCode;
} task_t ;

// estrutura que define um semáforo
typedef struct
{
  // preencher quando for necessário
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando for necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando for necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  // preencher quando for necessário
} mqueue_t ;

#endif