// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016
//
// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#ifndef NULL
#define NULL ((void *) 0)
#endif
#define STACKSIZE 32768
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>

// #include <ucontext.h>   // needed for changing contexts
// #define STACKSIZE 32768	// 32kb - tamanho de pilha das threads

typedef enum TaskType{ SYSTEM_TASK, USER_TASK } TaskType;

// Estrutura que define uma tarefa
typedef struct task_t {
    // struct task_t *prev, *next, *join;
    // ucontext_t context;
    // TaskType taskType;
    // int tid;
    // int priority, age;
    // int quantum;
    // int startTime, endTime;
    // int processorTime; 
    // int activations;
    // int exitCode;
    // int dynamicPriority;
    // int staticPriority;

    ucontext_t context;
    struct task_t *prev;
    struct task_t *next;
    struct task_t **queue;
    int id, morreu, prio_static, prio_din, user, quantum, activations, ready, wait;
    unsigned int exec_time, proc_time;
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
