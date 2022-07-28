/*
 * G8RTOS_Structure.h
 *
 *  Created on: Jan 12, 2017
 *      Author: Raz Aloni
 */

#ifndef G8RTOS_STRUCTURES_H_
#define G8RTOS_STRUCTURES_H_

#include "G8RTOS.h"
#include <stdbool.h>

/*********************************************** Data Structure Definitions ***********************************************************/

/*
 *  Thread Control Block:
 *      - Every thread has a Thread Control Block
 *      - The Thread Control Block holds information about the Thread Such as the Stack Pointer, Priority Level, and Blocked Status
 *      - For Lab 2 the TCB will only hold the Stack Pointer, next TCB and the previous TCB (for Round Robin Scheduling)
 */

#define MAX_NAME_LENGTH 16

/* Create tcb struct here */
typedef struct tcb_t tcb_t;

struct tcb_t
{
    int32_t* SP;
    bool Asleep;
    uint32_t Sleep_Count;

    uint8_t priority;
    bool isAlive;

    threadId_t threadID;
    char threadName[MAX_NAME_LENGTH];

    tcb_t* Next_TCB;
    tcb_t* Prev_TCB;
    semaphore_t *blocked;
};


/*
 *  Periodic Thread Control Block:
 *      - Holds a function pointer that points to the periodic thread to be executed
 *      - Has a period in us
 *      - Holds Current time
 *      - Contains pointer to the next periodic event - linked list
 */

typedef struct ptcb_t ptcb_t;

struct ptcb_t
{
    void (*Handler)(void);
    uint32_t Period;
    uint32_t Execute_Time;
    uint32_t Current_Time;
    ptcb_t* Next_PTCB;
    ptcb_t* Prev_PTCB;

};

/*********************************************** Data Structure Definitions ***********************************************************/


/*********************************************** Public Variables *********************************************************************/

tcb_t * CurrentlyRunningThread; // temporary next thread for priority scheduling
tcb_t * tempNextThread;

/*********************************************** Public Variables *********************************************************************/




#endif /* G8RTOS_STRUCTURES_H_ */
